// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "qsf_editor/PrecompiledHeader.h"
#include "qsf_editor/asset/terrain/TerrainEditHelper.h"
#include "qsf_editor/asset/import/AssetPackageImportHelper.h"
#include "qsf_editor/asset/AssetEditHelper.h"
#include "qsf_editor/asset/AssetStorageManager.h"
#include "qsf_editor/selection/entity/EntitySelectionManager.h"
#include "qsf_editor/settings/EditorSettingsGroup.h"
#include "qsf_editor/EditorHelper.h"

#include <qsf_editor_base/asset/compiler/TextureAssetCompiler.h>
#include <qsf_editor_base/operation/component/SetComponentPropertyOperation.h>
#include <qsf_editor_base/operation/entity/SelectEntityHiddenOperation.h>
#include <qsf_editor_base/operation/entity/DeselectEntityHiddenOperation.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include <qsf_editor_base/operation/CommitOperation.h>

#include <qsf/asset/Asset.h>
#include <qsf/asset/AssetProxy.h>
#include <qsf/asset/AssetSystem.h>
#include <qsf/asset/project/AssetPackage.h>
#include <qsf/asset/project/Project.h>
#include <qsf/file/FileSystem.h>
#include <qsf/map/Map.h>
#include <qsf/map/Entity.h>
#include <qsf/math/Math.h>
#include <qsf/plugin/QsfAssetTypes.h>
#include <qsf/renderer/terrain/TerrainComponent.h>
#include <qsf/renderer/terrain/TerrainDefinition.h>
#include <qsf/component/base/TransformComponent.h>
#include <qsf/QsfHelper.h>

#include <boost/filesystem.hpp>

#include <OGRE/Terrain/OgreTerrainGroup.h>

#include <sstream>
#include <iomanip>


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
		void fakeReadyCallback(std::shared_ptr<qsf::editor::TerrainEditHelper> terrainEditHelper)
		{
			// Nothing to do in here
		}

		bool epsilonEqual(const glm::vec3& a, const glm::vec3& b, float epsilon)
		{
			return ((fabs(a.x - b.x) < epsilon) && (fabs(a.y - b.y) < epsilon) && (fabs(a.z - b.z) < epsilon));
		}

		qsf::GlobalAssetId createAsset(qsf::editor::AssetEditHelper& assetEditHelper, const std::string& targetAssetPackageName, const std::string& typeName, const std::string& category, const std::string& name)
		{
			const qsf::Asset* asset = assetEditHelper.addAsset(targetAssetPackageName, typeName, category, name);
			QSF_CHECK(nullptr != asset, "QSF failed to create the terrain asset \"" << typeName << "/" << category << "/" << name << "\"", QSF_REACT_THROW);
			return asset->getGlobalAssetId();
		}

		void createTerrainAssets(qsf::editor::AssetEditHelper& assetEditHelper, qsf::TerrainDefinition& terrainDefinition, const std::string& targetAssetPackageName, uint32 terrainChunksPerEdge)
		{
			const std::string assetNamePrefix = '_' + terrainDefinition.getName();

			// Create source assets
			qsf::GlobalAssetId globalSourceBlendMapAssetId  = createAsset(assetEditHelper, targetAssetPackageName, qsf::QsfAssetTypes::TERRAIN_DATA_MAP, "_terrain", assetNamePrefix + "_source_blend_map");
			qsf::GlobalAssetId globalSourceColorMapAssetId  = createAsset(assetEditHelper, targetAssetPackageName, qsf::QsfAssetTypes::TERRAIN_DATA_MAP, "_terrain", assetNamePrefix + "_source_color_map");
			qsf::GlobalAssetId globalSourceHeightMapAssetId = createAsset(assetEditHelper, targetAssetPackageName, qsf::QsfAssetTypes::TERRAIN_DATA_MAP, "_terrain", assetNamePrefix + "_source_height_map");

			// Create derived textures
			qsf::GlobalAssetId globalColorMapAssetId  = createAsset(assetEditHelper, targetAssetPackageName, qsf::QsfAssetTypes::TEXTURE, "_terrain", assetNamePrefix + "_color_map");
			qsf::GlobalAssetId globalNormalMapAssetId = createAsset(assetEditHelper, targetAssetPackageName, qsf::QsfAssetTypes::TEXTURE, "_terrain", assetNamePrefix + "_normal_map");

			// Create derived terrain chunk assets
			std::vector<std::vector<qsf::GlobalAssetId>> globalTerrainChunkAssetIds;
			globalTerrainChunkAssetIds.resize(terrainChunksPerEdge);
			for (uint32 x = 0; x < terrainChunksPerEdge; ++x)
			{
				globalTerrainChunkAssetIds[x].resize(terrainChunksPerEdge);
				for (uint32 y = 0; y < terrainChunksPerEdge; ++y)
				{
					std::stringstream assetNameSuffix;
					assetNameSuffix << std::setw(4) << std::setfill('0') << x << "_" << std::setw(4) << std::setfill('0') << y;
					const std::string assetName = assetNamePrefix + '_' + assetNameSuffix.str();
					globalTerrainChunkAssetIds[x][y] = createAsset(assetEditHelper, targetAssetPackageName, qsf::QsfAssetTypes::OGRE_TERRAIN, "_terrain", assetName);
				}
			}

			// Initialize terrain definition
			terrainDefinition.initialize(globalColorMapAssetId, globalNormalMapAssetId, globalSourceBlendMapAssetId, globalSourceColorMapAssetId, globalSourceHeightMapAssetId, globalTerrainChunkAssetIds);
		}

		qsf::GlobalAssetId cloneAsset(qsf::GlobalAssetId sourceGlobalAssetId, qsf::editor::AssetEditHelper& assetEditHelper, const std::string& targetAssetPackageName, const std::string& name)
		{
			qsf::Asset* asset = assetEditHelper.duplicateAsset(sourceGlobalAssetId, targetAssetPackageName, &name);
			QSF_CHECK(nullptr != asset, "QSF failed to clone the terrain asset \"" << sourceGlobalAssetId << "\"", QSF_REACT_THROW);
			return asset->getGlobalAssetId();
		}

		std::string reconstructSourceAbsoluteFilename(qsf::GlobalAssetId sourceGlobalAssetId, uint64 entityId, const std::string& extension)
		{
			return QSF_FILE.getBaseDirectory() + "/source/terrain_data_map/map_" + std::to_string(sourceGlobalAssetId) + "_entity_" + std::to_string(entityId) + '.' + extension;
		}

		std::string getSourceAbsoluteFilename(qsf::GlobalAssetId sourceGlobalAssetId, qsf::GlobalAssetId sourceMapGlobalAssetId, uint64 entityId, qsf::editor::AssetEditHelper& assetEditHelper, const std::string& extension)
		{
			std::string sourceAbsoluteFilename;
			const qsf::CachedAsset* cachedAsset = QSF_ASSET.getCachedAssetByGlobalAssetId(sourceGlobalAssetId);
			if (nullptr != cachedAsset)
			{
				QSF_FILE.virtualToAbsoluteFilename(cachedAsset->getCachedAssetDataFilename(), sourceAbsoluteFilename);
			}

			// Self-healing of defective assets: We had a situations were the absolute filenames were empty
			// -> Still needed as fallback so support terrain source data which hasn't been converted, yet
			if (sourceAbsoluteFilename.empty())
			{
				sourceAbsoluteFilename = QSF_EDITOR_ASSET_STORAGE.getAssetSourcePath(sourceGlobalAssetId);
				if (sourceAbsoluteFilename.empty())
				{
					// Try to reconstruct the absolute filename
					sourceAbsoluteFilename = reconstructSourceAbsoluteFilename(sourceMapGlobalAssetId, entityId, extension);
				}
			}

			// Done
			return sourceAbsoluteFilename;
		}

		qsf::GlobalAssetId cloneSourceAsset(qsf::GlobalAssetId sourceGlobalAssetId, qsf::GlobalAssetId sourceMapGlobalAssetId, qsf::GlobalAssetId targetMapGlobalAssetId, uint64 entityId, qsf::editor::AssetEditHelper& assetEditHelper, const std::string& targetAssetPackageName, const std::string& name, const std::string& extension)
		{
			qsf::Asset* targetAsset = assetEditHelper.duplicateAsset(sourceGlobalAssetId, targetAssetPackageName, &name);
			QSF_CHECK(nullptr != targetAsset, "QSF failed to clone the terrain asset \"" << sourceGlobalAssetId << "\"", QSF_REACT_THROW);
			const qsf::GlobalAssetId globalTargetAssetId = targetAsset->getGlobalAssetId();

			{ // Duplicate the source asset
				const std::string sourceAbsoluteFilename = getSourceAbsoluteFilename(sourceGlobalAssetId, sourceMapGlobalAssetId, entityId, assetEditHelper, extension);
				if (boost::filesystem::exists(sourceAbsoluteFilename))
				{
					qsf::AssetPackage* intermediateAssetPackage = assetEditHelper.getIntermediateAssetPackage();
					QSF_CHECK(nullptr != intermediateAssetPackage, "QSF failed to clone the terrain asset \"" << sourceGlobalAssetId << "\" because there's no intermediate asset package", QSF_REACT_THROW);
					const std::string targetAbsoluteFilename = QSF_FILE.getBaseDirectory() + '/' + targetAsset->generateRelativeFilename(*intermediateAssetPackage, extension);

					try
					{
						// Copy cached asset file
						boost::filesystem::create_directories(boost::filesystem::path(targetAbsoluteFilename).parent_path());
						boost::filesystem::copy_file(sourceAbsoluteFilename, targetAbsoluteFilename, boost::filesystem::copy_option::overwrite_if_exists);

						// Tell asset edit helper that the source asset has changes
						qsf::editor::AssetPackageImportHelper(*intermediateAssetPackage).importLocalCachedAsset(*targetAsset, targetAbsoluteFilename);
						assetEditHelper.setAssetUploadData(globalTargetAssetId, true, true);
					}
					catch (const std::exception& e)
					{
						QSF_ERROR("QSF editor failed to copy terrain source asset from \"" << sourceAbsoluteFilename << "\" to \"" << targetAbsoluteFilename << "\". Exception caught: " << e.what(), QSF_REACT_NONE);
					}
				}
			}

			// Done
			return globalTargetAssetId;
		}

		void cloneTerrainAssets(const qsf::TerrainDefinition& sourceTerrainDefinition, qsf::editor::AssetEditHelper& assetEditHelper, qsf::TerrainDefinition& terrainDefinition, const std::string& targetAssetPackageName, uint32 terrainChunksPerEdge)
		{
			const std::string assetNamePrefix = '_' + terrainDefinition.getName();

			// Create source assets
			const qsf::GlobalAssetId sourceMapGlobalAssetId = sourceTerrainDefinition.getGlobalMapAssetId();
			const qsf::GlobalAssetId targetMapGlobalAssetId = terrainDefinition.getGlobalMapAssetId();
			const uint64 entityId = sourceTerrainDefinition.getEntityId();
			qsf::GlobalAssetId globalSourceBlendMapAssetId  = cloneSourceAsset(sourceTerrainDefinition.getSourceBlendMap(), sourceMapGlobalAssetId, targetMapGlobalAssetId, entityId, assetEditHelper, targetAssetPackageName, assetNamePrefix + "_source_blend_map", "blend_map");
			qsf::GlobalAssetId globalSourceColorMapAssetId  = cloneSourceAsset(sourceTerrainDefinition.getSourceColorMap(), sourceMapGlobalAssetId, targetMapGlobalAssetId, entityId, assetEditHelper, targetAssetPackageName, assetNamePrefix + "_source_color_map", "color_map");
			qsf::GlobalAssetId globalSourceHeightMapAssetId = cloneSourceAsset(sourceTerrainDefinition.getSourceHeightMap(), sourceMapGlobalAssetId, targetMapGlobalAssetId, entityId, assetEditHelper, targetAssetPackageName, assetNamePrefix + "_source_height_map", "height_map");

			// Create derived textures
			qsf::GlobalAssetId globalColorMapAssetId  = cloneAsset(sourceTerrainDefinition.getColorMap(), assetEditHelper, targetAssetPackageName, assetNamePrefix + "_color_map");
			qsf::GlobalAssetId globalNormalMapAssetId = cloneAsset(sourceTerrainDefinition.getNormalMap(), assetEditHelper, targetAssetPackageName, assetNamePrefix + "_normal_map");

			// Create derived terrain chunk assets
			std::vector<std::vector<qsf::GlobalAssetId>> globalTerrainChunkAssetIds;
			globalTerrainChunkAssetIds.resize(terrainChunksPerEdge);
			for (uint32 x = 0; x < terrainChunksPerEdge; ++x)
			{
				globalTerrainChunkAssetIds[x].resize(terrainChunksPerEdge);
				for (uint32 y = 0; y < terrainChunksPerEdge; ++y)
				{
					std::stringstream assetNameSuffix;
					assetNameSuffix << std::setw(4) << std::setfill('0') << x << "_" << std::setw(4) << std::setfill('0') << y;
					const std::string assetName = assetNamePrefix + '_' + assetNameSuffix.str();

					globalTerrainChunkAssetIds[x][y] = cloneAsset(sourceTerrainDefinition.getOgreTerrainChunk(x, y), assetEditHelper, targetAssetPackageName, assetName);
				}
			}

			// Initialize terrain definition
			terrainDefinition.initialize(globalColorMapAssetId, globalNormalMapAssetId, globalSourceBlendMapAssetId, globalSourceColorMapAssetId, globalSourceHeightMapAssetId, globalTerrainChunkAssetIds);
		}


//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace qsf
{
	namespace editor
	{


		//[-------------------------------------------------------]
		//[ Public static methods                                 ]
		//[-------------------------------------------------------]
		GlobalAssetId TerrainEditHelper::cloneTerrainAssets(const TerrainDefinition& sourceTerrainDefinition, AssetEditHelper& assetEditHelper, TerrainDefinition& targetTerrainDefinition, const std::string& targetAssetPackageName)
		{
			Asset* terrainDefinitionAsset = assetEditHelper.addAsset(targetAssetPackageName, QsfAssetTypes::TERRAIN, "terrain", targetTerrainDefinition.getName());
			if (nullptr != terrainDefinitionAsset)
			{
				const GlobalAssetId globalTerrainAssetId = terrainDefinitionAsset->getGlobalAssetId();
				const uint32 terrainChunksPerEdge = sourceTerrainDefinition.getTerrainChunksPerEdge();

				::detail::cloneTerrainAssets(sourceTerrainDefinition, assetEditHelper, targetTerrainDefinition, targetAssetPackageName, terrainChunksPerEdge);

				targetTerrainDefinition.saveByFilename(assetEditHelper.getCachedAssetPath(globalTerrainAssetId, TerrainDefinition::FILE_EXTENSION));
				assetEditHelper.setAssetUploadData(globalTerrainAssetId, true, true);

				{ // Collect asset dependencies
					std::vector<GlobalAssetId> assetDependencies;

					// Temporary dependencies (strictly speaking these assets are both, dependencies and derived assets, but for now we just call all of them dependencies... later on we will just depend on the source assets)
					targetTerrainDefinition.getAllGlobalAssetIds(assetDependencies);

					terrainDefinitionAsset->setDependencies(assetDependencies);
				}

				// Done
				return terrainDefinitionAsset->getGlobalAssetId();
			}

			// Error!
			return getUninitialized<GlobalAssetId>();
		}


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainEditHelper::~TerrainEditHelper()
		{
			shutdown();
		}

		bool TerrainEditHelper::isGood() const
		{
			return mIsGood;
		}

		bool TerrainEditHelper::isReady() const
		{
			return mIsReady;
		}

		uint64 TerrainEditHelper::getEntityId() const
		{
			return mEntityId;
		}

		void TerrainEditHelper::callWhenReady(boost::function<void()> readyCallback)
		{
			if (!readyCallback.empty())
			{
				if (mIsReady)
				{
					readyCallback();
				}
				else
				{
					mReadyCallbacks.push_back(readyCallback);
				}
			}
		}

		void TerrainEditHelper::save(boost::function<void()> readyCallback)
		{
			// If there are no changes, there's no need to save anything
			if (!mModifiedOgreTerrains.empty())
			{
				// If there is no callback defined we might be in a fire-and-forget situation so let's make sure we survive the save in case it takes a little longer...
				if (readyCallback.empty())
				{
					// We'll utilize a fake function for this that will make sure that a shared pointer is kept alive until the terrain save is done
					readyCallback = boost::bind(&::detail::fakeReadyCallback, shared_from_this());
				}

				onTerrainBusy();
				callWhenReady(readyCallback);

				// We must make sure that no previous upload is still pending when attempting to save, thus let's introduce yet another callback
				if (nullptr != mAssetEditHelper)
				{
					mAssetEditHelper->callWhenFinishedUploading(boost::bind(&TerrainEditHelper::saveTerrain, shared_from_this(), boost::function<void(bool)>(boost::bind(&TerrainEditHelper::onSaveTerrainDone, shared_from_this(), _1))));
				}
			}
		}

		TerrainDataMap* TerrainEditHelper::getTerrainDataMap(TerrainDataType dataType, bool createIfNotExist)
		{
			QSF_CHECK(isReady(), "qsf::editor::TerrainEditHelper::getTerrainDataMap() called although terrain edit helper isn't ready yet", QSF_REACT_THROW);
			if (createIfNotExist)
			{
				mModifiedTerrainDataTypes.insert(dataType);
			}
			return mTerrainData.getTerrainDataMap(dataType, createIfNotExist);
		}

		void TerrainEditHelper::updateOgreMap(TerrainDataType dataType)
		{
			QSF_CHECK(isReady(), "qsf::editor::TerrainEditHelper::updateOgreMap() called although terrain edit helper isn't ready yet", QSF_REACT_THROW);
			mTerrainData.updateOgreMap(dataType);
			mModifiedTerrainDataTypes.insert(dataType);
		}

		void TerrainEditHelper::addToModifiedOgreTerrains(const std::unordered_set<Ogre::Terrain*>& modifiedOgreTerrains)
		{
			for (Ogre::Terrain* ogreTerrain : modifiedOgreTerrains)
			{
				mModifiedOgreTerrains.insert(ogreTerrain);
			}
		}

		void TerrainEditHelper::setOgreTerrainHeight(uint16 x, uint16 y, float value, uint16 segments, Ogre::TerrainGroup& ogreTerrainGroup, std::unordered_set<Ogre::Terrain*>& modifiedOgreTerrains)
		{
			QSF_CHECK(isReady(), "qsf::editor::TerrainEditHelper::setOgreTerrainHeight() called although terrain edit helper isn't ready yet", QSF_REACT_THROW);
			mTerrainData.setOgreTerrainHeight(x, y, value, segments, ogreTerrainGroup, modifiedOgreTerrains);
			addToModifiedOgreTerrains(modifiedOgreTerrains);
		}

		void TerrainEditHelper::importMap(TerrainDataType dataType, const std::string& absoluteFilename, bool scaleAndBias, float minimumValue, float maximumValue)
		{
			QSF_CHECK(isReady(), "qsf::editor::TerrainEditHelper::importMap() called although terrain edit helper isn't ready yet", QSF_REACT_THROW);
			mTerrainData.importMap(dataType, absoluteFilename, scaleAndBias, minimumValue, maximumValue);
			markAllOgreTerrainsAsModified();
			mModifiedTerrainDataTypes.insert(dataType);
		}

		void TerrainEditHelper::exportMap(TerrainDataType dataType, const std::string& absoluteFilename, bool normalize)
		{
			QSF_CHECK(isReady(), "qsf::editor::TerrainEditHelper::exportMap() called although terrain edit helper isn't ready yet", QSF_REACT_THROW);
			mTerrainData.exportMap(dataType, absoluteFilename, normalize);
		}

		void TerrainEditHelper::importChannel(TerrainDataType dataType, int channelIndex, const std::string& absoluteFilename, float heightMapScale)
		{
			QSF_CHECK(isReady(), "qsf::editor::TerrainEditHelper::importChannel() called although terrain edit helper isn't ready yet", QSF_REACT_THROW);
			mTerrainData.importChannel(dataType, channelIndex, absoluteFilename);
			markAllOgreTerrainsAsModified();
			mModifiedTerrainDataTypes.insert(dataType);
		}

		void TerrainEditHelper::exportChannel(TerrainDataType dataType, int channelIndex, const std::string& absoluteFilename)
		{
			QSF_CHECK(isReady(), "qsf::editor::TerrainEditHelper::exportChannel() called although terrain edit helper isn't ready yet", QSF_REACT_THROW);
			mTerrainData.exportChannel(dataType, channelIndex, absoluteFilename);
		}

		void TerrainEditHelper::markAllOgreTerrainsAsModified()
		{
			const TerrainComponent* terrainComponent = getTerrainComponent();
			if (nullptr != terrainComponent)
			{
				const uint32 terrainChunksPerEdge = terrainComponent->getTerrainChunksPerEdge();
				const Ogre::TerrainGroup* ogreTerrainGroup = terrainComponent->getOgreTerrainGroup();
				QSF_CHECK(nullptr != ogreTerrainGroup, "qsf::editor::TerrainEditHelper::saveTerrain(): There's no OGRE terrain group instance", return);
				for (uint32 x = 0; x < terrainChunksPerEdge; ++x)
				{
					for (uint32 y = 0; y < terrainChunksPerEdge; ++y)
					{
						mModifiedOgreTerrains.insert(ogreTerrainGroup->getTerrain(x, y));
					}
				}
			}
		}

		void TerrainEditHelper::clearWorkspace()
		{
			QSF_CHECK(isReady(), "qsf::editor::TerrainEditHelper::clearWorkspace() called although terrain edit helper isn't ready yet", QSF_REACT_THROW);
			mTerrainData.clearWorkspace();
		}


		//[-------------------------------------------------------]
		//[ Protected methods                                     ]
		//[-------------------------------------------------------]
		TerrainEditHelper::TerrainEditHelper(GlobalAssetId globalMapAssetId, uint64 entityId) :
			mGlobalMapAssetId(globalMapAssetId),
			mEntityId(entityId),
			mIsGood(true),
			mIsReady(false),
			mTerrainData(entityId),
			mAssetEditHelper(nullptr)
		{
			// Nothing to do in here
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		TerrainComponent* TerrainEditHelper::getTerrainComponent() const
		{
			TerrainComponent* result = nullptr;

			if (mGlobalMapAssetId == QSF_MAINMAP.getGlobalAssetId())
			{
				const Entity* entity = QSF_MAINMAP.getEntityById(mEntityId);
				if (nullptr != entity)
				{
					result = entity->getComponent<TerrainComponent>();
				}
			}
			else
			{
				QSF_ERROR("QSF terrain edit helper was created for a different map!", QSF_REACT_NONE);
			}

			return result;
		}

		void TerrainEditHelper::startup()
		{
			bool isGood = false;

			onTerrainBusy();

			// If mAssetEditHelper is initialized this terrain edit helper has already started up...
			if (nullptr == mAssetEditHelper)
			{
				const Map& map = QSF_MAINMAP;
				if (mGlobalMapAssetId == map.getGlobalAssetId())
				{
					TerrainComponent* terrainComponent = getTerrainComponent();
					if (nullptr != terrainComponent)
					{
						{ // Check whether or not the terrain component has valid settings
							// TODO(co) We might want to put this into a map check
							const Entity* entity = map.getEntityById(terrainComponent->getEntityId());
							if (nullptr != entity)
							{
								const TransformComponent* transformComponent = entity->getComponent<TransformComponent>();
								if (nullptr != transformComponent)
								{
									QSF_CHECK(::detail::epsilonEqual(transformComponent->getScale(), Math::GLM_VEC3_UNIT_XYZ, glm::epsilon<float>()),
										"Terrain entity " << terrainComponent->getEntityId() << " has a none uniform scale, this is not recommended", QSF_REACT_NONE);
								}
							}
						}

						TerrainDefinition* terrainDefinition = terrainComponent->getTerrainDefinition();
						if (nullptr != terrainDefinition)
						{
							if (terrainDefinition->getEntityId() == mEntityId && terrainDefinition->getGlobalMapAssetId() == mGlobalMapAssetId)
							{
								editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<editor::EntitySelectionManager>();
								if (!entitySelectionManager.isEntityLocked(mEntityId))
								{
									base::SelectEntityHiddenOperation* selectEntityHiddenOperation = entitySelectionManager.createSelectEntityHiddenOperation(mEntityId);
									if (nullptr != selectEntityHiddenOperation)
									{
										// Commit the operation
										QSF_EDITOR_OPERATION.push(selectEntityHiddenOperation, false);

										// This method will reset the terrain, since OGRE wouldn't let go of its file handles otherwise
										terrainComponent->setEditing(true);

										// We need to reload the whole terrain from source data, since the above call did reset the whole terrain
										mTerrainData.getTerrainDataMap(DATATYPE_BLENDMAP, true);
										mTerrainData.getTerrainDataMap(DATATYPE_COLORMAP, true);
										mTerrainData.getTerrainDataMap(DATATYPE_HEIGHTMAP, true);

										{
											AssetProxy mapAssetProxy(mGlobalMapAssetId);
											mAssetEditHelper = std::shared_ptr<AssetEditHelper>(new AssetEditHelper(mapAssetProxy.getProjectId()));
											QSF_CHECK(nullptr != mapAssetProxy.getAssetPackage(), "qsf::editor::TerrainEditHelper::startup(): nullptr != mapAssetProxy.getAssetPackage()", QSF_REACT_THROW);
											mTargetAssetPackageName = mapAssetProxy.getAssetPackage()->getName();
										}

										if (terrainComponent->getTerrainAsset().isValid() && terrainDefinition->isValid())
										{
											mAssetEditHelper->callWhenReady(boost::bind(&TerrainEditHelper::onTerrainReady, shared_from_this(), true));

											// Looks good so far
											isGood = true;
										}
										else
										{
											Asset* terrainDefinitionAsset = mAssetEditHelper->addAsset(mTargetAssetPackageName, QsfAssetTypes::TERRAIN, "terrain", terrainDefinition->getName());
											if (nullptr != terrainDefinitionAsset)
											{
												const GlobalAssetId globalTerrainAssetId = terrainDefinitionAsset->getGlobalAssetId();
												const uint32 terrainChunksPerEdge = terrainComponent->getTerrainChunksPerEdge();

												::detail::createTerrainAssets(*mAssetEditHelper, *terrainDefinition, mTargetAssetPackageName, terrainChunksPerEdge);

												const std::string terrainDefinitionFilename = mAssetEditHelper->getCachedAssetPath(globalTerrainAssetId, TerrainDefinition::FILE_EXTENSION);
												terrainDefinition->saveByFilename(terrainDefinitionFilename);
												mAssetEditHelper->setAssetUploadData(globalTerrainAssetId, true, true);

												{ // Collect asset dependencies
													std::vector<GlobalAssetId> assetDependencies;

													// Temporary dependencies (strictly speaking these assets are both, dependencies and derived assets, but for now we just call all of them dependencies... later on we will just depend on the source assets)
													terrainDefinition->getAllGlobalAssetIds(assetDependencies);

													terrainDefinitionAsset->setDependencies(assetDependencies);
												}

												// Make sure all maps are loaded so they will all get saved
												mTerrainData.getTerrainDataMap(DATATYPE_BLENDMAP, true);
												mTerrainData.getTerrainDataMap(DATATYPE_COLORMAP, true);
												mTerrainData.getTerrainDataMap(DATATYPE_HEIGHTMAP, true);

												markAllOgreTerrainsAsModified();
												saveTerrain(boost::bind(&TerrainEditHelper::onSaveTerrainDoneAfterCreation, shared_from_this(), globalTerrainAssetId, _1));

												// Looks good so far
												isGood = true;
											}
										}
									}
								}
							}
							else
							{
								QSF_ERROR("QSF terrain definition in the selected terrain component was created for a different map or terrain", QSF_REACT_NONE);
							}
						}
					}
				}
				else
				{
					QSF_ERROR("QSF terrain edit helper was created for a different map", QSF_REACT_NONE);
				}
			}

			// If we reach this point with isGood not set to true an error must have occurred and we should report failure right away...
			if (!isGood)
			{
				onTerrainReady(false);
			}
		}

		void TerrainEditHelper::shutdown()
		{
			if (nullptr != mAssetEditHelper)
			{
				// Disconnect from save related signals
				if (QSF_EDITOR_NETWORK_MANAGER.isLoggedIn())
				{
					// In online mode we listen for flush (=the commit operation)
					// Disconnect our slot from the Qt signal of the operation manager
					disconnect(&QSF_EDITOR_OPERATION, &OperationManager::redoOperationExecuted, this, &TerrainEditHelper::onRedoOperationExecuted);
				}
				else
				{
					// In offline mode we listen for map save
					// Disconnect our Boost slot from the Boost signal of the QSF map
					QSF_MAINMAP.PostMapSave.disconnect(boost::bind(&TerrainEditHelper::onPostMapSave, this));
				}
				mAssetEditHelper = nullptr;

				// Deselect the terrain entity
				base::DeselectEntityHiddenOperation* deselectEntityHiddenOperation = QSF_EDITOR_SELECTION_SYSTEM.getSafe<editor::EntitySelectionManager>().createDeselectEntityHiddenOperation(mEntityId);
				if (nullptr != deselectEntityHiddenOperation)
				{
					QSF_EDITOR_OPERATION.push(deselectEntityHiddenOperation, false);
				}

				// We're leaving terrain edit mode
				TerrainComponent* terrainComponent = getTerrainComponent();
				QSF_CHECK(nullptr != terrainComponent, "qsf::editor::TerrainEditHelper::shutdown(): nullptr != terrainComponent", QSF_REACT_THROW);
				terrainComponent->setEditing(false);
			}
		}

		void TerrainEditHelper::onTerrainBusy()
		{
			mIsReady = false;

			// TODO(ca) I'm perfectly aware that I'm mixing GUI and functionality here... My excuses are: There are plenty of other places all over the editor where this is happening aswell and there really just isn't the time for a clean solution right now...
			MainWindow* mainWindow = QSF_EDITOR_APPLICATION.getMainWindow();
			if (nullptr != mainWindow)
			{
				// TODO(ca) Show modal "editor busy"-dialog
				mainWindow->setEnabled(false);
			}
		}

		void TerrainEditHelper::onTerrainReady(bool isGood)
		{
			// "mAssetEditHelper" can be a null pointer if the terrain is currently already edited by another user
			if (nullptr != mAssetEditHelper && mAssetEditHelper->isGood() && isGood)
			{
				mIsGood = true;

				if (QSF_EDITOR_NETWORK_MANAGER.isLoggedIn())
				{
					// In online mode we listen for flush (=the commit operation)
					// Connect our slot to the Qt signal of the operation manager
					connect(&QSF_EDITOR_OPERATION, &OperationManager::redoOperationExecuted, this, &TerrainEditHelper::onRedoOperationExecuted);
				}
				else
				{
					// In offline mode we listen for map save
					// Connect our Boost slot to the Boost signal of the QSF map
					QSF_MAINMAP.PostMapSave.connect(boost::bind(&TerrainEditHelper::onPostMapSave, this));
				}
			}
			else
			{
				QSF_ERROR("QSF editor failed to start terrain editing. Maybe the related assets are locked?", QSF_REACT_NONE);
				mIsGood = false;
				shutdown();
			}

			mIsReady = true;

			{ // TODO(tl): Check if this helps or not, after the terrain is ready we should update all chunks and layer against
				qsf::editor::TerrainDataMap* blendTerrainDataMap = mTerrainData.getTerrainDataMap(DATATYPE_BLENDMAP, true);
				if (nullptr != blendTerrainDataMap)
				{
					blendTerrainDataMap->setDirtyAll();
					mTerrainData.updateOgreMap(DATATYPE_BLENDMAP);
				}
			}

			{ // TODO(ca) I'm perfectly aware that I'm mixing GUI and functionality here... My excuses are: There are plenty of other places all over the editor where this is happening aswell and there really just isn't the time for a clean solution right now...
				MainWindow* mainWindow = QSF_EDITOR_APPLICATION.getMainWindow();
				if (nullptr != mainWindow)
				{
					// TODO(ca) Hide modal "editor busy"-dialog
					mainWindow->setEnabled(true);
				}
			}

			const auto callbacks = mReadyCallbacks;
			mReadyCallbacks.clear();
			for (const auto& callback : callbacks)
			{
				callback();
			}
		}

		void TerrainEditHelper::onSaveTerrainDoneAfterCreation(GlobalAssetId globalTerrainAssetId, bool isGood)
		{
			TerrainComponent* terrainComponent = getTerrainComponent();

			QSF_CHECK(nullptr != terrainComponent, "qsf::editor::TerrainEditHelper::onSaveTerrainDoneAfterCreation(): nullptr != terrainComponent", QSF_REACT_THROW);
			QSF_CHECK(isInitialized(globalTerrainAssetId), "qsf::editor::TerrainEditHelper::onSaveTerrainDoneAfterCreation(): isInitialized(globalTerrainAssetId)", QSF_REACT_THROW);
			QSF_CHECK(nullptr != mAssetEditHelper, "qsf::editor::TerrainEditHelper::onSaveTerrainDoneAfterCreation(): nullptr != mAssetEditHelper", QSF_REACT_THROW);

			if (isGood && mAssetEditHelper->isGood())
			{
				// TODO(ca) This operation must not be undoable, but should invalidate the clean state of the main QUndoStack, which it, sadly, doesn't... :'(
				QSF_EDITOR_OPERATION.push(new base::SetComponentPropertyOperation(*terrainComponent, TerrainComponent::TERRAIN_ASSET, AssetProxy(globalTerrainAssetId)), false);

				// Using this workaround for now...
				base::CompoundOperation* compoundOperation = new base::CompoundOperation();
				compoundOperation->setText("Upgraded Terrain (undoing this operation will have no effect)");
				QSF_EDITOR_OPERATION.push(compoundOperation);
			}

			onSaveTerrainDone(isGood);
		}

		void TerrainEditHelper::onPostMapSave()
		{
			save();
		}

		void TerrainEditHelper::saveTerrain(boost::function<void(bool)> resultCallback)
		{
			QSF_CHECK(nullptr != mAssetEditHelper, "qsf::editor::TerrainEditHelper::saveTerrain(): nullptr != mAssetEditHelper", QSF_REACT_THROW);

			if (!mAssetEditHelper->isSubmit())
			{
				const TerrainComponent* terrainComponent = mTerrainData.getTerrainComponent();
				if (nullptr == terrainComponent)
				{
					return;
				}
				const Ogre::TerrainGroup* ogreTerrainGroup = terrainComponent->getOgreTerrainGroup();
				QSF_CHECK(nullptr != ogreTerrainGroup, "qsf::editor::TerrainEditHelper::saveTerrain(): There's no OGRE terrain group instance", return);

				{ // Ensure the generation of all derived data is done before saving the data
					// TODO(co) Apparently does not work for the global normal map? Had situations in which apparently not all terrain chunks had updated global normal maps.
					const int terrainChunksPerEdge = terrainComponent->getTerrainChunksPerEdge();
					for (int chunkX = 0; chunkX < terrainChunksPerEdge; ++chunkX)
					{
						for (int chunkY = 0; chunkY < terrainChunksPerEdge; ++chunkY)
						{
							Ogre::Terrain* ogreTerrain = ogreTerrainGroup->getTerrain(chunkX, chunkY);
							QSF_CHECK(nullptr != ogreTerrain, "qsf::editor::TerrainEditHelper::saveTerrain(): There's no OGRE terrain instance", continue);
							ogreTerrain->update(true);
							ogreTerrain->updateDerivedData(true, Ogre::Terrain::DERIVED_DATA_ALL);
						}
					}
				}

				// Save the modified source assets of the terrain
				for (TerrainDataType dataType : mModifiedTerrainDataTypes)
				{
					saveSourceData(dataType);
				}

				// Now save the modified derived assets of the terrain
				if (mModifiedTerrainDataTypes.end() != mModifiedTerrainDataTypes.find(DATATYPE_HEIGHTMAP))
				{
					saveNormalMap(*ogreTerrainGroup, *terrainComponent);
				}
				if (mModifiedTerrainDataTypes.end() != mModifiedTerrainDataTypes.find(DATATYPE_COLORMAP))
				{
					saveColorMap(*ogreTerrainGroup, *terrainComponent);
				}

				{ // Disable global color map so we can see the final color map which is also used during runtime
					const int terrainChunksPerEdge = terrainComponent->getTerrainChunksPerEdge();
					for (int chunkX = 0; chunkX < terrainChunksPerEdge; ++chunkX)
					{
						for (int chunkY = 0; chunkY < terrainChunksPerEdge; ++chunkY)
						{
							Ogre::Terrain* ogreTerrain = ogreTerrainGroup->getTerrain(chunkX, chunkY);
							QSF_CHECK(nullptr != ogreTerrain, "qsf::editor::TerrainEditHelper::saveTerrain(): There's no OGRE terrain instance", continue);
							if (ogreTerrain->getGlobalColourMapEnabled())
							{
								ogreTerrain->setGlobalColourMapEnabled(false);
								ogreTerrain->getMaterial();	// We need to call this method in order to trigger a material update
							}
						}
					}
				}

				// Save terrain chunks
				saveTerrainChunks(*ogreTerrainGroup, *terrainComponent);
				mModifiedTerrainDataTypes.clear();

				// Submit changed assets to the server / asset system
				mAssetEditHelper->submit(boost::bind(resultCallback, true));
			}
		}

		void TerrainEditHelper::onSaveTerrainDone(bool isGood)
		{
			QSF_CHECK(nullptr != mAssetEditHelper, "qsf::editor::TerrainEditHelper::onSaveTerrainDone(): nullptr != mAssetEditHelper", QSF_REACT_THROW);
			mAssetEditHelper->reset();
			onTerrainReady(isGood && mAssetEditHelper->isGood());
		}

		GlobalAssetId TerrainEditHelper::getSourceDataGlobalAssetId(TerrainDataType dataType) const
		{
			GlobalAssetId result = getUninitialized<GlobalAssetId>();

			const TerrainComponent* terrainComponent = mTerrainData.getTerrainComponent();
			QSF_CHECK(nullptr != terrainComponent, "qsf::editor::TerrainEditHelper::getSourceDataGlobalAssetId(): nullptr != terrainComponent", QSF_REACT_THROW);
			const TerrainDefinition* terrainDefinition = terrainComponent->getTerrainDefinition();
			QSF_CHECK(nullptr != terrainDefinition, "qsf::editor::TerrainEditHelper::getSourceDataGlobalAssetId(): nullptr != terrainDefinition", QSF_REACT_THROW);

			switch (dataType)
			{
				case DATATYPE_HEIGHTMAP:
					result = terrainDefinition->getSourceHeightMap();
					break;

				case DATATYPE_COLORMAP:
					result = terrainDefinition->getSourceColorMap();
					break;

				case DATATYPE_BLENDMAP:
					result = terrainDefinition->getSourceBlendMap();
					break;

				case DATATYPE_DECORATIONMAP:
					// TODO(ca) This isn't supported yet, I guess?
					break;
			}

			return result;
		}

		void TerrainEditHelper::saveSourceData(TerrainDataType dataType)
		{
			TerrainDataMap* terrainDataMap = mTerrainData.getTerrainDataMap(dataType, false);
			if (nullptr != terrainDataMap)
			{
				QSF_CHECK(nullptr != mAssetEditHelper, "qsf::editor::TerrainEditHelper::saveSourceData(): nullptr != mAssetEditHelper", QSF_REACT_THROW);

				const GlobalAssetId globalAssetId = getSourceDataGlobalAssetId(dataType);
				if (isInitialized(globalAssetId))
				{
					Asset* asset = mAssetEditHelper->tryEditAsset(globalAssetId, mTargetAssetPackageName);

					// If the asset does not exist for some reason (broken asset package?), we have to create it right now
					if (nullptr == asset)
					{
						QSF_ERROR("QSF editor failed to find the terrain source data \"" << TerrainDataMap::getTerrainDataTypeAsString(dataType) << "\" with the global asset ID " << globalAssetId << ". Trying to create a new asset with this global asset ID.", QSF_REACT_NONE);

						TerrainComponent* terrainComponent = getTerrainComponent();
						QSF_CHECK(nullptr != terrainComponent, "qsf::editor::TerrainEditHelper::saveSourceData(): nullptr != terrainComponent", QSF_REACT_THROW);

						const TerrainDefinition* terrainDefinition = terrainComponent->getTerrainDefinition();
						QSF_CHECK(nullptr != terrainDefinition, "qsf::editor::TerrainEditHelper::saveSourceData(): nullptr != terrainDefinition", QSF_REACT_THROW);

						// Get asset name prefix and postfix
						const std::string assetNamePrefix = '_' + terrainDefinition->getName();
						std::string assetNamePostfix;
						switch (dataType)
						{
							case DATATYPE_HEIGHTMAP:
								assetNamePostfix = "_source_height_map";
								break;

							case DATATYPE_COLORMAP:
								assetNamePostfix = "_source_color_map";
								break;

							case DATATYPE_BLENDMAP:
								assetNamePostfix = "_source_blend_map";
								break;

							default:
								QSF_ERROR("QSF editor failed to create the terrain source asset \"" << TerrainDataMap::getTerrainDataTypeAsString(dataType) << "\" with the global asset ID " << globalAssetId << ". Terrain data type not supported.", return);
						}

						// Try to add the asset
						asset = mAssetEditHelper->addAssetByGlobalAssetId(globalAssetId, mTargetAssetPackageName, QsfAssetTypes::TERRAIN_DATA_MAP, "_terrain", assetNamePrefix + assetNamePostfix, mAssetEditHelper->getIntermediateAssetPackage());
						QSF_CHECK(nullptr != asset, "QSF editor failed to create the terrain source asset \"" << TerrainDataMap::getTerrainDataTypeAsString(dataType) << "\" with the global asset ID " << globalAssetId, return);
					}

					// Get our actual target file name
					std::string fileExtension;
					switch (dataType)
					{
						case DATATYPE_HEIGHTMAP:
							fileExtension = "height_map";
							break;

						case DATATYPE_COLORMAP:
							fileExtension = "color_map";
							break;

						case DATATYPE_BLENDMAP:
							fileExtension = "blend_map";
							break;

						default:
							QSF_ERROR("QSF editor failed to save the terrain source asset \"" << TerrainDataMap::getTerrainDataTypeAsString(dataType) << "\" with the global asset ID " << globalAssetId << ". Terrain data type not supported.", return);
					}
					const std::string absoluteFilename = QSF_FILE.getBaseDirectory() + '/' + mAssetEditHelper->getCachedAssetPath(globalAssetId, fileExtension);

					// Ensure the parent directory does exist
					boost::filesystem::create_directories(boost::filesystem::path(absoluteFilename).parent_path());

					// Serialize the modified OGRE mesh
					terrainDataMap->saveMap(absoluteFilename);

					// Tell asset edit helper that the source asset has changes
					mAssetEditHelper->setAssetUploadData(globalAssetId, true, true);
				}
			}
		}

		void TerrainEditHelper::saveColorMap(const Ogre::TerrainGroup& ogreTerrainGroup, const TerrainComponent& terrainComponent)
		{
			// Color map
			TerrainDataMap* colorTerrainDataMap = mTerrainData.getTerrainDataMap(DATATYPE_COLORMAP, false);
			if (nullptr != colorTerrainDataMap)
			{
				QSF_CHECK(nullptr != mAssetEditHelper, "qsf::editor::TerrainEditHelper::saveColorMap(): nullptr != mAssetEditHelper", QSF_REACT_THROW);

				const TerrainDefinition* terrainDefinition = terrainComponent.getTerrainDefinition();
				QSF_CHECK(nullptr != terrainDefinition, "qsf::editor::TerrainEditHelper::saveColorMap(): nullptr != terrainDefinition", QSF_REACT_THROW);

				const GlobalAssetId globalAssetId = terrainDefinition->getColorMap();
				if (isInitialized(globalAssetId))
				{
					{ // Start asset edit - but be aware that it's possible that there's no cached asset, yet
						Asset* asset = mAssetEditHelper->tryEditAsset(globalAssetId, mTargetAssetPackageName);
						if (nullptr != asset)
						{
							// Make sure asset category and name are correct
							asset->setCategory("_terrain");
							asset->setName(std::string("_") + terrainDefinition->getName() + "_color_map");
						}
						else
						{
							const std::string assetNamePrefix = std::string("_") + terrainDefinition->getName();
							asset = mAssetEditHelper->addAssetByGlobalAssetId(globalAssetId, mTargetAssetPackageName, QsfAssetTypes::TEXTURE, "_terrain", assetNamePrefix + "_color_map", mAssetEditHelper->getIntermediateAssetPackage());
							QSF_CHECK(nullptr != asset, "qsf::editor::TerrainEditHelper::saveColorMap() failed to create the normal map texture asset", QSF_REACT_THROW);
						}
					}

					// Construct the filename where to save the intermediate color map
					const std::string absoluteFilename = QSF_FILE.getBaseDirectory() + '/' + "temp/terrain/" + terrainDefinition->getName() + "_color_map.tif";

					// Ensure the directory is there, then serialize, else serialize will fail if the directory is not there, yet
					boost::filesystem::create_directories(boost::filesystem::path(absoluteFilename).parent_path());

					// Export the color map
					colorTerrainDataMap->exportMap(absoluteFilename, false);

					// Configure our texture compiler
					base::TextureAssetCompiler textureAssetCompiler;
					base::TextureAssetCompilerConfig& config = textureAssetCompiler.getConfig();
					{ // Red
						base::TextureAssetCompilerConfig::ChannelInfo channelInfo;
						channelInfo.sourceChannel = 0;
						config.channelConfig.push_back(channelInfo);
					}
					{ // Green
						base::TextureAssetCompilerConfig::ChannelInfo channelInfo;
						channelInfo.sourceChannel = 1;
						config.channelConfig.push_back(channelInfo);
					}
					{ // Blue
						base::TextureAssetCompilerConfig::ChannelInfo channelInfo;
						channelInfo.sourceChannel = 2;
						config.channelConfig.push_back(channelInfo);
					}
					{ // Alpha
						base::TextureAssetCompilerConfig::ChannelInfo channelInfo;
						channelInfo.sourceChannel = 3;
						config.channelConfig.push_back(channelInfo);
					}
					config.compression = base::TextureAssetCompilerConfig::BC2;
					config.generateMipmaps = true;

					// Get our actual target file name
					const std::string absoluteCachedAssetDataFilename = QSF_FILE.getBaseDirectory() + '/' + mAssetEditHelper->getCachedAssetPath(globalAssetId, textureAssetCompiler.getFileExtension());

					// Compile the color map
					textureAssetCompiler.compile(absoluteFilename, absoluteCachedAssetDataFilename, *mAssetEditHelper->getCachedAsset(globalAssetId));

					// We don't need the intermediate color map anymore
					boost::filesystem::remove(absoluteFilename);

					mAssetEditHelper->setAssetUploadData(globalAssetId, true, true);
				}
			}
		}

		void TerrainEditHelper::saveNormalMap(const Ogre::TerrainGroup& ogreTerrainGroup, const TerrainComponent& terrainComponent)
		{
			// Normal map
			TerrainDataMap* heightTerrainDataMap = mTerrainData.getTerrainDataMap(DATATYPE_HEIGHTMAP, false);
			if (nullptr != heightTerrainDataMap)
			{
				// TODO(co) This is just a quick'n'dirty proof-of-concept. Refactore the code to not have so much code duplication.

				QSF_CHECK(nullptr != mAssetEditHelper, "qsf::editor::TerrainEditHelper::saveNormalMap(): nullptr != mAssetEditHelper", QSF_REACT_THROW);

				const TerrainDefinition* terrainDefinition = terrainComponent.getTerrainDefinition();
				QSF_CHECK(nullptr != terrainDefinition, "qsf::editor::TerrainEditHelper::saveNormalMap(): nullptr != terrainDefinition", QSF_REACT_THROW);

				const GlobalAssetId globalAssetId = terrainDefinition->getNormalMap();
				if (isInitialized(globalAssetId))
				{
					{ // Start asset edit - but be aware that it's possible that there's no cached asset, yet
						Asset* asset = mAssetEditHelper->tryEditAsset(globalAssetId, mTargetAssetPackageName);
						if (nullptr != asset)
						{
							// Make sure asset category and name are correct
							asset->setCategory("_terrain");
							asset->setName(std::string("_") + terrainDefinition->getName() + "_normal_map");
						}
						else
						{
							const std::string assetNamePrefix = std::string("_") + terrainDefinition->getName();
							asset = mAssetEditHelper->addAssetByGlobalAssetId(globalAssetId, mTargetAssetPackageName, QsfAssetTypes::TEXTURE, "_terrain", assetNamePrefix + "_normal_map", mAssetEditHelper->getIntermediateAssetPackage());
							QSF_CHECK(nullptr != asset, "qsf::editor::TerrainEditHelper::saveNormalMap() failed to create the normal map texture asset", QSF_REACT_THROW);
						}
					}

					const int terrainChunksPerEdge = terrainComponent.getTerrainChunksPerEdge();

					// Apparently not each chunk always has a valid normal map. So, go looking for the first chunk with a valid normal map.
					int normalMapSize = 0;
					for (int i = 0; i < terrainChunksPerEdge; ++i)
					{
						for (int j = 0; j < terrainChunksPerEdge; ++j)
						{
							Ogre::Terrain* ogreTerrain = ogreTerrainGroup.getTerrain(i, j);
							QSF_CHECK(nullptr != ogreTerrain, "qsf::editor::TerrainEditHelper::saveNormalMap(): There's no OGRE terrain instance", continue);
							Ogre::TexturePtr ogreTexturePtr = ogreTerrain->getTerrainNormalMap();
							if (!ogreTexturePtr.isNull())
							{
								normalMapSize = ogreTexturePtr->getWidth();

								// Get us out of the loops
								i = j = terrainChunksPerEdge;
							}
						}
					}
					if (normalMapSize > 0)
					{
						// Construct the filename where to save the intermediate normal map
						const std::string absoluteFilename = QSF_FILE.getBaseDirectory() + '/' + "temp/terrain/" + terrainDefinition->getName() + "_normal_map.tif";

						// Ensure the directory is there, then serialize, else serialize will fail if the directory is not there, yet
						boost::filesystem::create_directories(boost::filesystem::path(absoluteFilename).parent_path());

						normalMapSize *= terrainChunksPerEdge;

						uint8* buffer = new uint8[normalMapSize * normalMapSize * 3];
						Ogre::Image ogreImage;
						ogreImage.loadDynamicImage(buffer, normalMapSize, normalMapSize, Ogre::PixelFormat::PF_R8G8B8);

						for (int chunkX = 0; chunkX < terrainChunksPerEdge; ++chunkX)
						{
							for (int chunkY = 0; chunkY < terrainChunksPerEdge; ++chunkY)
							{
								Ogre::Terrain* ogreTerrain = ogreTerrainGroup.getTerrain(chunkX, chunkY);
								QSF_CHECK(nullptr != ogreTerrain, "qsf::editor::TerrainEditHelper::saveNormalMap(): There's no OGRE terrain instance", continue);

								Ogre::Image ogreImageChunk;
								ogreTerrain->getTerrainNormalMap()->convertToImage(ogreImageChunk);
								ogreImageChunk.flipAroundX();

								int imageSize = ogreImageChunk.getWidth();
								for (uint16 chunkImageY = 0; chunkImageY < imageSize; ++chunkImageY)
								{
									for (uint16 chunkImageX = 0; chunkImageX < imageSize; ++chunkImageX)
									{
										const Ogre::ColourValue ogreColorValue = ogreImageChunk.getColourAt(chunkImageX, chunkImageY, 0);
										ogreImage.setColourAt(ogreColorValue, chunkX*imageSize + chunkImageX, chunkY*imageSize + chunkImageY, 0);
									}
								}
							}
						}

						// Save the image
						ogreImage.flipAroundX();
						ogreImage.save(absoluteFilename);

						// Cleanup
						delete [] buffer;

						// Configure our texture compiler
						base::TextureAssetCompiler textureAssetCompiler;
						base::TextureAssetCompilerConfig& config = textureAssetCompiler.getConfig();
						{ // Red
							base::TextureAssetCompilerConfig::ChannelInfo channelInfo;
							channelInfo.sourceChannel = 0;
							config.channelConfig.push_back(channelInfo);
						}
						{ // Green
							base::TextureAssetCompilerConfig::ChannelInfo channelInfo;
							channelInfo.sourceChannel = 1;
							config.channelConfig.push_back(channelInfo);
						}
						{ // Blue
							base::TextureAssetCompilerConfig::ChannelInfo channelInfo;
							channelInfo.sourceChannel = 2;
							config.channelConfig.push_back(channelInfo);
						}
						{ // Alpha
							base::TextureAssetCompilerConfig::ChannelInfo channelInfo;
							channelInfo.sourceChannel = 3;
							config.channelConfig.push_back(channelInfo);
						}
						config.compression = base::TextureAssetCompilerConfig::BC2;	// TODO(co) base::TextureAssetCompilerConfig::BC1 appears to be broken
						config.generateMipmaps = true;

						// Get our actual target file name
						const std::string absoluteCachedAssetDataFilename = QSF_FILE.getBaseDirectory() + '/' + mAssetEditHelper->getCachedAssetPath(globalAssetId, textureAssetCompiler.getFileExtension());

						// Compile the normal map
						textureAssetCompiler.compile(absoluteFilename, absoluteCachedAssetDataFilename, *mAssetEditHelper->getCachedAsset(globalAssetId));

						// We don't need the intermediate color map anymore
						boost::filesystem::remove(absoluteFilename);

						mAssetEditHelper->setAssetUploadData(globalAssetId, true, true);
					}
				}
			}
		}

		void TerrainEditHelper::saveTerrainChunks(const Ogre::TerrainGroup& ogreTerrainGroup, const TerrainComponent& terrainComponent)
		{
			TerrainDataMap* heightTerrainDataMap = mTerrainData.getTerrainDataMap(DATATYPE_HEIGHTMAP, false);
			TerrainDataMap* blendTerrainDataMap  = mTerrainData.getTerrainDataMap(DATATYPE_BLENDMAP, false);

			if (nullptr != heightTerrainDataMap || nullptr != blendTerrainDataMap)
			{
				// Because at the moment height map and blend maps are saved into dat files, we need to have both to work correctly
				// TODO(co) This looks odd: Variables are unused and the method calls look nearly the same as the one a few lines above. Please review.
				heightTerrainDataMap = mTerrainData.getTerrainDataMap(DATATYPE_HEIGHTMAP, true);
				blendTerrainDataMap  = mTerrainData.getTerrainDataMap(DATATYPE_BLENDMAP, true);

				// Save OGRE terrain
				try
				{
					QSF_CHECK(nullptr != mAssetEditHelper, "qsf::editor::TerrainEditHelper::saveTerrainChunks(): nullptr != mAssetEditHelper", QSF_REACT_THROW);

					const TerrainDefinition* terrainDefinition = terrainComponent.getTerrainDefinition();
					QSF_CHECK(nullptr != terrainDefinition, "qsf::editor::TerrainEditHelper::saveTerrainChunks(): nullptr != terrainDefinition", QSF_REACT_THROW);

					TerrainDataMap* colorTerrainDataMap = mTerrainData.getTerrainDataMap(DATATYPE_COLORMAP, false);
					const std::string assetNamePrefix = std::string("_") + terrainDefinition->getName();

					const int terrainChunksPerEdge = terrainComponent.getTerrainChunksPerEdge();
					for (int x = 0; x < terrainChunksPerEdge; ++x)
					{
						for (int y = 0; y < terrainChunksPerEdge; ++y)
						{
							Ogre::Terrain* ogreTerrain = ogreTerrainGroup.getTerrain(x, y);
							QSF_CHECK(nullptr != ogreTerrain, "qsf::editor::TerrainEditHelper::saveTerrainChunks(): There's no OGRE terrain instance", continue);

							const GlobalAssetId globalAssetId = terrainDefinition->getOgreTerrainChunk(x, y);
							if (isInitialized(globalAssetId) && mModifiedOgreTerrains.find(ogreTerrain) != mModifiedOgreTerrains.cend())
							{
								{ // Start asset edit - but be aware that it's possible that there's no cached asset, yet
									Asset* asset = mAssetEditHelper->tryEditAsset(globalAssetId, mTargetAssetPackageName);
									if (nullptr != asset)
									{
										// Make sure asset category and name are correct
										std::stringstream assetNameSuffix;
										assetNameSuffix << std::setw(4) << std::setfill('0') << x << "_" << std::setw(4) << std::setfill('0') << y;
										const std::string assetName = assetNamePrefix + '_' + assetNameSuffix.str();

										asset->setCategory("_terrain");
										asset->setName(assetName);
									}
								}

								// Get our actual target file name
								const std::string absoluteCachedAssetDataFilename = QSF_FILE.getBaseDirectory() + '/' + mAssetEditHelper->getCachedAssetPath(globalAssetId, "chunk");

								// Ensure the directory is there, then serialize, else serialize will fail if the directory is not there, yet
								boost::filesystem::create_directories(boost::filesystem::path(absoluteCachedAssetDataFilename).parent_path());

								// Temporarily disable stuff we don't want inside the dat-terrain
								const bool globalColourMapEnabled = ogreTerrain->getGlobalColourMapEnabled();
								{
									if (nullptr != colorTerrainDataMap && globalColourMapEnabled)
									{
										// Please don't store a per terrain chunk global color map inside the OGRE terrain, it's inefficient because it's raw texture data
										ogreTerrain->setGlobalColourMapEnabled(false);
									}

									// Please don't store a per terrain chunk global normal map and light map inside the OGRE terrain, it's inefficient because it's raw texture data
									// TODO(co)
									ogreTerrain->_setNormalMapRequired(false);
								}

								// Save OGRE terrain
								ogreTerrain->save(absoluteCachedAssetDataFilename);

								mAssetEditHelper->setAssetUploadData(globalAssetId, true, true);

								// Reenable stuff we temporarily disabled while saving the OGRE terrain (else OGRE would have written certain stuff which we don't want to have inside the dat-terrain)
								ogreTerrain->_setNormalMapRequired(true);
								if (nullptr != colorTerrainDataMap && globalColourMapEnabled)
								{
									ogreTerrain->setGlobalColourMapEnabled(globalColourMapEnabled);
								}
							}
						}
					}

					mModifiedOgreTerrains.clear();

					// Reenable stuff we temporarily disabled while saving the OGRE terrain (else OGRE would have written certain stuff which we don't want to have inside the dat-terrain)
					if (nullptr != colorTerrainDataMap)
					{
						colorTerrainDataMap->setDirtyAll();
						mTerrainData.updateOgreTerrainColorMap();
					}
				}
				catch (const std::exception& e)
				{
					QSF_ERROR("QSF failed to save the OGRE terrain. Exception caught: " << e.what(), QSF_REACT_NONE);
					mIsGood = false;
				}
			}
		}


		//[-------------------------------------------------------]
		//[ Private Qt Slots                                      ]
		//[-------------------------------------------------------]
		void TerrainEditHelper::onRedoOperationExecuted(const base::Operation& operation)
		{
			if (base::CommitOperation::OPERATION_ID == operation.getId())
			{
				save();
			}
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // qsf
