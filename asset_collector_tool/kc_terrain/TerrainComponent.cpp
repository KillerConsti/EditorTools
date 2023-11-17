// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool\kc_terrain\TerrainComponent.h>
#include <asset_collector_tool\kc_terrain\TerrainContext.h>
#include <asset_collector_tool\kc_terrain\TerrainDefinition.h>
#include "qsf/renderer/component/CameraComponent.h"
#include "qsf/renderer/material/material/MaterialManager.h"
#include "qsf/renderer/material/MaterialSystem.h"
#include "qsf/renderer/window/RenderWindow.h"
#include "qsf/renderer/RendererSystem.h"
#include "qsf/component/base/TransformComponent.h"
#include "qsf/component/utility/BoostSignalComponent.h"
#include "qsf/map/Map.h"
#include "qsf/map/Entity.h"
#include "qsf/math/Convert.h"
#include "qsf/plugin/QsfAssetTypes.h"
#include "qsf/asset/Asset.h"
#include "qsf/asset/AssetSystem.h"
#include "qsf/settings/SettingsGroupManager.h"
#include "qsf/settings/RendererSettingsGroup.h"
#include "qsf/log/LogSystem.h"
#include "qsf/QsfHelper.h"

#include <OGRE/Terrain/OgreTerrain.h>
#include <OGRE/Terrain/OgreTerrainGroup.h>

#include <OGRE/OgreMaterialManager.h>
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace kc_terrain
{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	const uint32 TerrainComponent::COMPONENT_ID			   = qsf::StringHash("kc_terrain::TerrainComponent");
	const uint32 TerrainComponent::HEIGHT_MAP_SIZE		   = qsf::StringHash("HeightMapSize");
	const uint32 TerrainComponent::COLOR_MAP_SIZE		   = qsf::StringHash("ColorMapSize");
	const uint32 TerrainComponent::BLEND_MAP_SIZE		   = qsf::StringHash("BlendMapSize");
	const uint32 TerrainComponent::TERRAIN_CHUNKS_PER_EDGE = qsf::StringHash("TerrainChunksPerEdge");
	const uint32 TerrainComponent::TERRAIN_WORLD_SIZE	   = qsf::StringHash("TerrainWorldSize");
	const uint32 TerrainComponent::SKIRT_SIZE			   = qsf::StringHash("SkirtSize");
	const uint32 TerrainComponent::MAX_PIXEL_ERROR		   = qsf::StringHash("MaxPixelError");
	const uint32 TerrainComponent::TERRAIN_ASSET		   = qsf::StringHash("TerrainAsset");
	const uint8  TerrainComponent::RENDER_QUEUE_GROUP_ID = 50;	// See "Ogre::TerrainQuadTreeNode::Movable::Movable()"


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	TerrainComponent::~TerrainComponent()
	{
		if (nullptr != mTerrainDefinition)
		{
			delete mTerrainDefinition;
		}
	}

	void TerrainComponent::setHeightMapSize(uint32 heightMapSize)
	{
		// TODO(co) I assume updating other data as a result is required
		mHeightMapSize = heightMapSize;
	}

	void TerrainComponent::setColorMapSize(uint32 colorMapSize)
	{
		// TODO(co) I assume updating other data as a result is required
		mColorMapSize = colorMapSize;
	}

	void TerrainComponent::setBlendMapSize(uint32 blendMapSize)
	{
		// TODO(co) I assume updating other data as a result is required
		mBlendMapSize = blendMapSize;
	}

	void TerrainComponent::setTerrainChunksPerEdge(int terrainChunksPerEdge)
	{
		// TODO(co) I assume updating other data as a result is required
		mTerrainChunksPerEdge = terrainChunksPerEdge;
	}

	void TerrainComponent::setTerrainWorldSize(float terrainWorldSize)
	{
		// State change?
		if (mTerrainWorldSize != terrainWorldSize)
		{
			// Backup the given value
			mTerrainWorldSize = terrainWorldSize;

			// Redirect the request to the OGRE terrain
			if (nullptr != mOgreTerrainGroup)
			{
				// OGRE handles the terrain world size for each single terrain inside the terrain group, for QSF in order to keep things simple for the user the terrain world size is for the whole thing
				mOgreTerrainGroup->setTerrainWorldSize(mTerrainWorldSize / static_cast<float>(mTerrainChunksPerEdge));
			}

			// Promote the property change
			promotePropertyChange(TERRAIN_WORLD_SIZE);
		}
	}

	void TerrainComponent::setSkirtSize(float skirtSize)
	{
		// State change?
		if (mSkirtSize != skirtSize)
		{
			// Backup the given value
			mSkirtSize = skirtSize;

			// Redirect the request to the OGRE terrain
			if (nullptr != mOgreTerrainGlobalOptions)
			{
				mOgreTerrainGlobalOptions->setSkirtSize(mSkirtSize);
				buildHeightMap();
			}

			// Promote the property change
			promotePropertyChange(SKIRT_SIZE);
		}
	}

	void TerrainComponent::setMaxPixelError(float maxPixelError)
	{
		// State change?
		if (mMaxPixelError != maxPixelError)
		{
			// Backup the given value
			mMaxPixelError = maxPixelError;

			// Redirect the request to the OGRE terrain
			updateOgreTerrainMaxPixelError();

			// Promote the property change
			promotePropertyChange(MAX_PIXEL_ERROR);
		}
	}

	bool TerrainComponent::getTerrainHitByRenderWindow(const qsf::RenderWindow& renderWindow, int xPosition, int yPosition, glm::vec3* position) const
	{
		// Get the camera component the render window is using
		const qsf::CameraComponent* cameraComponent = renderWindow.getCameraComponent();
		if (nullptr != cameraComponent)
		{
			// Get the normalized mouse position
			const glm::vec2 normalizedPosition = renderWindow.getNormalizedWindowSpaceCoords(xPosition, yPosition);

			// Get a ray at the given viewport position
			const Ogre::Ray ray = qsf::Convert::getOgreRay(cameraComponent->getRayAtViewportPosition(normalizedPosition.x, normalizedPosition.y));

			// Intersect the ray with terrain and if it hit, return the position
			return getTerrainHitByRay(ray, position);
		}

		// Error!
		return false;
	}

	bool TerrainComponent::getTerrainHitByRay(const Ogre::Ray& ray, glm::vec3* position) const
	{
		// Intersect the ray with terrain and if it hit, return the position
		if (nullptr != mOgreTerrainGroup)
		{
			const Ogre::TerrainGroup::RayResult rayResult = mOgreTerrainGroup->rayIntersects(ray);
			if (rayResult.hit)
			{
				if (nullptr != position)
				{
					*position = qsf::Convert::getGlmVec3(rayResult.position);
				}
				return true;
			}
		}

		// Error!
		return false;
	}

	bool TerrainComponent::getTerrainHitBoundingBoxByRay(Ogre::Ray& ray, float* closestDistance) const
	{
		if (nullptr != mOgreTerrainGroup)
		{
			Ogre::TerrainGroup::TerrainIterator ogreTerrainIterator = mOgreTerrainGroup->getTerrainIterator();
			while (ogreTerrainIterator.hasMoreElements())
			{
				const Ogre::Terrain* ogreTerrain = ogreTerrainIterator.getNext()->instance;
				if (nullptr != ogreTerrain)
				{
					const std::pair<bool, Ogre::Real> result = ray.intersects(ogreTerrain->getWorldAABB());
					if (result.first)
					{
						// Hit
						if (nullptr != closestDistance)
						{
							*closestDistance = result.second;
						}
						return true;
					}
				}
			}
		}

		// No hit
		return false;
	}

	int TerrainComponent::getTerrainSegments() const
	{
		// If we have no terrain group, return
		if (nullptr == mOgreTerrainGroup)
		{
			return 0;
		}

		// If we have no terrain group, return
		Ogre::Terrain* ogreTerrain = mOgreTerrainGroup->getTerrain(0, 0);
		if (nullptr == ogreTerrain)
		{
			return 0;
		}

		return (ogreTerrain->getSize() - 1) * mTerrainChunksPerEdge + 1;
	}

	void TerrainComponent::setTerrainAsset(const qsf::AssetProxy& assetProxy)
	{
		// State change?
		if (mTerrainAsset != assetProxy)
		{
			// Backup the given value
			mTerrainAsset = assetProxy;

			// Redirect the request to the OGRE terrain
			if (nullptr != mOgreTerrainGroup)
			{
				buildHeightMap();
			}

			// Promote the property change
			promotePropertyChange(TERRAIN_ASSET);
		}
	}

	void TerrainComponent::setEditing(bool isEditing)
	{
		mIsEditing = isEditing;

		if (nullptr != mOgreTerrainGroup)
		{
			removeAllOgreTerrains();

			// Load terrain
			defineTerrain();
			try
			{
				// TODO(co) Don't asynchronously load the OGRE terrain, when using OGRE 1.9 there are certain hang issues
				mOgreTerrainGroup->loadAllTerrains(true);
			}
			catch (const std::exception& e)
			{
				QSF_ERROR("QSF failed to load the terrain. Exception caught: " << e.what(), QSF_REACT_NONE);
			}

			mOgreTerrainGroup->freeTemporaryResources();
		}
	}

	TerrainDefinition* TerrainComponent::getTerrainDefinition()
	{
		if (nullptr == mTerrainDefinition)
		{
			mTerrainDefinition = new TerrainDefinition(getEntity().getMap().getGlobalAssetId(), getEntityId());
			loadTerrainDefinition();
		}
		return mTerrainDefinition;
	}

	void TerrainComponent::SetNewColorMap(qsf::GlobalAssetId NewAssetId)
	{
	return;
		static qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
		QSF_LOG_PRINTS(INFO, "matname: " << 1)
		if(mTerrainContext->GetMaterialGenerator() != nullptr)
		QSF_LOG_PRINTS(INFO,"matgen is not null")
		if(getOgreTerrain() == nullptr)
		QSF_LOG_PRINTS(INFO,"Ogre Terrain is null")
			QSF_LOG_PRINTS(INFO, getOgreTerrain()->getMaterialName().c_str())
			qsf::Material* terrainMaterial = materialManager.findElement(qsf::StringHash(getOgreTerrain()->getMaterialName().c_str()));
			//326081
		terrainMaterial->setPropertyById("GlobalColorMap", qsf::MaterialPropertyValue::fromGlobalAssetId(326081));
		
		mTerrainContext->GetMaterialGenerator()->generate(getOgreTerrain());
		Ogre::MaterialManager::getSingleton().reloadAll();
		//mTerrainContext->GetMaterialGenerator()->u
		mTerrainContext->TerrainContext::releaseContextReference();
		mTerrainContext = new kc_terrain::TerrainContext();
		mTerrainContext->addContextReference();
	}

	qsf::GlobalAssetId TerrainComponent::GetColorMap()
	{
		return mColorMap;
	}

	Ogre::Terrain* TerrainComponent::getOgreTerrain() const
	{
		// TODO(co) This method looks odd, wasn't the implementation changed to use terrain group?
		return (nullptr != mOgreTerrainGroup) ? mOgreTerrainGroup->getTerrain(0, 0) : nullptr;
	}


	//[-------------------------------------------------------]
	//[ Protected virtual qsf::Component methods              ]
	//[-------------------------------------------------------]
	void TerrainComponent::onComponentPropertyChange(const Component& component, uint32 propertyId)
	{
		// Call the base implementation
		RendererComponent::onComponentPropertyChange(component, propertyId);

		// Evaluate the component
		if (qsf::TransformComponent::COMPONENT_ID == component.getId())
		{
			if (qsf::TransformComponent::POSITION == propertyId && nullptr != mOgreTerrainGroup)
			{
				setPosition(static_cast<const qsf::TransformComponent&>(component).getPosition());
			}

			// Rotation and scale are not supported by the OGRE terrain
		}
	}

	bool TerrainComponent::onStartup()
	{
		// Get the OGRE scene manager instance
		Ogre::SceneManager* ogreSceneManager = getOgreSceneManager();
		if (nullptr != ogreSceneManager)
		{
			// Add a context reference
			mTerrainContext = new kc_terrain::TerrainContext();
			mTerrainContext->addContextReference();

			// Request OGRE terrain globals instance
			mOgreTerrainGlobalOptions = mTerrainContext->getOgreTerrainGlobalOptions();
			QSF_ASSERT(nullptr != mOgreTerrainGlobalOptions, "QSF failed to obtain an global OGRE terrain options instance", QSF_REACT_NONE);

			// Create OGRE terrain group instance
			// -> OGRE handles the terrain world size for each single terrain inside the terrain group, for QSF in order to keep things simple for the user the terrain world size is for the whole thing
			mOgreTerrainGroup = new Ogre::TerrainGroup(ogreSceneManager, Ogre::Terrain::ALIGN_X_Z, 65, mTerrainWorldSize / static_cast<float>(mTerrainChunksPerEdge));
			{
				// mOgreTerrainGlobalOptions->setCastsDynamicShadows(true);	// TODO(co) Casting shadow looks fine on the empty map, but creates nasty artefact on other maps?
				mOgreTerrainGlobalOptions->setSkirtSize(mSkirtSize);
				updateOgreTerrainMaxPixelError();
				mOgreTerrainGlobalOptions->setLayerBlendMapSize(mBlendMapSize);
			}

			{ // Use transform component, in case there's one
				const qsf::TransformComponent* transformComponent = getEntity().getComponent<qsf::TransformComponent>();
				if (nullptr != transformComponent)
				{
					// Position
					setPosition(transformComponent->getPosition());

					// Rotation and scale are not supported by the OGRE terrain
				}
			}

			// Connect our Boost slot to the Boost signal of the QSF asset system
			QSF_ASSET.AssetsMounted.connect(boost::bind(&TerrainComponent::onAssetsMounted, this, _1));
			QSF_ASSET.AssetsUnmounted.connect(boost::bind(&TerrainComponent::onAssetsUnmounted, this, _1));

			buildHeightMap();

			// Update the visibility state of the internal OGRE scene node
			updateOgreSceneNodeVisibility();

			// We want to listen on component property changes inside the core entity: Get "qsf::BoostSignalComponent" instance or create it in case it does not exist, yet
			qsf::BoostSignalComponent* boostSignalComponent = getEntity().getMap().getCoreEntity().getOrCreateComponent<qsf::BoostSignalComponent>();
			if (nullptr != boostSignalComponent)
			{
				// Connect to the Boost signal
				boostSignalComponent->ComponentPropertyChange.connect(boost::bind(&TerrainComponent::onComponentPropertyChange, this, _1, _2));
			}

			QSF_SETTINGSGROUP.PropertyChanged.connect(boost::bind(&TerrainComponent::onSettingsPropertyChanged, this, _1, _2));

			// Done
			return true;
		}

		// Error!
		return false;
	}

	void TerrainComponent::onShutdown()
	{
		QSF_SETTINGSGROUP.PropertyChanged.disconnect(boost::bind(&TerrainComponent::onSettingsPropertyChanged, this, _1, _2));

		// Get the OGRE scene manager instance
		Ogre::SceneManager* ogreSceneManager = getOgreSceneManager();
		if (nullptr != ogreSceneManager)
		{
			{ // We want to stop listening on component property changes inside the core entity
				qsf::BoostSignalComponent* boostSignalComponent = getEntity().getMap().getCoreEntity().getComponent<qsf::BoostSignalComponent>();
				if (nullptr != boostSignalComponent)
				{
					// Disconnect from the Boost signal
					boostSignalComponent->ComponentPropertyChange.disconnect(boost::bind(&TerrainComponent::onComponentPropertyChange, this, _1, _2));
				}
			}

			// Disconnect our Boost slot from the Boost signal of the QSF asset system
			QSF_ASSET.AssetsMounted.disconnect(boost::bind(&TerrainComponent::onAssetsMounted, this, _1));
			QSF_ASSET.AssetsUnmounted.disconnect(boost::bind(&TerrainComponent::onAssetsUnmounted, this, _1));

			mGlobalTerrainAssetIds.clear();

			// Destroy OGRE terrain group and OGRE terrain globals instance
			// TODO(co) Terrain shutdown can take ages. I appears that the light map generation, which runs in a separate thread continues running until it's done.
			// See "PixelBox* Terrain::calculateLightmap(const Rect& rect, const Rect& extraTargetRect, Rect& outFinalRect)" in OGRE terrain. Can we shut down this thread early?
			removeAllOgreTerrains();
			delete mOgreTerrainGroup;
			mOgreTerrainGroup = nullptr;
			mOgreTerrainGlobalOptions = nullptr;	// Don't destroy the instance, it's managed by the terrain context

			// Release a context reference
			mTerrainContext->TerrainContext::releaseContextReference();
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual qsf::RendererComponent methods      ]
	//[-------------------------------------------------------]
	void TerrainComponent::setOgreSceneNodeVisibility(bool visible)
	{
		if (nullptr != mOgreTerrainGroup)
		{
			Ogre::TerrainGroup::TerrainIterator terrainIterator = mOgreTerrainGroup->getTerrainIterator();
			while (terrainIterator.hasMoreElements())
			{
				Ogre::Terrain* ogreTerrain = terrainIterator.getNext()->instance;
				if (nullptr != ogreTerrain)
				{
					ogreTerrain->_getRootSceneNode()->setVisible(visible);
				}
			}
		}
	}


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void TerrainComponent::onSettingsPropertyChanged(const qsf::ParameterGroup& parameterGroup, uint32 property)
	{
		if (&parameterGroup == qsf::RendererSettingsGroup::getInstance() && property == qsf::RendererSettingsGroup::LOD_BIAS)
		{
			updateOgreTerrainMaxPixelError();
		}
	}

	void TerrainComponent::updateOgreTerrainMaxPixelError()
	{
		if (nullptr != mOgreTerrainGlobalOptions)
		{
			// Disable LOD usage
			// TODO(co) This is primarily for EMERGENCY 5 were we always need the full terrain quality, or horrible visual clipping artifacts will come up
			mOgreTerrainGlobalOptions->setMaxPixelError(mMaxPixelError * qsf::RendererSettingsGroup::getInstanceSafe().getLodBias());
		}
	}

	void TerrainComponent::onAssetsMounted(const qsf::Assets& assets)
	{
		for (const qsf::Asset* asset : assets)
		{
			const qsf::AssetTypeId typeId = asset->getTypeId();
			if (typeId == qsf::QsfAssetTypes::TERRAIN || typeId == qsf::QsfAssetTypes::OGRE_TERRAIN)
			{
				const qsf::GlobalAssetId globalAssetId = asset->getGlobalAssetId();
				if (mGlobalTerrainAssetIds.find(globalAssetId) != mGlobalTerrainAssetIds.end())
				{
					QSF_LOG_PRINTS(TRACE, "Mounting asset " << asset->getName() << "(" << globalAssetId << ")");
					onAssetChanged(*asset);
				}
			}
		}
	}

	void TerrainComponent::onAssetsUnmounted(const qsf::Assets& assets)
	{
		for (const qsf::Asset* asset : assets)
		{
			const qsf::AssetTypeId typeId = asset->getTypeId();
			if (typeId == qsf::QsfAssetTypes::TERRAIN || typeId == qsf::QsfAssetTypes::OGRE_TERRAIN)
			{
				const qsf::GlobalAssetId globalAssetId = asset->getGlobalAssetId();
				if (mGlobalTerrainAssetIds.find(globalAssetId) != mGlobalTerrainAssetIds.end())
				{
					QSF_LOG_PRINTS(TRACE, "Unmounting asset " << asset->getName() << "(" << globalAssetId << ")");
					onAssetChanged(*asset);
				}
			}
		}
	}

	void TerrainComponent::onAssetChanged(const qsf::Asset& asset)
	{
		const qsf::GlobalAssetId globalAssetId = asset.getGlobalAssetId();
		if (mTerrainAsset.getGlobalAssetId() == globalAssetId)
		{
			// Create a fresh asset proxy, since the old one might have cached some no longer valid data...
			mTerrainAsset = qsf::AssetProxy(mTerrainAsset.getGlobalAssetId());
			buildHeightMap();
		}
		else if (!mIsEditing)
		{
			// We only care for ogre terrain chunks here since all other dependencies are textures, which are already covered by our general delayed loading support
			if (asset.getTypeId() == qsf::QsfAssetTypes::OGRE_TERRAIN)
			{
				for (int x = 0; x < mTerrainChunksPerEdge; ++x)
				{
					for (int y = 0; y < mTerrainChunksPerEdge; ++y)
					{
						if (mTerrainDefinition->getOgreTerrainChunk(x, y) == globalAssetId)
						{
							mOgreTerrainGroup->removeTerrain(x, y);

							// See if an asset is mounted with this global asset ID
							const std::string relativeFilename = qsf::AssetProxy(globalAssetId).getCachedAssetDataFilename();
							if (relativeFilename.empty())
							{
								mOgreTerrainGroup->defineTerrain(x, y, 1.0f);
							}
							else
							{
								mOgreTerrainGroup->defineTerrain(x, y, relativeFilename);
							}

							try
							{
								// TODO(co) Don't asynchronously load the OGRE terrain, when using OGRE 1.9 there are certain hang issues
								mOgreTerrainGroup->loadAllTerrains(true);
							}
							catch (const std::exception& e)
							{
								QSF_ERROR("QSF failed to load the terrain. Exception caught: " << e.what(), QSF_REACT_NONE);
							}

							mOgreTerrainGroup->freeTemporaryResources();
						}
					}
				}
			}
		}
	}

	void TerrainComponent::loadTerrainDefinition()
	{
		if (mTerrainAsset.isValid())
		{
			mGlobalTerrainAssetIds.clear();
			mGlobalTerrainAssetIds.insert(mTerrainAsset.getGlobalAssetId());

			if (!mTerrainAsset.getCachedAssetDataFilename().empty() && mTerrainDefinition->loadFromAsset(mTerrainAsset.getGlobalAssetId()))
			{
				std::vector<qsf::GlobalAssetId> globalDerivedAssetIds;
				mTerrainDefinition->getDerivedGlobalAssetIds(globalDerivedAssetIds);
				mGlobalTerrainAssetIds.insert(globalDerivedAssetIds.begin(), globalDerivedAssetIds.end());
			}
			else
			{
				mTerrainDefinition->clear();
			}
		}
	}

	void TerrainComponent::defineTerrain()
	{
		if (nullptr != mOgreTerrainGroup)
		{
			{
				const int size = mColorMapSize / mTerrainChunksPerEdge;
				mOgreTerrainGlobalOptions->setLightMapSize(size);
				mOgreTerrainGlobalOptions->setDefaultGlobalColourMapSize(size);
				mOgreTerrainGlobalOptions->setLayerBlendMapSize(mBlendMapSize / mTerrainChunksPerEdge);
			}

			Ogre::Terrain::ImportData& ogreTerrainImportData = mOgreTerrainGroup->getDefaultImportSettings();
			{
				ogreTerrainImportData.terrainSize = (mHeightMapSize / mTerrainChunksPerEdge) + 1;
				ogreTerrainImportData.worldSize = mTerrainWorldSize;
				ogreTerrainImportData.inputScale = 1.0f;
				ogreTerrainImportData.minBatchSize = (ogreTerrainImportData.minBatchSize > ogreTerrainImportData.terrainSize) ? ogreTerrainImportData.terrainSize : 65;
				if (ogreTerrainImportData.minBatchSize > 65)
				{
					ogreTerrainImportData.minBatchSize = 65;
				}
				ogreTerrainImportData.maxBatchSize = ogreTerrainImportData.minBatchSize;	// We're not using terrain LOD since this caused problems multiple times

				// Textures
				ogreTerrainImportData.layerList.resize(0);
			}

			// TODO(sw) HACK: We ignore here the editing flag this change seems to solve that when changing into editing mode that the terrain is shown in an lower LOD level (QSF Bug H239)
			// I guess the reason for this was that in ogre 1.9.x the terrain module wasn't releasing the file handles after loading the data.
			// But why does this different methods have an impact on the visual aspect of the terrain?
			// Does loading the terrain data from an file sets some data different then defining the terrain from an import data definition?
			mOgreTerrainGroup->setTerrainSize(ogreTerrainImportData.terrainSize);
			if (mTerrainDefinition->isValid())
			{
				for (int x = 0; x < mTerrainChunksPerEdge; ++x)
				{
					for (int y = 0; y < mTerrainChunksPerEdge; ++y)
					{
						mOgreTerrainGroup->defineTerrain(x, y, qsf::AssetProxy(mTerrainDefinition->getOgreTerrainChunk(x, y)).getCachedAssetDataFilename());
					}
				}
			}
			else
			{
				for (int x = 0; x < mTerrainChunksPerEdge; ++x)
				{
					for (int y = 0; y < mTerrainChunksPerEdge; ++y)
					{
						mOgreTerrainGroup->defineTerrain(x, y, 1.0f);
					}
				}
			}
		}
	}

	void TerrainComponent::removeAllOgreTerrains()
	{
		// Optimization: To avoid constant allocations/deallocations, use a static instance (not multi-threading safe, of course)
		static std::vector<qsf::MaterialId> materialIds;
		materialIds.clear();

		{ // The OGRE terrain is pretty out-of-date and has been poorly migrated over the years from one OGRE version to another. Ensure that the referenced materials are gone.
			Ogre::TerrainGroup::TerrainIterator ogreTerrainIterator = mOgreTerrainGroup->getTerrainIterator();
			while (ogreTerrainIterator.hasMoreElements())
			{
				const Ogre::Terrain* ogreTerrain = ogreTerrainIterator.getNext()->instance;
				if (nullptr != ogreTerrain)
				{
					// Just gather, don't destroy or OGRE internals will go nuts
					// -> See "Ogre::HlmsDatablock::~HlmsDatablock()": Assert: "This Datablock is still being used by some Renderables. Change their Datablocks before destroying this."
					materialIds.push_back(qsf::StringHash(ogreTerrain->getMaterialName()));
				}
			}
		}

		// Tell the OGRE terrain group to kill'em all
		mOgreTerrainGroup->removeAllTerrains();

		// Destroy no longer required materials
		static qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
		for (qsf::MaterialId materialId : materialIds)
		{
			materialManager.destroyElement(materialId);
		}
	}

	void TerrainComponent::buildHeightMap()
	{
		if (nullptr != mOgreTerrainGroup)
		{
			if (!mIsEditing)
			{
				removeAllOgreTerrains();
			}

			if (nullptr != mTerrainDefinition && (mTerrainDefinition->getGlobalMapAssetId() != getEntity().getMap().getGlobalAssetId() || mTerrainDefinition->getEntityId() != getEntityId()))
			{
				delete mTerrainDefinition;
				mTerrainDefinition = nullptr;
			}
			if (nullptr != mTerrainDefinition)
			{
				loadTerrainDefinition();
			}
			else
			{
				mTerrainDefinition = getTerrainDefinition();
			}

			if (!mIsEditing)
			{
				// Load terrain
				defineTerrain();
				try
				{
					// TODO(co) Don't asynchronously load the OGRE terrain, when using OGRE 1.9 there are certain hang issues
					mOgreTerrainGroup->loadAllTerrains(true);
				}
				catch (const std::exception& e)
				{
					QSF_ERROR("QSF failed to load the terrain. Exception caught: " << e.what(), QSF_REACT_NONE);
				}

				mOgreTerrainGroup->freeTemporaryResources();

				// TODO(co) The OGRE default stuff eats up our valuable CPU performance
				/*
				// The default OGRE terrain behaviour is horrible. Each and every terrain instance will register itself as
				// frame listener and will perform some CPU heavy work inside "Ogre::Terrain::preFindVisibleObjects()" - especially
				// the "Ogre::TerrainQuadTreeNode::calculateCurrentLod()" call is evil. This method does CPU work for each and
				// every terrain instance, the early-out for not visible terrains was commented by someone with
				// "disable this, could cause 'jumps' in LOD as children go out of frustum".
				// -> So, remove the terrain instance at once as frame listener to get rid of this performance killing insanity.
				// -> Not critical, because we're going to destroy the terrain instance before we destroy the OGRE scene manger instance.
				Ogre::TerrainGroup::TerrainIterator terrainIterator = mOgreTerrainGroup->getTerrainIterator();
				while (terrainIterator.hasMoreElements())
				{
					Ogre::Terrain* ogreTerrain = terrainIterator.getNext()->instance;
					if (nullptr != ogreTerrain)
					{
						ogreTerrain->_getRootSceneNode()->getCreator()->removeListener(ogreTerrain);
					}
				}
				*/
			}
		}
	}

	void TerrainComponent::setPosition(const glm::vec3& position)
	{
		if (nullptr != mOgreTerrainGroup)
		{
			const float offset = (mTerrainChunksPerEdge - 1) / 2.0f * static_cast<float>(mTerrainWorldSize) / mTerrainChunksPerEdge;
			Ogre::Vector3 ogrePosition(qsf::Convert::getOgreVector3(position));
			ogrePosition.x -= offset;
			//nasty position hack
			ogrePosition.y += 10.f;
			ogrePosition.z += offset;
			mOgreTerrainGroup->setOrigin(ogrePosition);
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf
