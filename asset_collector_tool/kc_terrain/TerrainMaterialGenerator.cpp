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
#include "qsf/QsfHelper.h"

#include <OGRE/OgreMaterialManager.h>

#include <OGRE/Terrain/OgreTerrain.h>
#include <OGRE/Terrain/OgreTerrainGroup.h>

#include <qsf\log\LogSystem.h>
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
			const int terrainChunksPerEdge = terrainComponent.getTerrainChunksPerEdge();
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
			const int   blendMapIndex	  = (layerIndex - 1) / 4;
			const int   blendMapComponent = (layerIndex - 1) % 4;
			const float uvMultiplier	  = ogreTerrain.getLayerUVMultiplier(layerIndex);

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
	TerrainMaterialGenerator::Profile::Profile(Ogre::TerrainMaterialGenerator* parent, const Ogre::String& name, const Ogre::String& desc,uint64 ColorMap) :
		Ogre::TerrainMaterialGenerator::Profile(parent, name, desc)
	{
		m_profil_ColorMap = ColorMap;
		QSF_LOG_PRINTS(INFO, "constructor " <<m_profil_ColorMap);
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
		const Ogre::String& matName = ogreTerrain->getMaterialName();
		mMatName = matName;

		createMaterial(matName, ogreTerrain);

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
		QSF_LOG_PRINTS(INFO,"TMG"<< ColorMap)
		mProfiles.push_back(OGRE_NEW Profile(this, "SM2", "Profile for rendering on Shader Model 2 capable cards",ColorMap));
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


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void TerrainMaterialGenerator::Profile::createMaterial(const Ogre::String& matName, const Ogre::Terrain* ogreTerrain)
	{
		static qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
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
			// A global color map which spans all terrain chunks, usually only used during runtime for efficiency
			//static const qsf::GlobalAssetId missingTextureGlobalAssetId = qsf::AssetProxy("qsf/texture/default/missing").getGlobalAssetId();
			const kc_terrain::TerrainDefinition* terrainDefinition = terrainComponent->getTerrainDefinition();
			/*qsf::GlobalAssetId globalAssetId = (nullptr != terrainDefinition && terrainDefinition->isValid()) ? terrainDefinition->getColorMap() : qsf::getUninitialized<qsf::GlobalAssetId>();
			if (qsf::isUninitialized(globalAssetId) || nullptr == QSF_ASSET.getAssetByGlobalAssetId(globalAssetId))
			{
				globalAssetId = ColorMapAsset;
			}*/
			terrainMaterial->setPropertyById("UseGlobalColorMap", qsf::MaterialPropertyValue::fromBoolean(qsf::isInitialized(ColorMapAsset)));
			terrainMaterial->setPropertyById("GlobalColorMap", qsf::MaterialPropertyValue::fromGlobalAssetId(ColorMapAsset));
		}

		// Global normal map
		if (ogreTerrain->getTerrainNormalMap().isNull())
		{
			// Global normal map transform
			::detail::setMapTransformMaterialProperty(*terrainMaterial, *terrainComponent, *ogreTerrain, "GlobalNormalMapTransform", true);

			// A global normal map which spans all terrain chunks, usually only used during runtime for efficiency
			const kc_terrain::TerrainDefinition* terrainDefinition = terrainComponent->getTerrainDefinition();
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
			else
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
		const uint32 numberOfBlendMaps = std::min(ogreTerrain->getBlendTextureCount(maximumNumberOfLayers), ogreTerrain->getBlendTextureCount());
		uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(ogreTerrain->getLayerCount()));
		terrainMaterial->setPropertyById("NumberOfLayers", qsf::MaterialPropertyValue::fromInteger(numberOfLayers));
		terrainMaterial->setPropertyById("NumberOfBlendMaps", qsf::MaterialPropertyValue::fromInteger(numberOfBlendMaps));

		// Blend maps
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
			const std::string globalAssetIdAsString = ogreTerrain->getLayerTextureName(layerIndex, 0);
			static const qsf::AssetSystem& assetSystem = QSF_ASSET;
			const qsf::GlobalAssetId globalAssetId = assetSystem.globalAssetIdAsStringToGlobalAssetId(globalAssetIdAsString);
			if (nullptr != assetSystem.getAssetByGlobalAssetId(globalAssetId))
			{
				const qsf::Material* layerMaterial = materialManager.findElement(qsf::StringHash(globalAssetIdAsString));
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
					QSF_ERROR("Terrain layer material " << globalAssetIdAsString << " not found, restart editor, ignore this error and use terrain tool to reevaluate all layer", QSF_REACT_NONE);
				}
			}
			else
			{
				QSF_ERROR("Terrain layer asset " << globalAssetId << " not found, restart editor, ignore this error and use terrain tool to reevaluate all layer", QSF_REACT_NONE);
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
