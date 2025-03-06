// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool\kc_terrain\TerrainMaterialGenerator.h>
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
#include <qsf_editor/asset/AssetEditHelper.h>
#include <qsf/asset/project/AssetPackage.h>
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
			kc_terrain::TerrainComponent* TC = &const_cast<kc_terrain::TerrainComponent&>(terrainComponent);
			auto terrainChunksPerEdge = TC->kc_getTerrainChunksPerEdge();
			//(kc)no offset if entity is not in the middle
			auto TPos = terrainComponent.getEntity().getComponent<qsf::TransformComponent>()->getPosition();
			float xOffset = ogreTerrain.getPosition().x- TPos.x - ogreTerrain.getWorldSize() * 0.5f + globalTerrainWorldSize * 0.5f;
			float yOffset = ogreTerrain.getPosition().z- TPos.z - ogreTerrain.getWorldSize() * 0.5f + globalTerrainWorldSize * 0.5f;
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

		void setMapTransformMaterialPropertyNoOffset(qsf::Material& material, const kc_terrain::TerrainComponent& terrainComponent, const Ogre::Terrain& ogreTerrain, qsf::MaterialPropertyId materialPropertyId, bool scale)
		{
			// TODO(co) Proper texture coordinate offset generation
			const float globalTerrainWorldSize = terrainComponent.getTerrainWorldSize();
			kc_terrain::TerrainComponent* TC = &const_cast<kc_terrain::TerrainComponent&>(terrainComponent);
			auto terrainChunksPerEdge = TC->kc_getTerrainChunksPerEdge();
			if (scale)
			{
				material.setPropertyById(materialPropertyId, qsf::MaterialPropertyValue::fromFloat4(0, 0, 1.0f / terrainChunksPerEdge, 1.0f / terrainChunksPerEdge));
			}
			else
			{
				material.setPropertyById(materialPropertyId, qsf::MaterialPropertyValue::fromFloat4(0, 0, 1.0f, 1.0f));
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
	TerrainMaterialGenerator::Profile::Profile(Ogre::TerrainMaterialGenerator* parent, const Ogre::String& name, const Ogre::String& desc, uint64 ColorMap) :
		Ogre::TerrainMaterialGenerator::Profile(parent, name, desc)
	{
		m_profil_ColorMap = ColorMap;
		// Nothing to do in here
	}

	TerrainMaterialGenerator::Profile::~Profile()
	{
		qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
		materialManager.destroyElement(qsf::StringHash(mMatName));
		materialManager.destroyElement(qsf::StringHash(mMatNameComp));
	}


	//[-------------------------------------------------------]
	//[ Public virtual Ogre::TerrainMaterialGenerator::Profile methods ]
	//[-------------------------------------------------------]
	bool TerrainMaterialGenerator::Profile::isVertexCompressionSupported() const
	{
		return false;
	}

	Ogre::MaterialPtr TerrainMaterialGenerator::Profile::generate(const Ogre::Terrain* ogreTerrain)
	{
		//new
		//if (Ogre::MaterialManager::getSingleton().getByName(ogreTerrain->getMaterialName().c_str()).getPointer() != nullptr)
			//return;
		/*Ogre::MaterialPtr mMat = Ogre::MaterialManager::getSingleton().create(ogreTerrain->getMaterialName(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);
		Ogre::Technique* mTech = mMat->createTechnique();
		Ogre::Pass* mPass = mTech->createPass();
		Ogre::TextureUnitState* mTexUnitState = mPass->createTextureUnitState();
		qsf::GlobalAssetId ColorMapAsset;
		if (m_profil_ColorMap == qsf::getUninitialized<uint64>())
		{
			ColorMapAsset = qsf::AssetProxy("qsf/texture/default/missing").getGlobalAssetId();
		}
		else
		{
			ColorMapAsset = m_profil_ColorMap;
		}

		Ogre::TexturePtr mTex = Ogre::TextureManager::getSingleton().load(qsf::AssetProxy(ColorMapAsset).getLocalAssetName() + ".dds", "General");
		mPass->createTextureUnitState()->setTextureName(mTex->getName());
		return mMat;*/
		const Ogre::String& matName = ogreTerrain->getMaterialName();
		createMaterial(matName, ogreTerrain);
		//QSF_LOG_PRINTS(INFO,"Is a usefull pointer" << Ogre::MaterialManager::getSingleton().getByName(matName).get())

		//this is a nullptr
		return Ogre::MaterialManager::getSingleton().getByName(matName);
	}

	Ogre::MaterialPtr TerrainMaterialGenerator::Profile::generateForCompositeMap(const Ogre::Terrain* ogreTerrain)
	{
		const Ogre::String matName = ogreTerrain->getMaterialName() + "/comp";
		mMatNameComp = matName;

		createMaterial(matName, ogreTerrain);

		return Ogre::MaterialManager::getSingleton().getByName(matName);
	}

	void TerrainMaterialGenerator::Profile::setLightmapEnabled(bool enabled)
	{
		// TODO(co) This method is new in OGRE 1.9. Check whether or not we have to do something important in here.
	}

	Ogre::uint8 TerrainMaterialGenerator::Profile::getMaxLayers(const Ogre::Terrain* ogreTerrain) const
	{
		// Count the texture units free
		Ogre::uint8 freeTextureUnits = OGRE_MAX_TEXTURE_LAYERS;
		--freeTextureUnits;	// Color map
		--freeTextureUnits;	// Normal map

		// Each layer needs 2.25 units(1x_crgb_ha, 1x_nag_sr_gb, 0.25xblend)
		return static_cast<Ogre::uint8>(freeTextureUnits / 2.25f);
	}

	void TerrainMaterialGenerator::Profile::updateParams(const Ogre::MaterialPtr& ogreMaterial, const Ogre::Terrain* ogreTerrain)
	{
		// Nothing to do in here
	}

	void TerrainMaterialGenerator::Profile::updateParamsForCompositeMap(const Ogre::MaterialPtr& ogreMaterial, const Ogre::Terrain* ogreTerrain)
	{
		// Nothing to do in here
	}

	void TerrainMaterialGenerator::Profile::requestOptions(Ogre::Terrain* ogreTerrain)
	{
		ogreTerrain->_setMorphRequired(false);	// We don't need terrain vertex morphing, the visual enhancement nearly not visible, but the performance impact can be seen when using profilers
		ogreTerrain->_setLightMapRequired(false, false);
	}

	void TerrainMaterialGenerator::Profile::SetColorMap(uint64 ColorMap)
	{
		m_profil_ColorMap = ColorMap;
	}

	void TerrainMaterialGenerator::Profile::CreateEditableColorMap(const Ogre::Terrain * ogreTerrain)
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
	TerrainMaterialGenerator::TerrainMaterialGenerator(uint64 ColorMap)
	{
		mLayerDecl.samplers.emplace_back("_crgb_ha", Ogre::PF_BYTE_RGBA);
		mLayerDecl.samplers.emplace_back("_nag_sr_gb", Ogre::PF_BYTE_RGBA);

		mLayerDecl.elements.emplace_back(0, Ogre::TLSS_ALBEDO, 0, 3);
		mLayerDecl.elements.emplace_back(0, Ogre::TLSS_HEIGHT, 3, 1);
		mLayerDecl.elements.emplace_back(1, Ogre::TLSS_NORMAL, 0, 2);
		mLayerDecl.elements.emplace_back(1, Ogre::TLSS_SPECULAR, 2, 1);
		mLayerDecl.elements.emplace_back(1, Ogre::TLSS_SPECULAR, 3, 1);

			mProfiles.push_back(OGRE_NEW Profile(this, "SM2", "Profile for rendering on Shader Model 2 capable cards", ColorMap));
		setActiveProfile("SM2");
		mColorMap = ColorMap;
		// Connect our Boost slot to the Boost signal of the QSF asset system
		QSF_ASSET.AssetsMounted.connect(boost::bind(&TerrainMaterialGenerator::onAssetsMounted, this, _1));
	}

	TerrainMaterialGenerator::~TerrainMaterialGenerator()
	{
		// Disconnect our Boost slot from the Boost signal of the QSF asset system
		QSF_ASSET.AssetsMounted.disconnect(boost::bind(&TerrainMaterialGenerator::onAssetsMounted, this, _1));
	}

	void TerrainMaterialGenerator::RefreshMaterial(const Ogre::Terrain* ogreTerrain)
	{
			auto  OgreTerrain = kc_terrain::TerrainMaterialGenerator::generate(ogreTerrain);

	}

	void TerrainMaterialGenerator::UpdateColorMap(const Ogre::Terrain * ogreTerrain)
	{
		if (qsf::AssetProxy(mColorMap).getGlobalAssetId() != qsf::getUninitialized<uint64>())
		{
			auto mAssetEditHelper = std::shared_ptr<qsf::editor::AssetEditHelper>(new qsf::editor::AssetEditHelper());
			std::string TargetAssetName = qsf::AssetProxy(mColorMap).getAssetPackage()->getName();
			mAssetEditHelper->tryEditAsset(qsf::AssetProxy(mColorMap).getGlobalAssetId(),TargetAssetName);
			if(!mAssetEditHelper->setAssetUploadData(mColorMap, true, true))
			{ 
			//QSF_LOG_PRINTS(INFO,"couldnt update colormap?")
			}
			mAssetEditHelper->submit();
		}
		//auto  OgreTerrain = kc_terrain::TerrainMaterialGenerator::generate(ogreTerrain);
		//_markChanged();
		return;

	}

	void TerrainMaterialGenerator::UpdateSmallColorMap(const Ogre::Terrain * ogreTerrain, qsf::GlobalAssetId MaterialAssetId)
	{
		if (qsf::AssetProxy(MaterialAssetId).getGlobalAssetId() != qsf::getUninitialized<uint64>())
		{
			auto mAssetEditHelper = std::shared_ptr<qsf::editor::AssetEditHelper>(new qsf::editor::AssetEditHelper());
			std::string TargetAssetName = qsf::AssetProxy(MaterialAssetId).getAssetPackage()->getName();
			mAssetEditHelper->tryEditAsset(qsf::AssetProxy(MaterialAssetId).getGlobalAssetId(), TargetAssetName);
			if (!mAssetEditHelper->setAssetUploadData(MaterialAssetId, true, true))
			{
				//QSF_LOG_PRINTS(INFO,"couldnt update colormap?")
			}
			mAssetEditHelper->submit();
		}
		//we could improve speed if we dont have to save map
		/*auto Mat = ogreTerrain->getMaterial();
		auto iterator = Mat->getTechniqueIterator();
		while (iterator.hasMoreElements())
		{
			auto tech = iterator.getNext();
			auto passes = tech->getPassIterator();
			while (passes.hasMoreElements())
			{
				auto pass = passes.getNext();
				QSF_LOG_PRINTS(INFO,"Pass stuff" << pass->getName().c_str() <<" "<< pass->getColourWriteEnabled() << " "<<pass->getResourceGroup().c_str() << " ")
				//pass->getMacroblock()
			}
		}*/

	}

	void TerrainMaterialGenerator::Profile::CreateOgreMaterial(const Ogre::Terrain* Terrain)
	{
		return;
		if(Ogre::MaterialManager::getSingleton().getByName(Terrain->getMaterialName().c_str()).getPointer() != nullptr)
			return;
		Ogre::MaterialPtr mMat = Ogre::MaterialManager::getSingleton().create(Terrain->getMaterialName(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);
		Ogre::Technique* mTech = mMat->createTechnique();
		Ogre::Pass* mPass = mTech->createPass();
		Ogre::TextureUnitState* mTexUnitState = mPass->createTextureUnitState();
		qsf::GlobalAssetId ColorMapAsset;
		if (m_profil_ColorMap == qsf::getUninitialized<uint64>())
		{
			ColorMapAsset = qsf::AssetProxy("qsf/texture/default/missing").getGlobalAssetId();
		}
		else
		{
			ColorMapAsset = m_profil_ColorMap;
		}
		
		Ogre::TexturePtr mTex = Ogre::TextureManager::getSingleton().load(qsf::AssetProxy(ColorMapAsset).getLocalAssetName()+".dds", "General");
		mPass->createTextureUnitState()->setTextureName(mTex->getName());
		auto point = Terrain->getGlobalColourMap();
		//Terrain->mater

		uint8* buffer = new uint8[1024 * 1024 * 8];
		Ogre::Image ogreImage;
		ogreImage.loadDynamicImage(buffer, 1024, 1024, Ogre::PixelFormat::PF_FLOAT32_GR);
		for (size_t t = 0; t < 1024 - 1; t++)
		{
			for (size_t j = 0; j < 1024 - 1; j++)
			{
				const Ogre::ColourValue ogreColorValue = Ogre::ColourValue((float)t/1024.f, (float)j/1024.f, (float)t/1024.f+j/1024.f);
				ogreImage.setColourAt(ogreColorValue, t, j, 0);
			}
			//ogreImage.setColourAt()
		}
		QSF_LOG_PRINTS(INFO,"load image")
		Ogre::ConstImagePtrList IPL;
		IPL.push_back(static_cast<const Ogre::Image*>(&ogreImage));
		point->_loadImages(IPL);
	}

	void TerrainMaterialGenerator::ChangeColorMapToSmallerMap(Ogre::Terrain * Terrain,std::string LocalAssetName)
	{
		qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
		auto matName = Terrain->getMaterialName();
		qsf::Material* terrainMaterial = materialManager.findElement(qsf::StringHash(matName));

		
		
		//QSF_LOG_PRINTS(INFO,parts_per_line)
			const kc_terrain::TerrainComponent* terrainComponent = qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<kc_terrain::TerrainComponent>();
			::detail::setMapTransformMaterialPropertyNoOffset(*terrainMaterial, *terrainComponent, *Terrain, "GlobalColorMapTransform", false);


		auto AssetId = qsf::AssetProxy(LocalAssetName).getGlobalAssetId();
		if (AssetId == qsf::getUninitialized<uint64>())
		{
			AssetId = qsf::AssetProxy("qsf/texture/default/missing").getGlobalAssetId();
		}
		terrainMaterial->setPropertyById("GlobalColorMap", qsf::MaterialPropertyValue::fromGlobalAssetId(AssetId));
	}
	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void TerrainMaterialGenerator::Profile::createMaterial(const Ogre::String& matName, const Ogre::Terrain* ogreTerrain)
	{
		//CreateOgreMaterial(ogreTerrain);
		//return;
		qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
		//if(materialManager.findElement(qsf::StringHash(matName)))
		QSF_ASSERT(matName == ogreTerrain->getMaterialName(), "qsf::TerrainMaterialGenerator::Profile::createMaterial(): OGRE terrain material name mismatch", QSF_REACT_NONE);

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
			terrainMaterial->setPropertyById("UseGlobalColorMap", qsf::MaterialPropertyValue::fromBoolean(qsf::isInitialized(ColorMapAsset)),qsf::MaterialProperty::Usage::DYNAMIC);
			//terrainMaterial->setPropertyById("GlobalColorMap", qsf::MaterialPropertyValue::fromGlobalAssetId(qsf::getUninitialized<uint64>()), qsf::MaterialProperty::Usage::DYNAMIC);
			terrainMaterial->setPropertyById("GlobalColorMap", qsf::MaterialPropertyValue::fromGlobalAssetId(ColorMapAsset), qsf::MaterialProperty::Usage::DYNAMIC);
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

		for (uint32 i = 0; i < numberOfBlendMaps; ++i)
		{
				terrainMaterial->setPropertyById(qsf::StringHash("BlendMap" + std::to_string(i)), qsf::MaterialPropertyValue::fromResourceName(ogreTerrain->getBlendTextureName(i)));
		}
		// Texture layers
		for (uint32 layerIndex = 0; layerIndex < numberOfLayers; ++layerIndex)
		{
			
			const std::string layerIndexAsString = std::to_string(layerIndex);
			bool layerCreated = false;

			// Inside the first texture name of the terrain layer we store the global asset ID of the QSF material the terrain layer is using, we need nothing more
			//nasty hack
			//if(layerIndex == 0)
			std::string LayerName = ogreTerrain->getLayerTextureName(layerIndex,0);
			//std::string globalAssetIdAsString = Textures.at(layerIndex);
			//QSF_LOG_PRINTS(INFO, "material generator"<< layerIndex << " "  << globalAssetIdAsString)
			uint64 globalAssetId = qsf::getUninitialized<uint64>();
			//std::string globalAssetIdAsString = ogreTerrain->getLayerTextureName(layerIndex, 0);
			if(qsf::AssetProxy(LayerName).getAsset() != nullptr)
				globalAssetId = qsf::AssetProxy(LayerName).getGlobalAssetId();
			static const qsf::AssetSystem& assetSystem = QSF_ASSET;

			uint64 globalAssetId__ = qsf::getUninitialized<uint64>();
			if (nullptr != qsf::AssetProxy(LayerName).getAsset())
				globalAssetId__ = qsf::AssetProxy(LayerName).getGlobalAssetId();

				if (nullptr != qsf::AssetProxy(LayerName).getAsset())//assetSystem.getAssetByGlobalAssetId(globalAssetId))
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
				
						QSF_ERROR("Terrain layer material " << LayerName << " not found, restart editor, ignore this error and use terrain tool to reevaluate all layer (1)" << LayerName, QSF_REACT_NONE);
					}
				}
				else
				{
					QSF_ERROR("Terrain layer asset " << LayerName << " not found, restart editor, ignore this error and use terrain tool to reevaluate all layer (2)" << LayerName, QSF_REACT_NONE);
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
	void TerrainMaterialGenerator::onAssetsMounted(const qsf::Assets& assets)
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


	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
} // qsf
