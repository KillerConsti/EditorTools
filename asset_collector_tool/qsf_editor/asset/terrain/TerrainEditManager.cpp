// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "qsf_editor/PrecompiledHeader.h"
#include "qsf_editor/asset/terrain/TerrainEditManager.h"
#include "qsf_editor/asset/terrain/TerrainEditHelper.h"

#include <qsf/asset/Asset.h>
#include <qsf/asset/AssetSystem.h>
#include <qsf/plugin/QsfAssetTypes.h>
#include <qsf/map/Map.h>
#include <qsf/QsfHelper.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace qsf
{
	namespace editor
	{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainEditManager::TerrainEditManager()
		{
			// Connect our Boost slot to the Boost signal of the QSF asset system
			// -> Connect at_front to make sure we're updated before less important stuff
			QSF_ASSET.AssetsMounted.connect(boost::bind(&TerrainEditManager::onAssetsChanged, this, _1), boost::signals2::at_front);
			QSF_ASSET.AssetsUnmounted.connect(boost::bind(&TerrainEditManager::onAssetsChanged, this, _1));
		}

		TerrainEditManager::~TerrainEditManager()
		{
			// Disconnect our Boost slot from the Boost signal of the QSF asset system
			QSF_ASSET.AssetsMounted.disconnect(boost::bind(&TerrainEditManager::onAssetsChanged, this, _1));
			QSF_ASSET.AssetsUnmounted.disconnect(boost::bind(&TerrainEditManager::onAssetsChanged, this, _1));
		}

		bool TerrainEditManager::isReady() const
		{
			// Check all terrain edit helper instances
			for (const auto& element : mTerrainEditHelperMap)
			{
				// TODO(co) It's not cool that "mTerrainEditHelperMap" can contain null pointers
				const TerrainEditHelper* terrainEditHelper = element.second.lock().get();
				if (nullptr != terrainEditHelper && !terrainEditHelper->isReady())
				{
					// Not ready
					return false;
				}
			}

			// Ready
			return true;
		}

		std::shared_ptr<TerrainEditHelper> TerrainEditManager::getTerrainEditHelper(uint64 entityId)
		{
			// Try to receive already existing terrain edit helper
			const GlobalAssetId globalMapAssetId = QSF_MAINMAP.getGlobalAssetId();
			TerrainEditHelperMap::const_iterator iterator = mTerrainEditHelperMap.find(std::make_pair(globalMapAssetId, entityId));
			std::shared_ptr<TerrainEditHelper> terrainEditHelper = (mTerrainEditHelperMap.end() != iterator) ? iterator->second.lock() : nullptr;

			// Startup terrain edit helper
			if (nullptr == terrainEditHelper)
			{
				terrainEditHelper = std::shared_ptr<TerrainEditHelper>(new TerrainEditHelper(globalMapAssetId, entityId));
				terrainEditHelper->startup();
				mTerrainEditHelperMap[std::make_pair(globalMapAssetId, entityId)] = terrainEditHelper;
			}

			// Done
			return terrainEditHelper;
		}

		std::shared_ptr<TerrainEditHelper> TerrainEditManager::findTerrainEditHelper(uint64 entityId) const
		{
			TerrainEditHelperMap::const_iterator iterator = mTerrainEditHelperMap.find(std::make_pair(QSF_MAINMAP.getGlobalAssetId(), entityId));
			return (mTerrainEditHelperMap.end() != iterator) ? iterator->second.lock() : nullptr;
		}

		TerrainEditManager::LayerList& TerrainEditManager::getLayerList()
		{
			if (mLayerList.empty())
			{
				// Loop through all QSF materials
				std::vector<Asset*> assets;
				QSF_ASSET.getAssetsOfType(QsfAssetTypes::MATERIAL, assets);
				for (Asset* asset : assets)
				{
					if (asset->getCategory() == "terrain_layer")
					{
						// Register new terrain layer
						mLayerList.push_back(asset->getGlobalAssetId());
					}
				}
			}

			return mLayerList;
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void TerrainEditManager::onAssetsChanged(const Assets& assets)
		{
			for (const Asset* asset : assets)
			{
				// We're only interested in material assets
				if (asset->getTypeId() == QsfAssetTypes::MATERIAL)
				{
					// Forget about our cached layer list
					mLayerList.clear();

					// No need to continue
					break;
				}
			}
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // qsf
