// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool\kc_terrain\TerrainMaterialGeneratorOgreMaterial.h>
#include <asset_collector_tool\kc_terrain\TerrainComponent.h>
#include <asset_collector_tool\kc_terrain\TerrainDefinition.h>
#include "qsf/renderer/material/MaterialSystem.h"
#include "qsf/renderer/material/material/MaterialManager.h"
#include "qsf/renderer/RendererSystem.h"
#include "qsf/settings/CompositingSettingsGroup.h"
#include "qsf/plugin/QsfAssetTypes.h"
#include "qsf/map/Map.h"
#include "qsf/map/Entity.h"
#include "qsf/map/query/ComponentMapQuery.h"
#include "qsf/component/base/TransformComponent.h"
#include "qsf/asset/Asset.h"
#include "qsf/asset/AssetSystem.h"
#include "qsf/asset/AssetProxy.h"
#include "qsf/QsfHelper.h"

#include <OGRE/OgreMaterialManager.h>
#include <ogre\Ogre.h>
#include <OGRE/Terrain/OgreTerrain.h>
#include <OGRE/Terrain/OgreTerrainGroup.h>
#include <ogre\OgreTechnique.h>
#include <qsf\log\LogSystem.h>
#include <qsf/file/cache/FileCacheManager.h>
#include <qsf/Qsf.h>
#include <qsf/application/Application.h>
#include <qsf/file/FileSystem.h>
#include <qsf/renderer/RendererSystem.h>
#include <qsf/renderer/texture/TextureStreamingManager.h>
#include <qsf/application/Application.h>
#include <qsf/renderer/texture/TextureStreamer.h>
#include <qsf/renderer/material/cache/MaterialSystemCacheManager.h>
#include <qsf/file/cache/FileCache.h>
#include <qsf/file/helper/FileListing.h>
//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		//[-------------------------------------------------------]
		//[ Global functions                                      ]
		//[-------------------------------------------------------]
		void setIdentityMapTransformMaterialProperty(qsf::Material& material, qsf::MaterialPropertyId materialPropertyId)
		{
			material.setPropertyById(materialPropertyId, qsf::MaterialPropertyValue::fromFloat4(0.0f, 0.0f, 1.0f, 1.0f));
		}

		void setMapTransformMaterialProperty(qsf::Material& material, const kc_terrain::TerrainComponent& terrainComponent, const Ogre::Terrain& ogreTerrain, qsf::MaterialPropertyId materialPropertyId, bool scale)
		{
			// TODO(co) Proper texture coordinate offset generation
			const float globalTerrainWorldSize = terrainComponent.getTerrainWorldSize();
			auto a = const_cast<kc_terrain::TerrainComponent*>(&terrainComponent);
			const int terrainChunksPerEdge = a->kc_getTerrainChunksPerEdge();

			float xOffset = ogreTerrain.getPosition().x - ogreTerrain.getWorldSize() * 0.5f + globalTerrainWorldSize * 0.5f;
			float yOffset = ogreTerrain.getPosition().z - ogreTerrain.getWorldSize() * 0.5f + globalTerrainWorldSize * 0.5f;
			xOffset /= globalTerrainWorldSize;
			yOffset /= globalTerrainWorldSize;
			if (scale)
			{
				material.setPropertyById(materialPropertyId, qsf::MaterialPropertyValue::fromFloat4(xOffset, yOffset, 1.0f / terrainChunksPerEdge, 1.0f / terrainChunksPerEdge));
			}
			else
			{
				material.setPropertyById(materialPropertyId, qsf::MaterialPropertyValue::fromFloat4(xOffset, yOffset, 1.0f, 1.0f));
			}
		}

		void setLayerBlendMapComponentUvMultiplier(const Ogre::Terrain& ogreTerrain, uint32 layerIndex, const std::string& layerIndexAsString, qsf::Material& terrainMaterial)
		{
			// Gather data
			const int   blendMapIndex = (layerIndex - 1) / 4;
			const int   blendMapComponent = (layerIndex - 1) % 4;
			const float uvMultiplier = ogreTerrain.getLayerUVMultiplier(layerIndex);

			// Set terrain material property value
			terrainMaterial.setPropertyById(qsf::StringHash("LayerBlendMapComponentUvMultiplier" + layerIndexAsString), qsf::MaterialPropertyValue::fromFloat3(static_cast<float>(blendMapIndex), static_cast<float>(blendMapComponent), uvMultiplier));
		}

		void setTerrainLayerMaterialProperties(const qsf::MaterialProperties& layerMaterialProperties, const std::string& layerIndexAsString, qsf::Material& terrainMaterial)
		{
			// Gather parameters for maximum height, blend falloff, visible threshold and parallax scale
			float maximumHeight = 0.0f;
			float blendFalloff = 0.1f;
			float parallaxOffset = 0.5f;
			float parallaxScale = 0.5f;
			const qsf::MaterialPropertyValue* layerMaterialPropertyValue = layerMaterialProperties.getPropertyById("MaximalHeight");	// TODO(co) Sadly, "MaximalHeight" is out there used in the material assets. Should have been "MaximumHeight".
			if (nullptr != layerMaterialPropertyValue)
			{
				maximumHeight = layerMaterialPropertyValue->getFloatValue();
			}
			layerMaterialPropertyValue = layerMaterialProperties.getPropertyById("BlendFalloff");
			if (nullptr != layerMaterialPropertyValue)
			{
				blendFalloff = layerMaterialPropertyValue->getFloatValue();
			}
			layerMaterialPropertyValue = layerMaterialProperties.getPropertyById("ParallaxOffset");
			if (nullptr != layerMaterialPropertyValue)
			{
				parallaxOffset = layerMaterialPropertyValue->getFloatValue();
			}
			layerMaterialPropertyValue = layerMaterialProperties.getPropertyById("ParallaxScale");
			if (nullptr != layerMaterialPropertyValue)
			{
				parallaxScale = layerMaterialPropertyValue->getFloatValue();
			}

			// Set terrain material property value
			terrainMaterial.setPropertyById(qsf::StringHash("LayerMaterialProperties" + layerIndexAsString), qsf::MaterialPropertyValue::fromFloat4(maximumHeight, blendFalloff, parallaxOffset, parallaxScale));
		}


		//[-------------------------------------------------------]
		//[ Anonymous detail namespace                            ]
		//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace kc_terrain
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	TerrainMaterialGeneratorOgreMaterial::Profile::Profile(Ogre::TerrainMaterialGenerator* parent, const Ogre::String& name, const Ogre::String& desc, uint64 ColorMap) :
		Ogre::TerrainMaterialGenerator::Profile(parent, name, desc),
		 mShaderGen(0)
		, mLayerNormalMappingEnabled(true)
		, mLayerParallaxMappingEnabled(true)
		, mLayerSpecularMappingEnabled(true)
		, mGlobalColourMapEnabled(true)
		, mLightmapEnabled(true)
		, mCompositeMapEnabled(true)
		, mReceiveDynamicShadows(true)
		, mPSSM(0)
		, mDepthShadows(false)
		, mLowLodShadows(false)
		, mSM3Available(false)
		, mSM4Available(false)
	{
		m_profil_ColorMap = ColorMap;
		// Nothing to do in here
	}

	TerrainMaterialGeneratorOgreMaterial::Profile::~Profile()
	{
		qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
		materialManager.destroyElement(qsf::StringHash(mMatName));
		materialManager.destroyElement(qsf::StringHash(mMatNameComp));
	}


	//[-------------------------------------------------------]
	//[ Public virtual Ogre::TerrainMaterialGeneratorOgreMaterial::Profile methods ]
	//[-------------------------------------------------------]
	bool TerrainMaterialGeneratorOgreMaterial::Profile::isVertexCompressionSupported() const
	{
		return false;
	}

	Ogre::MaterialPtr TerrainMaterialGeneratorOgreMaterial::Profile::generate(const Ogre::Terrain* ogreTerrain)
	{
		// re-use old material if exists
		Ogre::MaterialPtr mat = ogreTerrain->_getMaterial();
		if (mat.isNull())
		{
			Ogre::MaterialManager& matMgr = Ogre::MaterialManager::getSingleton();

			// it's important that the names are deterministic for a given terrain, so
			// use the terrain pointer as an ID
			const Ogre::String& matName = ogreTerrain->getMaterialName();
			mat = matMgr.getByName(matName);
			if (mat.isNull())
			{
				mat = matMgr.create(matName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			}
		}
		// clear everything
		mat->removeAllTechniques();

		// Automatically disable normal & parallax mapping if card cannot handle it
		// We do this rather than having a specific technique for it since it's simpler
		Ogre::GpuProgramManager& gmgr = Ogre::GpuProgramManager::getSingleton();
		if (!gmgr.isSyntaxSupported("ps_4_0") && !gmgr.isSyntaxSupported("ps_3_0") && !gmgr.isSyntaxSupported("ps_2_x")
			&& !gmgr.isSyntaxSupported("fp40") && !gmgr.isSyntaxSupported("arbfp1") && !gmgr.isSyntaxSupported("glsl")
			&& !gmgr.isSyntaxSupported("glsles"))
		{
		//kc do this stuff
			//setLayerNormalMappingEnabled(false);
			//setLayerParallaxMappingEnabled(false);
		}

		addTechnique(mat, ogreTerrain, HIGH_LOD);

		// LOD
		//kc add
		/*if (mCompositeMapEnabled)
		{
			addTechnique(mat, terrain, LOW_LOD);
			Material::LodValueArray lodValues;
			lodValues.push_back(TerrainGlobalOptions::getSingleton().getCompositeMapDistance());
			mat->setLodLevels(lodValues);
			Technique* lowLodTechnique = mat->getTechnique(1);
			lowLodTechnique->setLodIndex(1);
		}

		updateParams(mat, terrain);*/
		if (mat.get())
		{
			QSF_LOG_PRINTS(INFO,"Material exists" << mat.get()->isLoaded() << " mat name " << mat.get()->getName())
			//mat.get()->compile();
			//mat.get()->load();
			//ogreTerrain->getGlobalColourMap()
			
			auto materialManager = &QSF_MATERIAL.getMaterialManager();
			mat->setDiffuse(Ogre::Real(255.f),Ogre::Real(0.f),Ogre::Real(0.f),Ogre::Real(1.f));
		}
		return mat;
		//ogreTerrain->enti
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::addTechnique(
		const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain, TerrainMaterialGeneratorOgreMaterial::Profile::TechniqueType tt)
	{
		Ogre::Technique* tech = mat->createTechnique();

		// Only supporting one pass
		Ogre::Pass* pass = tech->createPass();

		Ogre::GpuProgramManager& gmgr = Ogre::GpuProgramManager::getSingleton();
		Ogre::HighLevelGpuProgramManager& hmgr = Ogre::HighLevelGpuProgramManager::getSingleton();
		/*if (!mShaderGen)
		{
			//mShaderGen = OGRE_NEW ShaderHelper();
			bool check2x = mLayerNormalMappingEnabled || mLayerParallaxMappingEnabled;
			/*if (hmgr.isLanguageSupported("hlsl") &&
				((check2x && gmgr.isSyntaxSupported("ps_4_0"))))
			{
				mShaderGen = OGRE_NEW ShaderHelperHLSL();
			}
			else if (hmgr.isLanguageSupported("glsl") &&
				Ogre::Root::getSingleton().getRenderSystem()->getNativeShadingLanguageVersion() >= 150)
			{
				mShaderGen = OGRE_NEW ShaderHelperGLSL();
			}
			else if (hmgr.isLanguageSupported("glsles"))
			{
				mShaderGen = OGRE_NEW ShaderHelperGLSLES();
			}
			else if (hmgr.isLanguageSupported("cg"))*/
			/*{
				mShaderGen = OGRE_NEW ShaderHelperCg();
			}

			// check SM3 features
			mSM3Available = Ogre::GpuProgramManager::getSingleton().isSyntaxSupported("ps_3_0");
			mSM4Available = Ogre::GpuProgramManager::getSingleton().isSyntaxSupported("ps_4_0");
		}*/

		//Ogre::HighLevelGpuProgramPtr vprog = mShaderGen->generateVertexProgram(this, terrain, tt);
		//Ogre::HighLevelGpuProgramPtr fprog = mShaderGen->generateFragmentProgram(this, terrain, tt);

		//pass->setVertexProgram(vprog->getName());
		//pass->setFragmentProgram(fprog->getName());

		if (tt == HIGH_LOD || tt == RENDER_COMPOSITE_MAP)
		{
			// global normal map ... do we have one?
			Ogre::TextureUnitState* tu = pass->createTextureUnitState();
			
			//tu->setTextureName(terrain->getTerrainNormalMap()->getName());
			// Bugfix for D3D11 Render System
			// tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);

			// global colour map
			//if (terrain->getGlobalColourMapEnabled() && isGlobalColourMapEnabled())
			{
				qsf::GlobalAssetId ColorMapAsset;
				if (m_profil_ColorMap == qsf::getUninitialized<uint64>())
				{
					ColorMapAsset = qsf::AssetProxy("qsf/texture/default/missing").getGlobalAssetId();
				}
				else
				{
					ColorMapAsset = m_profil_ColorMap;
				}
				//set color map
				QSF_LOG_PRINTS(INFO,"set color map")
					try
				{
					Ogre::TexturePtr mTex = Ogre::TextureManager::getSingleton().load(qsf::AssetProxy(ColorMapAsset).getLocalAssetName()+".dds" , "General");
					if(mTex.get() == nullptr)
					QSF_LOG_PRINTS(INFO,"mTex is null")
					//tu->setTextureName(mTex->getName());
					
					tu->setTexture(mTex);
					/*tu = */tu->setTextureName(qsf::AssetProxy(ColorMapAsset).getLocalAssetName());
					pass->setDiffuse((Ogre::Real)255.0, (Ogre::Real)0.0, (Ogre::Real)0.0, (Ogre::Real)0.3);
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO, "set color map problem "<< e.what())
				}
				QSF_LOG_PRINTS(INFO, "set color map DONE")
				//tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			}
			return;
			// light map
			/*if (isLightmapEnabled())
			{
				tu = pass->createTextureUnitState(terrain->getLightmap()->getName());
				//tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			}*/

			// blend maps
			//Ogre::uint maxLayers = getMaxLayers(terrain);
			//Ogre::uint numBlendTextures = std::min(terrain->getBlendTextureCount(maxLayers), terrain->getBlendTextureCount());
			//Ogre::uint numLayers = std::min(maxLayers, static_cast<uint>(terrain->getLayerCount()));

			const uint32 maxLayers = getMaxLayers(terrain);
			uint32 numBlendTextures = std::min(terrain->getBlendTextureCount(maxLayers), terrain->getBlendTextureCount());
			uint32 numLayers = std::min(maxLayers, static_cast<uint32>(terrain->getLayerCount()));
			for (uint8 i = 0; i < numBlendTextures; ++i)
			{
				//tu = pass->createTextureUnitState(terrain->getBlendTextureName(i));
				//tu->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			}

			// layer textures
			for (uint8 i = 0; i < numLayers; ++i)
			{
				// diffuse / specular
				pass->createTextureUnitState(terrain->getLayerTextureName(i, 0));
				// normal / height
				pass->createTextureUnitState(terrain->getLayerTextureName(i, 1));
			}

		}
		else //duno how to set up composite map
		{
			// LOW_LOD textures
			// composite map
			Ogre::TextureUnitState* tu = pass->createTextureUnitState();
			tu->setTextureName(terrain->getCompositeMap()->getName());
			//tu->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

			// That's it!

		}

		// Add shadow textures (always at the end)
		/*if (isShadowingEnabled(tt, terrain))
		{
			Ogre::uint numTextures = 1;
			if (getReceiveDynamicShadowsPSSM())
			{
				numTextures = (uint)getReceiveDynamicShadowsPSSM()->getSplitCount();
			}
			for (Ogre::uint i = 0; i < numTextures; ++i)
			{
				TextureUnitState* tu = pass->createTextureUnitState();
				tu->setContentType(TextureUnitState::CONTENT_SHADOW);
				tu->setTextureAddressingMode(TextureUnitState::TAM_BORDER);
				tu->setTextureBorderColour(ColourValue::White);
			}
		}*/

	}
	bool TerrainMaterialGeneratorOgreMaterial::Profile::isShadowingEnabled(Profile::TechniqueType tt, const Ogre::Terrain * terrain) const
	{
		return getReceiveDynamicShadowsEnabled() && tt != RENDER_COMPOSITE_MAP &&
			(tt != LOW_LOD || mLowLodShadows);
	}
	void TerrainMaterialGeneratorOgreMaterial::Profile::setReceiveDynamicShadowsPSSM(Ogre::PSSMShadowCameraSetup * pssmSettings)
	{
		if (pssmSettings != mPSSM)
		{
			mPSSM = pssmSettings;
			mParent->_markChanged();
		}
	}
	void TerrainMaterialGeneratorOgreMaterial::Profile::setGlobalColourMapEnabled(bool enabled)
	{
		if (enabled != mGlobalColourMapEnabled)
		{
			mGlobalColourMapEnabled = enabled;
			mParent->_markChanged();
		}
	}
	Ogre::MaterialPtr TerrainMaterialGeneratorOgreMaterial::Profile::generateForCompositeMap(const Ogre::Terrain* ogreTerrain)
	{
		const Ogre::String matName = ogreTerrain->getMaterialName() + "/comp";
		mMatNameComp = matName;
		QSF_LOG_PRINTS(INFO,"sb called create for composite")
		//createMaterial(matName, ogreTerrain);

		return Ogre::MaterialManager::getSingleton().getByName(matName);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::setLightmapEnabled(bool enabled)
	{
		// TODO(co) This method is new in OGRE 1.9. Check whether or not we have to do something important in here.
	}

	Ogre::uint8 TerrainMaterialGeneratorOgreMaterial::Profile::getMaxLayers(const Ogre::Terrain* ogreTerrain) const
	{
		// Count the texture units free
		Ogre::uint8 freeTextureUnits = OGRE_MAX_TEXTURE_LAYERS;
		--freeTextureUnits;	// Color map
		--freeTextureUnits;	// Normal map

		// Each layer needs 2.25 units(1x_crgb_ha, 1x_nag_sr_gb, 0.25xblend)
		return static_cast<Ogre::uint8>(freeTextureUnits / 2.25f);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::updateParams(const Ogre::MaterialPtr& ogreMaterial, const Ogre::Terrain* ogreTerrain)
	{
		// Nothing to do in here
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::updateParamsForCompositeMap(const Ogre::MaterialPtr& ogreMaterial, const Ogre::Terrain* ogreTerrain)
	{
		// Nothing to do in here
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::requestOptions(Ogre::Terrain* ogreTerrain)
	{
		ogreTerrain->_setMorphRequired(false);	// We don't need terrain vertex morphing, the visual enhancement nearly not visible, but the performance impact can be seen when using profilers
		ogreTerrain->_setLightMapRequired(false, false);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::SetColorMap(uint64 ColorMap)
	{
		m_profil_ColorMap = ColorMap;
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::CreateEditableColorMap(const Ogre::Terrain * ogreTerrain)
	{
		qsf::GlobalAssetId ColorMapAsset;
		if (m_profil_ColorMap == qsf::getUninitialized<uint64>())
		{
			ColorMapAsset = qsf::AssetProxy("qsf/texture/default/missing").getGlobalAssetId();
		}
		else
		{
			ColorMapAsset = m_profil_ColorMap;
		}

		//Ogre::ResourceGroupManager::getSingleton().addResourceLocation(pathcopy.c_str(), Heightmaps);
		auto Filepath = qsf::AssetProxy(ColorMapAsset).getLocalAssetName();
		Ogre::Image OI;
		OI.load(Ogre::String(Filepath).c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		uint32 i_height = OI.getHeight();
		uint32 i_width = OI.getWidth();
		//copy image into buffer :)
		/*QSF_LOG_PRINTS(INFO,"get color at")
			try
		{
			//this works
			auto val = OI.getColourAt(0, 0, 0);
			QSF_LOG_PRINTS(INFO, val.r << " "<< val.g  <<" " << val.b)
		}
		catch (const std::exception&)
		{

		}*/
		
		

	}


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	TerrainMaterialGeneratorOgreMaterial::TerrainMaterialGeneratorOgreMaterial(uint64 ColorMap)
	{
		// define the layers
		// We expect terrain textures to have no alpha, so we use the alpha channel
		// in the albedo texture to store specular reflection
		// similarly we double-up the normal and height (for parallax)
		mLayerDecl.samplers.push_back(Ogre::TerrainLayerSampler("albedo_specular", Ogre::PF_BYTE_RGBA));
		mLayerDecl.samplers.push_back(Ogre::TerrainLayerSampler("normal_height", Ogre::PF_BYTE_RGBA));

		/*mLayerDecl.elements.push_back(
			TerrainLayerSamplerElement(0, TLSS_ALBEDO, 0, 3));
		mLayerDecl.elements.push_back(
			TerrainLayerSamplerElement(0, TLSS_SPECULAR, 3, 1));
		mLayerDecl.elements.push_back(
			TerrainLayerSamplerElement(1, TLSS_NORMAL, 0, 3));
		mLayerDecl.elements.push_back(
			TerrainLayerSamplerElement(1, TLSS_HEIGHT, 3, 1));*/


		/*mLayerDecl.samplers.emplace_back("_crgb_ha", Ogre::PF_BYTE_RGBA);
		mLayerDecl.samplers.emplace_back("_nag_sr_gb", Ogre::PF_BYTE_RGBA);*/

		mLayerDecl.elements.emplace_back(0, Ogre::TLSS_ALBEDO, 0, 3);
		mLayerDecl.elements.emplace_back(0, Ogre::TLSS_HEIGHT, 3, 1);
		mLayerDecl.elements.emplace_back(1, Ogre::TLSS_NORMAL, 0, 2);
		mLayerDecl.elements.emplace_back(1, Ogre::TLSS_SPECULAR, 2, 1);
		mLayerDecl.elements.emplace_back(1, Ogre::TLSS_SPECULAR, 3, 1);

			mProfiles.push_back(OGRE_NEW Profile(this, "SM2", "Profile for rendering on Shader Model 2 capable cards", ColorMap));
		setActiveProfile("SM2");
		mColorMap = ColorMap;
		// Connect our Boost slot to the Boost signal of the QSF asset system
		QSF_ASSET.AssetsMounted.connect(boost::bind(&TerrainMaterialGeneratorOgreMaterial::onAssetsMounted, this, _1));
	}

	TerrainMaterialGeneratorOgreMaterial::~TerrainMaterialGeneratorOgreMaterial()
	{
		// Disconnect our Boost slot from the Boost signal of the QSF asset system
		QSF_ASSET.AssetsMounted.disconnect(boost::bind(&TerrainMaterialGeneratorOgreMaterial::onAssetsMounted, this, _1));
	}

	void TerrainMaterialGeneratorOgreMaterial::RefreshMaterial(const Ogre::Terrain* ogreTerrain)
	{
			auto  OgreTerrain = kc_terrain::TerrainMaterialGeneratorOgreMaterial::generate(ogreTerrain);
			_markChanged();
			OgreTerrain->_dirtyState();
	}

	void TerrainMaterialGeneratorOgreMaterial::UpdateColorMap(const Ogre::Terrain * ogreTerrain)
	{
		
		qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
		//if(materialManager.findElement(qsf::StringHash(matName)))

		// In case the terrain material instance is already there, just update it
		qsf::Material* terrainMaterial = materialManager.findElement(qsf::StringHash(ogreTerrain->getMaterialName()));
		QSF_LOG_PRINTS(INFO,terrainMaterial->getMaterialParent());
		auto  OgreTerrain = kc_terrain::TerrainMaterialGeneratorOgreMaterial::generate(ogreTerrain);
		_markChanged();
		return;

	}

	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void TerrainMaterialGeneratorOgreMaterial::Profile::createMaterial(const Ogre::String& matName, const Ogre::Terrain* ogreTerrain)
	{
		//CreateOgreMaterial(ogreTerrain);
		//return;
		qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
		//if(materialManager.findElement(qsf::StringHash(matName)))
		QSF_ASSERT(matName == ogreTerrain->getMaterialName(), "qsf::TerrainMaterialGeneratorOgreMaterial::Profile::createMaterial(): OGRE terrain material name mismatch", QSF_REACT_NONE);

		// In case the terrain material instance is already there, just update it
		qsf::Material* terrainMaterial = materialManager.findElement(qsf::StringHash(matName));
		if (nullptr == terrainMaterial)
		{

			// Create terrain material instance
			terrainMaterial = materialManager.createMaterial(matName, qsf::StringHash("qsf_terrain"));
			QSF_CHECK(nullptr != terrainMaterial, "QSF failed to create QSF terrain material " << matName << " instance", return);
		}

		// Apply terrain quality setting
		qsf::compositing::CompositingSettingsGroup::getInstanceSafe().applyTerrainQualityToMaterial(*terrainMaterial);

		// Asset relevant information
		// TODO(co) Don't use QSF_MAIMAP in here. Support multiple terrains per map.
		const qsf::Map& map = QSF_MAINMAP;
		const kc_terrain::TerrainComponent* terrainComponent = qsf::ComponentMapQuery(map).getFirstInstance<kc_terrain::TerrainComponent>();
		QSF_CHECK(nullptr != terrainComponent, "There are no terrain component instances inside the map", return);

		// Global color map
		if (ogreTerrain->getGlobalColourMapEnabled() && !ogreTerrain->getGlobalColourMap().isNull())
		{
			// Global color map transform
			::detail::setIdentityMapTransformMaterialProperty(*terrainMaterial, "GlobalColorMapTransform");

			// Set color map
			terrainMaterial->setPropertyById("UseGlobalColorMap", qsf::MaterialPropertyValue::fromBoolean(true));
			terrainMaterial->setPropertyById("GlobalColorMap", qsf::MaterialPropertyValue::fromResourceName(ogreTerrain->getGlobalColourMap()->getName()));
		}
		else
		{
			// Global color map transform
			::detail::setMapTransformMaterialProperty(*terrainMaterial, *terrainComponent, *ogreTerrain, "GlobalColorMapTransform", true);
			qsf::GlobalAssetId ColorMapAsset;
			if (m_profil_ColorMap == qsf::getUninitialized<uint64>())
			{
				ColorMapAsset = qsf::AssetProxy("qsf/texture/default/missing").getGlobalAssetId();
			}
			else
			{
				ColorMapAsset = m_profil_ColorMap;
			}
			// A global color map which spans all terrain chunks, usually only used during runtime for efficiency
			//static const qsf::GlobalAssetId missingTextureGlobalAssetId = qsf::AssetProxy("qsf/texture/default/missing").getGlobalAssetId();
			/*qsf::GlobalAssetId globalAssetId = (nullptr != terrainDefinition && terrainDefinition->isValid()) ? terrainDefinition->getColorMap() : qsf::getUninitialized<qsf::GlobalAssetId>();
			if (qsf::isUninitialized(globalAssetId) || nullptr == QSF_ASSET.getAssetByGlobalAssetId(globalAssetId))
			{
				globalAssetId = ColorMapAsset;
			}*/
			terrainMaterial->setPropertyById("UseGlobalColorMap", qsf::MaterialPropertyValue::fromBoolean(qsf::isInitialized(ColorMapAsset)),qsf::MaterialProperty::Usage::DYNAMIC);
			terrainMaterial->setPropertyById("GlobalColorMap", qsf::MaterialPropertyValue::fromGlobalAssetId(ColorMapAsset));
		}

		// Global normal map
		if (ogreTerrain->getTerrainNormalMap().isNull())
		{
			// Global normal map transform
			::detail::setMapTransformMaterialProperty(*terrainMaterial, *terrainComponent, *ogreTerrain, "GlobalNormalMapTransform", true);

			// A global normal map which spans all terrain chunks, usually only used during runtime for efficiency
			/*const kc_terrain::TerrainDefinition* terrainDefinition = terrainComponent->getTerrainDefinition();
			if (nullptr != terrainDefinition && terrainDefinition->isValid())
			{
				// Just use the global asset ID and let our resource management do the rest...
				qsf::GlobalAssetId globalAssetId = terrainDefinition->getNormalMap();
				if (nullptr == QSF_ASSET.getAssetByGlobalAssetId(globalAssetId))
				{
					qsf::setUninitialized(globalAssetId);
				}
				terrainMaterial->setPropertyById("UseGlobalNormalMap", qsf::MaterialPropertyValue::fromBoolean(qsf::isInitialized(globalAssetId)));
				terrainMaterial->setPropertyById("GlobalNormalMap", qsf::MaterialPropertyValue::fromGlobalAssetId(globalAssetId));
			}
			else*/
			{
				terrainMaterial->setPropertyById("UseGlobalNormalMap", qsf::MaterialPropertyValue::fromBoolean(false));
				terrainMaterial->setPropertyById("GlobalNormalMap", qsf::MaterialPropertyValue::fromGlobalAssetId(qsf::getUninitialized<qsf::GlobalAssetId>()));
			}
		}
		else
		{
			// Global normal map transform
			::detail::setIdentityMapTransformMaterialProperty(*terrainMaterial, "GlobalNormalMapTransform");

			// Set normal map
			terrainMaterial->setPropertyById("UseGlobalNormalMap", qsf::MaterialPropertyValue::fromBoolean(true));
			terrainMaterial->setPropertyById("GlobalNormalMap", qsf::MaterialPropertyValue::fromResourceName(ogreTerrain->getTerrainNormalMap()->getName()));
		}

		// Layers: UV layer offset and terrain size
		::detail::setMapTransformMaterialProperty(*terrainMaterial, *terrainComponent, *ogreTerrain, "UvLayerTransform", false);
		terrainMaterial->setPropertyById("TerrainWorldSize", qsf::MaterialPropertyValue::fromFloat(ogreTerrain->getWorldSize()));

		// Number of layers and number of blend maps
		const uint32 maximumNumberOfLayers = getMaxLayers(ogreTerrain);
		uint32 numberOfBlendMaps = std::min(ogreTerrain->getBlendTextureCount(maximumNumberOfLayers), ogreTerrain->getBlendTextureCount());
		uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(ogreTerrain->getLayerCount()));
		//uint32 numberOfLayers = 3;
		terrainMaterial->setPropertyById("NumberOfLayers", qsf::MaterialPropertyValue::fromInteger(numberOfLayers));
		terrainMaterial->setPropertyById("NumberOfBlendMaps", qsf::MaterialPropertyValue::fromInteger(numberOfBlendMaps));

		// Blend maps
		//numberOfBlendMaps =6;
		std::vector<std::string> Textures;
		Textures.push_back("em5/material/terrain_layer/terrain_nature_grass01");
		Textures.push_back("em5/material/terrain_layer/terrain_urban_medieval_cobbles");
		Textures.push_back("em5/material/terrain_layer/terrain_nature_dirt01_fine");
		Textures.push_back("em5/material/terrain_layer/terrain_nature_sand01");
		Textures.push_back("em5/material/terrain_layer/terrain_urban_herringbone01");
		Textures.push_back("em5/material/terrain_layer/terrain_urban_herringbone01");
		Textures.push_back("em5/material/terrain_layer/terrain_urban_herringbone01");
		Textures.push_back("em5/material/terrain_layer/terrain_urban_herringbone01");
		Textures.push_back("em5/material/terrain_layer/terrain_urban_herringbone01");
		Textures.push_back("em5/material/terrain_layer/terrain_urban_herringbone01");
		//QSF_LOG_PRINTS(INFO,"number of layers " << numberOfLayers << "number of blendmaps" << numberOfBlendMaps<< " BlendTextureCount "<< ogreTerrain->getBlendTextureCount())
		for (uint32 i = 0; i < numberOfBlendMaps; ++i)
		{
				terrainMaterial->setPropertyById(qsf::StringHash("BlendMap" + std::to_string(i)), qsf::MaterialPropertyValue::fromResourceName(ogreTerrain->getBlendTextureName(i)));
				//terrainMaterial->setPropertyById(qsf::StringHash("BlendMap" + std::to_string(i)), qsf::MaterialPropertyValue::fromResourceName(Textures.at(i)));
		}
		// Texture layers
		for (uint32 layerIndex = 0; layerIndex < numberOfLayers; ++layerIndex)
		{
			
			const std::string layerIndexAsString = std::to_string(layerIndex);
			bool layerCreated = false;

			// Inside the first texture name of the terrain layer we store the global asset ID of the QSF material the terrain layer is using, we need nothing more
			//nasty hack
			//if(layerIndex == 0)
			std::string globalAssetIdAsString = Textures.at(layerIndex);
			//QSF_LOG_PRINTS(INFO, "material generator"<< layerIndex << " "  << globalAssetIdAsString)
			uint64 globalAssetId = qsf::getUninitialized<uint64>();
			//std::string globalAssetIdAsString = ogreTerrain->getLayerTextureName(layerIndex, 0);
			if(qsf::AssetProxy(globalAssetIdAsString).getAsset() != nullptr)
				globalAssetId = qsf::AssetProxy(globalAssetIdAsString).getGlobalAssetId();
			static const qsf::AssetSystem& assetSystem = QSF_ASSET;

			uint64 globalAssetId__ = qsf::getUninitialized<uint64>();
			if (nullptr != qsf::AssetProxy(globalAssetIdAsString).getAsset())
				globalAssetId__ = qsf::AssetProxy(globalAssetIdAsString).getGlobalAssetId();

				if (nullptr != qsf::AssetProxy(globalAssetIdAsString).getAsset())//assetSystem.getAssetByGlobalAssetId(globalAssetId))
				{
					const qsf::Material* layerMaterial = QSF_MATERIAL.getMaterialManager().findElement(qsf::StringHash(boost::lexical_cast<std::string>(globalAssetId)));

					if (nullptr != layerMaterial)
					{
						// Diffuse height
							const qsf::MaterialPropertyValue* layerMaterialPropertyValue = layerMaterial->getPropertyById("_crgb_ha");
						if (nullptr != layerMaterialPropertyValue)
						{
							terrainMaterial->setPropertyById(qsf::StringHash("_crgb_ha_" + layerIndexAsString), *layerMaterialPropertyValue);
						}
						else
						{
							QSF_ERROR("Broken material without _crgb_ha texture", QSF_REACT_NONE);
						}
							// Normal specular gloss
							layerMaterialPropertyValue = layerMaterial->getPropertyById("_nag_sr_gb");
						if (nullptr != layerMaterialPropertyValue)
						{
							terrainMaterial->setPropertyById(qsf::StringHash("_nag_sr_gb_" + layerIndexAsString), *layerMaterialPropertyValue);
						}
						else
						{
							QSF_ERROR("Broken material without _nag_sr_gb texture", QSF_REACT_NONE);
						}
						{ // World size
							float worldSize = 1.0f;
							layerMaterialPropertyValue = layerMaterial->getPropertyById("WorldSize");
							if (nullptr != layerMaterialPropertyValue)
							{
								worldSize = layerMaterialPropertyValue->getFloatValue();
							}
							// TODO(co) We might want to optimize this by e.g. directly setting the shader parameter instead of going over the OGRE terrain instance
							// TODO(tl) We should check how we can calculate UVMultiplier ourself from world size when we have time
							const_cast<Ogre::Terrain*>(ogreTerrain)->setLayerWorldSize(layerIndex, worldSize);
						}
							// Set terrain layer material properties
							::detail::setLayerBlendMapComponentUvMultiplier(*ogreTerrain, layerIndex, layerIndexAsString, *terrainMaterial);
							::detail::setTerrainLayerMaterialProperties(layerMaterial->getMaterialProperties(), layerIndexAsString, *terrainMaterial);
							// Layer has been created successfully
							layerCreated = true;
					}
					else
					{
				
						QSF_ERROR("Terrain layer material " << globalAssetIdAsString << " not found, restart editor, ignore this error and use terrain tool to reevaluate all layer (1)" << globalAssetIdAsString, QSF_REACT_NONE);
					}
				}
				else
				{
					QSF_ERROR("Terrain layer asset " << globalAssetId << " not found, restart editor, ignore this error and use terrain tool to reevaluate all layer (2)" << globalAssetIdAsString, QSF_REACT_NONE);
				}

			// Error handling: The show must go on
			if (!layerCreated)
			{
				// If one layer is defective, we can't use the following layers or we end up in a material-shader chaos
				numberOfLayers = layerIndex;
				terrainMaterial->setPropertyById("NumberOfLayers", qsf::MaterialPropertyValue::fromInteger(numberOfLayers));
				layerIndex = static_cast<int>(numberOfLayers);
			}
		}
	}






	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void TerrainMaterialGeneratorOgreMaterial::onAssetsMounted(const qsf::Assets& assets)
	{
		static qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
		const qsf::AssetSystem& assetSystem = QSF_ASSET;
		const qsf::ComponentMapQuery componentMapQuery(QSF_MAINMAP);
		for (const qsf::Asset* asset : assets)
		{
			// We're only interested in material assets
			if (asset->getTypeId() == qsf::QsfAssetTypes::MATERIAL && asset->getCategory() == "terrain_layer")
			{
				// TODO(co) This solution is surely not the most elegant and efficient. Works for now. If you can improve it, please do so.
				for (TerrainComponent* terrainComponent : componentMapQuery.getAllInstances<TerrainComponent>())
				{
					Ogre::TerrainGroup* ogreTerrainGroup = terrainComponent->getOgreTerrainGroup();
					if (nullptr != ogreTerrainGroup)
					{
						Ogre::TerrainGroup::TerrainIterator ogreTerrainIterator = ogreTerrainGroup->getTerrainIterator();
						while (ogreTerrainIterator.hasMoreElements())
						{
							const Ogre::Terrain* ogreTerrain = ogreTerrainIterator.getNext()->instance;
							if (nullptr != ogreTerrain)
							{
								qsf::Material* terrainMaterial = nullptr;
								const uint32 maximumNumberOfLayers = getMaxLayers(ogreTerrain);
								const uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(ogreTerrain->getLayerCount()));
								for (uint32 layerIndex = 0; layerIndex < numberOfLayers; ++layerIndex)
								{
									const std::string layerIndexAsString = std::to_string(layerIndex);

									// Inside the first texture name of the terrain layer we store the global asset ID of the QSF material the terrain layer is using, we need nothing more
									const std::string globalAssetIdAsString = ogreTerrain->getLayerTextureName(layerIndex, 0);
									const qsf::GlobalAssetId globalAssetId = assetSystem.globalAssetIdAsStringToGlobalAssetId(globalAssetIdAsString);
									if (nullptr != assetSystem.getAssetByGlobalAssetId(globalAssetId))
									{
										// Update terrain material
										const qsf::Material* layerMaterial = materialManager.findElement(qsf::StringHash(globalAssetIdAsString));
										if (nullptr != layerMaterial)
										{
											// TODO(co) We might want to optimize this by e.g. directly setting the shader parameter instead of going over the OGRE terrain instance
											{ // Terrain world size
												const qsf::MaterialPropertyValue* layerMaterialPropertyValue = layerMaterial->getPropertyById("WorldSize");
												if (nullptr != layerMaterialPropertyValue)
												{
													// TODO(co) Const cast is no longer required as soon as the TODO from above is done
													const_cast<Ogre::Terrain*>(ogreTerrain)->setLayerWorldSize(layerIndex, layerMaterialPropertyValue->getFloatValue());
												}
											}

											// Set terrain layer material properties
											if (nullptr == terrainMaterial)
											{
												terrainMaterial = materialManager.findElement(qsf::StringHash(ogreTerrain->getMaterialName()));
											}
											if (nullptr != terrainMaterial)
											{
												::detail::setLayerBlendMapComponentUvMultiplier(*ogreTerrain, layerIndex, layerIndexAsString, *terrainMaterial);
												::detail::setTerrainLayerMaterialProperties(layerMaterial->getMaterialProperties(), layerIndexAsString, *terrainMaterial);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	//Shader stuff from Ogre TerrainMaterialGeneratorOgreMaterial 2.1

	//---------------------------------------------------------------------
	//---------------------------------------------------------------------
	Ogre::HighLevelGpuProgramPtr TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateVertexProgram(
			const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt)
	{
		Ogre::HighLevelGpuProgramPtr ret = createVertexProgram(prof, terrain, tt);

		Ogre::StringStream sourceStr;
		generateVertexProgramSource(prof, terrain, tt, sourceStr);

		ret->setSource(sourceStr.str());
		ret->load();
		defaultVpParams(prof, terrain, tt, ret);

#if OGRE_DEBUG_MODE
		LogManager::getSingleton().stream(LML_TRIVIAL) << "*** Terrain Vertex Program: "
			<< ret->getName() << " ***\n" << ret->getSource() << "\n***   ***";
#endif
		return ret;

	}
	//---------------------------------------------------------------------
	Ogre::HighLevelGpuProgramPtr
		TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateFragmentProgram(
			const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt)
	{
		Ogre::HighLevelGpuProgramPtr ret = createFragmentProgram(prof, terrain, tt);

		Ogre::StringStream sourceStr;
		generateFragmentProgramSource(prof, terrain, tt, sourceStr);
		ret->setSource(sourceStr.str());
		ret->load();
		defaultFpParams(prof, terrain, tt, ret);

#if OGRE_DEBUG_MODE
		LogManager::getSingleton().stream(LML_TRIVIAL) << "*** Terrain Fragment Program: "
			<< ret->getName() << " ***\n" << ret->getSource() << "\n*** ***";
#endif
		return ret;
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateVertexProgramSource(
		const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt, Ogre::StringStream& outStream)
	{
		generateVpHeader(prof, terrain, tt, outStream);

		if (tt != LOW_LOD)
		{
		
			Ogre::uint maxLayers = prof->getMaxLayers(terrain);
			Ogre::uint numLayers = std::min(maxLayers, static_cast<Ogre::uint>(terrain->getLayerCount()));

			for (Ogre::uint i = 0; i < numLayers; ++i)
				generateVpLayer(prof, terrain, tt, i, outStream);
		}

		generateVpFooter(prof, terrain, tt, outStream);

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateFragmentProgramSource(
		const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt, Ogre::StringStream& outStream)
	{
		generateFpHeader(prof, terrain, tt, outStream);

		if (tt != LOW_LOD)
		{
			Ogre::uint maxLayers = prof->getMaxLayers(terrain);
			Ogre::uint numLayers = std::min(maxLayers, static_cast<Ogre::uint>(terrain->getLayerCount()));

			for (Ogre::uint i = 0; i < numLayers; ++i)
				generateFpLayer(prof, terrain, tt, i, outStream);
		}

		generateFpFooter(prof, terrain, tt, outStream);
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::defaultVpParams(
		const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt, const Ogre::HighLevelGpuProgramPtr& prog)
	{
		Ogre::GpuProgramParametersSharedPtr params = prog->getDefaultParameters();
		params->setIgnoreMissingParams(true);
		params->setNamedAutoConstant("worldMatrix", Ogre::GpuProgramParameters::ACT_WORLD_MATRIX);
		params->setNamedAutoConstant("viewProjMatrix", Ogre::GpuProgramParameters::ACT_VIEWPROJ_MATRIX);
		//params->setNamedAutoConstant("lodMorph", Ogre::GpuProgramParameters::ACT_CUSTOM,Terrain::LOD_MORPH_CUSTOM_PARAM);
		params->setNamedAutoConstant("fogParams", Ogre::GpuProgramParameters::ACT_FOG_PARAMS);
		if (prof->isShadowingEnabled(tt, terrain))
		{
			Ogre::uint numTextures = 1;
			if (prof->getReceiveDynamicShadowsPSSM())
			{
				numTextures = (Ogre::uint)prof->getReceiveDynamicShadowsPSSM()->getSplitCount();
			}
			for (Ogre::uint i = 0; i < numTextures; ++i)
			{
				params->setNamedAutoConstant("texViewProjMatrix" + Ogre::StringConverter::toString(i),
					Ogre::GpuProgramParameters::ACT_TEXTURE_VIEWPROJ_MATRIX, i);
				if (prof->getReceiveDynamicShadowsDepth())
				{
					params->setNamedAutoConstant("depthRange" + Ogre::StringConverter::toString(i),
						Ogre::GpuProgramParameters::ACT_SHADOW_SCENE_DEPTH_RANGE, i);
				}
			}
		}

		if (terrain->_getUseVertexCompression() && tt != RENDER_COMPOSITE_MAP)
		{
			Ogre::Matrix4 posIndexToObjectSpace;
			terrain->getPointTransform(&posIndexToObjectSpace);
			params->setNamedConstant("posIndexToObjectSpace", posIndexToObjectSpace);
		}



	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::defaultFpParams(
		const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt, const Ogre::HighLevelGpuProgramPtr& prog)
	{
		Ogre::GpuProgramParametersSharedPtr params = prog->getDefaultParameters();
		params->setIgnoreMissingParams(true);

		params->setNamedAutoConstant("ambient", Ogre::GpuProgramParameters::ACT_AMBIENT_LIGHT_COLOUR);
		params->setNamedAutoConstant("lightPosObjSpace", Ogre::GpuProgramParameters::ACT_LIGHT_POSITION_OBJECT_SPACE, 0);
		params->setNamedAutoConstant("lightDiffuseColour", Ogre::GpuProgramParameters::ACT_LIGHT_DIFFUSE_COLOUR, 0);
		params->setNamedAutoConstant("lightSpecularColour", Ogre::GpuProgramParameters::ACT_LIGHT_SPECULAR_COLOUR, 0);
		params->setNamedAutoConstant("eyePosObjSpace", Ogre::GpuProgramParameters::ACT_CAMERA_POSITION_OBJECT_SPACE);
		params->setNamedAutoConstant("fogColour", Ogre::GpuProgramParameters::ACT_FOG_COLOUR);

		if (prof->isShadowingEnabled(tt, terrain))
		{
			Ogre::uint numTextures = 1;
			if (prof->getReceiveDynamicShadowsPSSM())
			{
				Ogre::PSSMShadowCameraSetup* pssm = prof->getReceiveDynamicShadowsPSSM();
				numTextures = (Ogre::uint)pssm->getSplitCount();
				Ogre::Vector4 splitPoints;
				const Ogre::PSSMShadowCameraSetup::SplitPointList& splitPointList = pssm->getSplitPoints();
				// Populate from split point 1, not 0, since split 0 isn't useful (usually 0)
				for (Ogre::uint i = 1; i < numTextures; ++i)
				{
					splitPoints[i - 1] = splitPointList[i];
				}
				params->setNamedConstant("pssmSplitPoints", splitPoints);
			}

			if (prof->getReceiveDynamicShadowsDepth())
			{
				size_t samplerOffset = (tt == HIGH_LOD) ? mShadowSamplerStartHi : mShadowSamplerStartLo;
				for (Ogre::uint i = 0; i < numTextures; ++i)
				{
					params->setNamedAutoConstant("inverseShadowmapSize" + Ogre::StringConverter::toString(i),
						Ogre::GpuProgramParameters::ACT_INVERSE_TEXTURE_SIZE, i + samplerOffset);
				}
			}
		}

		// Explicitly bind samplers for GLSL
		if ((prof->_getShaderLanguage() == "glsl") || (prof->_getShaderLanguage() == "glsles"))
		{
			int numSamplers = 0;
			if (tt == LOW_LOD)
			{
				params->setNamedConstant("compositeMap", (int)numSamplers++);
			}
			else
			{
				params->setNamedConstant("globalNormal", (int)numSamplers++);

				if (terrain->getGlobalColourMapEnabled() && prof->isGlobalColourMapEnabled())
				{
					params->setNamedConstant("globalColourMap", (int)numSamplers++);
				}
				if (prof->isLightmapEnabled())
				{
					params->setNamedConstant("lightMap", (int)numSamplers++);
				}

				Ogre::uint maxLayers = prof->getMaxLayers(terrain);
				Ogre::uint numBlendTextures = std::min(terrain->getBlendTextureCount(maxLayers), terrain->getBlendTextureCount());
				Ogre::uint numLayers = std::min(maxLayers, static_cast<Ogre::uint>(terrain->getLayerCount()));
				// Blend textures - sampler definitions
				for (Ogre::uint i = 0; i < numBlendTextures; ++i)
				{
					params->setNamedConstant("blendTex" + Ogre::StringConverter::toString(i), (int)numSamplers++);
				}

				// Layer textures - sampler definitions & UV multipliers
				for (Ogre::uint i = 0; i < numLayers; ++i)
				{
					params->setNamedConstant("difftex" + Ogre::StringConverter::toString(i), (int)numSamplers++);
					params->setNamedConstant("normtex" + Ogre::StringConverter::toString(i), (int)numSamplers++);
				}

				Ogre::uint numShadowTextures = 1;
				if (prof->getReceiveDynamicShadowsPSSM())
					numShadowTextures = (Ogre::uint)prof->getReceiveDynamicShadowsPSSM()->getSplitCount();

				for (Ogre::uint i = 0; i < numShadowTextures; ++i)
				{
					if (prof->isShadowingEnabled(tt, terrain))
						params->setNamedConstant("shadowMap" + Ogre::StringConverter::toString(i), (int)numSamplers++);
				}
			}
		}
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::updateParams(
		const Profile* prof, const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain, bool compositeMap)
	{
		Ogre::Pass* p = mat->getTechnique(0)->getPass(0);
		if (compositeMap)
		{
			updateVpParams(prof, terrain, RENDER_COMPOSITE_MAP, p->getVertexProgramParameters());
			updateFpParams(prof, terrain, RENDER_COMPOSITE_MAP, p->getFragmentProgramParameters());
		}
		else
		{
			// high lod
			updateVpParams(prof, terrain, HIGH_LOD, p->getVertexProgramParameters());
			updateFpParams(prof, terrain, HIGH_LOD, p->getFragmentProgramParameters());

			if (prof->isCompositeMapEnabled())
			{
				// low lod
				p = mat->getTechnique(1)->getPass(0);
				updateVpParams(prof, terrain, LOW_LOD, p->getVertexProgramParameters());
				updateFpParams(prof, terrain, LOW_LOD, p->getFragmentProgramParameters());
			}
		}
	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::updateVpParams(
		const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt, const Ogre::GpuProgramParametersSharedPtr& params)
	{
		params->setIgnoreMissingParams(true);
		Ogre::uint maxLayers = prof->getMaxLayers(terrain);
		Ogre::uint numLayers = std::min(maxLayers, static_cast<Ogre::uint>(terrain->getLayerCount()));
		Ogre::uint numUVMul = numLayers / 4;
		if (numLayers % 4)
			++numUVMul;
		for (Ogre::uint i = 0; i < numUVMul; ++i)
		{
			Ogre::Vector4 uvMul(
				terrain->getLayerUVMultiplier(i * 4),
				terrain->getLayerUVMultiplier(i * 4 + 1),
				terrain->getLayerUVMultiplier(i * 4 + 2),
				terrain->getLayerUVMultiplier(i * 4 + 3)
			);
			params->setNamedConstant("uvMul_" + Ogre::StringConverter::toString(i), uvMul);
		}

		if (terrain->_getUseVertexCompression() && tt != RENDER_COMPOSITE_MAP)
		{
			Ogre::Real baseUVScale = 1.0f / (terrain->getSize() - 1);
			params->setNamedConstant("baseUVScale", baseUVScale);
		}

	}
	//---------------------------------------------------------------------
	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::updateFpParams(
		const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt, const Ogre::GpuProgramParametersSharedPtr& params)
	{
		params->setIgnoreMissingParams(true);
		// TODO - parameterise this?
		Ogre::Vector4 scaleBiasSpecular((Ogre::Real)0.03, (Ogre::Real)-0.04, (Ogre::Real)32, (Ogre::Real)1);
		params->setNamedConstant("scaleBiasSpecular", scaleBiasSpecular);

	}
	//---------------------------------------------------------------------
	Ogre::String TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::getChannel(Ogre::uint idx)
	{
		Ogre::uint rem = idx % 4;
		switch (rem)
		{
		case 0:
		default:
			return "r";
		case 1:
			return "g";
		case 2:
			return "b";
		case 3:
			return "a";
		};
	}
	//---------------------------------------------------------------------
	Ogre::String TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::getVertexProgramName(
		const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt)
	{
		Ogre::String progName = terrain->getMaterialName() + "/sm2/vp";

		switch (tt)
		{
		case HIGH_LOD:
			progName += "/hlod";
			break;
		case LOW_LOD:
			progName += "/llod";
			break;
		case RENDER_COMPOSITE_MAP:
			progName += "/comp";
			break;
		}

		return progName;

	}
	//---------------------------------------------------------------------
	Ogre::String TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::getFragmentProgramName(
		const Profile* prof, const Ogre::Terrain* terrain, TechniqueType tt)
	{

		Ogre::String progName = terrain->getMaterialName() + "/sm2/fp";

		switch (tt)
		{
		case HIGH_LOD:
			progName += "/hlod";
			break;
		case LOW_LOD:
			progName += "/llod";
			break;
		case RENDER_COMPOSITE_MAP:
			progName += "/comp";
			break;
		}

		return progName;
	}

	Ogre::HighLevelGpuProgramPtr TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::createVertexProgram(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt)
	{
		return Ogre::HighLevelGpuProgramPtr();
		 //return TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::createVertexProgram(prof,terrain,tt);
	}

	Ogre::HighLevelGpuProgramPtr TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::createFragmentProgram(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt)
	{
		return Ogre::HighLevelGpuProgramPtr();
		
		//return TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::createFragmentProgram(prof,terrain,tt);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateVpHeader(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::StringStream & outStream)
	{
		//TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateVpHeader(prof,terrain,tt,outStream);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateFpHeader(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::StringStream & outStream)
	{
		//TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateFpHeader(prof, terrain, tt, outStream);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateVpLayer(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream & outStream)
	{
		return;
		//TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateVpLayer(prof, terrain, tt,layer, outStream);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateFpLayer(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream & outStream)
	{
		//TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateVpLayer(prof, terrain, tt,layer, outStream);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateVpFooter(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::StringStream & outStream)
	{
		//TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateVpFooter(prof, terrain, tt, outStream);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateFpFooter(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::StringStream & outStream)
	{
		//TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateFpFooter(prof, terrain, tt, outStream);
	}

	Ogre::uint TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateVpDynamicShadowsParams(Ogre::uint texCoordStart, const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::StringStream & outStream)
	{
		return 0;
		//return TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelper::generateVpDynamicShadowsParams(texCoordStart, prof, terrain, tt, outStream);
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateVpDynamicShadows(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::StringStream & outStream)
	{
		return;
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateFpDynamicShadowsHelpers(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::StringStream & outStream)
	{
		return;
	}

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateFpDynamicShadowsParams(Ogre::uint * texCoord, Ogre::uint * sampler, const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::StringStream & outStream)
	{
		return;
	}
		

	void TerrainMaterialGeneratorOgreMaterial::Profile::ShaderHelperCg::generateFpDynamicShadows(const Profile * prof, const Ogre::Terrain * terrain, Profile::TechniqueType tt, Ogre::StringStream & outStream)
	{
		return;
	}

	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
} // qsf
