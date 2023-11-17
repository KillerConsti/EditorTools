// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool\kc_terrain\TerrainDefinition.h>
#include "qsf/asset/AssetProxy.h"
#include "qsf/base/BoostPtreeHelper.h"
#include "qsf/file/FileSystem.h"
#include "qsf/file/FileHelper.h"
#include "qsf/QsfHelper.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace kc_terrain
{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	const char	 TerrainDefinition::FILE_EXTENSION[] = "json";
	const char	 TerrainDefinition::FORMAT_TYPE[]	 = "QsfTerrain";
	const uint32 TerrainDefinition::FORMAT_VERSION	 = 1;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	TerrainDefinition::TerrainDefinition(qsf::GlobalAssetId globalMapAssetId, uint64 entityId) :
		mGlobalMapAssetId(globalMapAssetId),
		mEntityId(entityId),
		mColorMap(qsf::getUninitialized<qsf::GlobalAssetId>()),
		mNormalMap(qsf::getUninitialized<qsf::GlobalAssetId>()),
		mSourceBlendMap(qsf::getUninitialized<qsf::GlobalAssetId>()),
		mSourceColorMap(qsf::getUninitialized<qsf::GlobalAssetId>()),
		mSourceHeightMap(qsf::getUninitialized<qsf::GlobalAssetId>()),
		mTerrainChunksPerEdge(qsf::getUninitialized<uint32>())
	{
		// Nothing to do in here
	}

	TerrainDefinition::TerrainDefinition(const TerrainDefinition& terrainDefinition) :
		mGlobalMapAssetId(terrainDefinition.mGlobalMapAssetId),
		mEntityId(terrainDefinition.mEntityId),
		mColorMap(terrainDefinition.mColorMap),
		mNormalMap(terrainDefinition.mNormalMap),
		mSourceBlendMap(terrainDefinition.mSourceBlendMap),
		mSourceColorMap(terrainDefinition.mSourceColorMap),
		mSourceHeightMap(terrainDefinition.mSourceHeightMap),
		mTerrainChunksPerEdge(terrainDefinition.mTerrainChunksPerEdge),
		mOgreTerrainChunks(terrainDefinition.mOgreTerrainChunks)
	{
		// Nothing to do in here
	}

	qsf::GlobalAssetId TerrainDefinition::getGlobalMapAssetId() const
	{
		return mGlobalMapAssetId;
	}

	uint64 TerrainDefinition::getEntityId() const
	{
		return mEntityId;
	}

	std::string TerrainDefinition::getName() const
	{
		const std::string globalMapAssetIdAsString = std::to_string(mGlobalMapAssetId);
		const std::string entityIdAsString = std::to_string(mEntityId);
		return std::string("map_") + globalMapAssetIdAsString + "_entity_" + entityIdAsString;
	}

	bool TerrainDefinition::isValid() const
	{
		// These are all initialized at the same time so we only need to check for one...
		return qsf::isInitialized(mColorMap); // && isInitialized(mNormalMap) && isInitialized(mSourceBlendMap) && isInitialized(mSourceColorMap) && isInitialized(mSourceHeightMap) && isInitialized(mTerrainChunksPerEdge);
	}

	qsf::GlobalAssetId TerrainDefinition::getColorMap() const
	{
		return mColorMap;
	}

	qsf::GlobalAssetId TerrainDefinition::getNormalMap() const
	{
		return mNormalMap;
	}

	qsf::GlobalAssetId TerrainDefinition::getSourceBlendMap() const
	{
		return mSourceBlendMap;
	}

	qsf::GlobalAssetId TerrainDefinition::getSourceColorMap() const
	{
		return mSourceColorMap;
	}

	qsf::GlobalAssetId TerrainDefinition::getSourceHeightMap() const
	{
		return mSourceHeightMap;
	}

	uint32 TerrainDefinition::getTerrainChunksPerEdge() const
	{
		return mTerrainChunksPerEdge;
	}

	qsf::GlobalAssetId TerrainDefinition::getOgreTerrainChunk(uint32 x, uint32 y) const
	{
		qsf::GlobalAssetId result = qsf::getUninitialized<qsf::GlobalAssetId>();

		if (isValid() && x < mTerrainChunksPerEdge && y < mTerrainChunksPerEdge)
		{
			result = mOgreTerrainChunks[x][y];
		}

		return result;
	}

	void TerrainDefinition::getAllGlobalAssetIds(std::vector<qsf::GlobalAssetId>& globalAssetIds) const
	{
		getSourceGlobalAssetIds(globalAssetIds);
		getDerivedGlobalAssetIds(globalAssetIds);
	}

	void TerrainDefinition::getSourceGlobalAssetIds(std::vector<qsf::GlobalAssetId>& globalAssetIds) const
	{
		if (isValid())
		{
			globalAssetIds.push_back(getSourceHeightMap());
			globalAssetIds.push_back(getSourceColorMap());
			globalAssetIds.push_back(getSourceBlendMap());
		}
	}

	void TerrainDefinition::getDerivedGlobalAssetIds(std::vector<qsf::GlobalAssetId>& globalAssetIds) const
	{
		if (isValid())
		{
			globalAssetIds.push_back(getColorMap());
			globalAssetIds.push_back(getNormalMap());

			for (uint32 x = 0; x < mTerrainChunksPerEdge; ++x)
			{
				for (uint32 y = 0; y < mTerrainChunksPerEdge; ++y)
				{
					globalAssetIds.push_back(getOgreTerrainChunk(x, y));
				}
			}
		}
	}

	void TerrainDefinition::clear()
	{
		qsf::setUninitialized(mColorMap);
		qsf::setUninitialized(mNormalMap);

		qsf::setUninitialized(mSourceBlendMap);
		qsf::setUninitialized(mSourceColorMap);
		qsf::setUninitialized(mSourceHeightMap);

		qsf::setUninitialized(mTerrainChunksPerEdge);
		mOgreTerrainChunks.clear();
	}

	void TerrainDefinition::initialize(qsf::GlobalAssetId globalColorMapAssetId, qsf::GlobalAssetId globalNormalMapAssetId, qsf::GlobalAssetId globalSourceBlendMapAssetId, qsf::GlobalAssetId globalSourceColorMapAssetId, qsf::GlobalAssetId globalSourceHeightMapAssetId, const std::vector<std::vector<qsf::GlobalAssetId>>& globalTerrainChunkAssetIds)
	{
		clear();

		const uint32 terrainChunksPerEdge = static_cast<uint32>(globalTerrainChunkAssetIds.size());

		for (const auto& globalTerrainChunkAssetIdsRow : globalTerrainChunkAssetIds)
		{
			QSF_CHECK(globalTerrainChunkAssetIdsRow.size() == terrainChunksPerEdge, "QSF TerrainDefinition::initialize() called with invalid parameters", return);
		}

		mTerrainChunksPerEdge = terrainChunksPerEdge;

		mColorMap = globalColorMapAssetId;
		mNormalMap = globalNormalMapAssetId;

		mSourceBlendMap = globalSourceBlendMapAssetId;
		mSourceColorMap = globalSourceColorMapAssetId;
		mSourceHeightMap = globalSourceHeightMapAssetId;

		mOgreTerrainChunks = globalTerrainChunkAssetIds;
	}

	bool TerrainDefinition::loadFromAsset(qsf::GlobalAssetId globalAssetId)
	{
		bool result = false;

		clear();

		// The tree we're going to load
		boost::property_tree::ptree rootPTree;

		try
		{
			const std::string absoluteFilename = qsf::AssetProxy(globalAssetId).getAbsoluteCachedAssetDataFilename();
			{
				boost::nowide::ifstream ifs(absoluteFilename);
				qsf::FileHelper::readJson(ifs, rootPTree);
			}

			// TODO(co) Use "qsf::FileHelper::validateJsonFormatHeader()" and ensure the code is still working correctly
			// Check the file format type and version
			const boost::property_tree::ptree& formatPTree = rootPTree.get_child("Format");

			// Format type
			const std::string formatType = formatPTree.get<std::string>("Type");
			if (FORMAT_TYPE == formatType)
			{
				// Format version
				uint32 formatVersion = formatPTree.get<uint32>("Version");
				if (FORMAT_VERSION == formatVersion)
				{
					// Load properties
					const boost::property_tree::ptree& propertiesPTree = rootPTree.get_child("Properties");

					const qsf::GlobalAssetId globalMapAssetId = propertiesPTree.get<qsf::GlobalAssetId>("GlobalMapAssetId");
					if (globalMapAssetId != mGlobalMapAssetId)
					{
						QSF_WARN("QSF is loading a terrain definition for the map with global asset ID \"" << mGlobalMapAssetId << "\", but the terrain definition was stored for a map with global asset ID \"" << globalMapAssetId << "\"!", QSF_REACT_THROW);
					}

					const uint64 entityId = propertiesPTree.get<uint64>("EntityId");
					if (entityId != mEntityId)
					{
						QSF_WARN("QSF is loading the terrain definition for the entity with id \"" << mEntityId << "\", but the terrain definition was stored for an entity with id \"" << entityId << "\"!", QSF_REACT_THROW);
					}

					mColorMap = propertiesPTree.get<qsf::GlobalAssetId>("ColorMap");
					mNormalMap = propertiesPTree.get<qsf::GlobalAssetId>("NormalMap");

					mSourceBlendMap = propertiesPTree.get<qsf::GlobalAssetId>("SourceBlendMap");
					mSourceColorMap = propertiesPTree.get<qsf::GlobalAssetId>("SourceColorMap");
					mSourceHeightMap = propertiesPTree.get<qsf::GlobalAssetId>("SourceHeightMap");

					mTerrainChunksPerEdge = propertiesPTree.get<uint32>("TerrainChunksPerEdge");

					{ // Load ogre terrain chunks
						const boost::property_tree::ptree& ogreTerrainChunksPTree = propertiesPTree.get_child("OgreTerrainChunks");

						mOgreTerrainChunks.clear();
						mOgreTerrainChunks.resize(mTerrainChunksPerEdge);
						for (uint32 x = 0; x < mTerrainChunksPerEdge; ++x)
						{
							mOgreTerrainChunks[x].resize(mTerrainChunksPerEdge);
							for (uint32 y = 0; y < mTerrainChunksPerEdge; ++y)
							{
								const std::string key = std::to_string(x) + ',' + std::to_string(y);
								mOgreTerrainChunks[x][y] = ogreTerrainChunksPTree.get<qsf::GlobalAssetId>(key);
							}
						}
					}

					result = true;
				}
				else
				{
					// Error! Invalid format version!
					QSF_ERROR("QSF failed to load the terrain definition: Invalid format version, found \"" << formatVersion << "\" but must be \"" << FORMAT_VERSION << '\"', QSF_REACT_THROW);
				}
			}
			else
			{
				// Error! Invalid format type!
				QSF_ERROR("QSF failed to load the terrain definition: Invalid format type, found \"" << formatType << "\" but must be \"" << FORMAT_TYPE << '\"', QSF_REACT_THROW);
			}
		}
		catch (const std::exception& e)
		{
			QSF_ERROR("QSF failed to load the terrain definition from asset with global asset ID \"" << globalAssetId << "\": " << e.what(), QSF_REACT_NONE);
		}

		return result;
	}

	void TerrainDefinition::saveByFilename(const std::string& filename)
	{
		if (isValid())
		{
			// The tree we're going to save as JSON
			boost::property_tree::ptree rootPTree;

			{ // First of all: Format type and version
				boost::property_tree::ptree formatPTree;
				formatPTree.put("Type",     FORMAT_TYPE);
				formatPTree.put("Version",  FORMAT_VERSION);
				rootPTree.add_child("Format", formatPTree);
			}

			{ // Serialize the asset package properties to the Boost ptree
				boost::property_tree::ptree propertiesPTree;

				propertiesPTree.put("GlobalMapAssetId", mGlobalMapAssetId);
				propertiesPTree.put("EntityId", mEntityId);

				propertiesPTree.put("ColorMap", mColorMap);
				propertiesPTree.put("NormalMap", mNormalMap);

				propertiesPTree.put("SourceBlendMap", mSourceBlendMap);
				propertiesPTree.put("SourceColorMap", mSourceColorMap);
				propertiesPTree.put("SourceHeightMap", mSourceHeightMap);

				propertiesPTree.put("TerrainChunksPerEdge", mTerrainChunksPerEdge);

				{ // Store ogre terrain chunks
					boost::property_tree::ptree ogreTerrainChunksPTree;

					for (uint32 x = 0; x < mTerrainChunksPerEdge; ++x)
					{
						for (uint32 y = 0; y < mTerrainChunksPerEdge; ++y)
						{
							const std::string key = std::to_string(x) + ',' + std::to_string(y);
							ogreTerrainChunksPTree.put(key, mOgreTerrainChunks[x][y]);
						}
					}

					propertiesPTree.add_child("OgreTerrainChunks", ogreTerrainChunksPTree);
				}

				rootPTree.add_child("Properties", propertiesPTree);
			}

			// Write to disk
			try
			{
				const boost::filesystem::path absoluteFilename = QSF_FILE.getBaseDirectory() + '/' + filename;

				boost::filesystem::create_directories(absoluteFilename.parent_path());

				boost::nowide::ofstream ofs(absoluteFilename.generic_string());
				qsf::FileHelper::writeJson(ofs, rootPTree);
			}
			catch (const std::exception& e)
			{
				QSF_ERROR("QSF failed to store the terrain definition to the file \"" << filename << "\": " << e.what(), QSF_REACT_NONE);
			}
		}
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf
