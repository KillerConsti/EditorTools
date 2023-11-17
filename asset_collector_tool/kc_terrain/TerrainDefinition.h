// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "qsf/Export.h"
#include "qsf/asset/AssetSystemTypes.h"


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace kc_terrain
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	*  @brief
	*    Terrain definition container/handler
	*/
	class TerrainDefinition
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static const char	FORMAT_TYPE[];
		static const uint32 FORMAT_VERSION;
		static const char	FILE_EXTENSION[];


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		TerrainDefinition(qsf::GlobalAssetId globalMapAssetId, uint64 entityId);
		TerrainDefinition(const TerrainDefinition& terrainDefinition);

		qsf::GlobalAssetId getGlobalMapAssetId() const;
		uint64 getEntityId() const;

		std::string getName() const;

		bool isValid() const;

		qsf::GlobalAssetId getColorMap() const;
		qsf::GlobalAssetId getNormalMap() const;

		qsf::GlobalAssetId getSourceBlendMap() const;
		qsf::GlobalAssetId getSourceColorMap() const;
		qsf::GlobalAssetId getSourceHeightMap() const;

		uint32 getTerrainChunksPerEdge() const;
		qsf::GlobalAssetId getOgreTerrainChunk(uint32 x, uint32 y) const;

		void getAllGlobalAssetIds(std::vector<qsf::GlobalAssetId>& globalAssetIds) const;
		void getSourceGlobalAssetIds(std::vector<qsf::GlobalAssetId>& globalAssetIds) const;
		void getDerivedGlobalAssetIds(std::vector<qsf::GlobalAssetId>& globalAssetIds) const;

		void clear();

		void initialize(qsf::GlobalAssetId globalColorMapAssetId, qsf::GlobalAssetId globalNormalMapAssetId, qsf::GlobalAssetId globalSourceBlendMapAssetId, qsf::GlobalAssetId globalSourceColorMapAssetId, qsf::GlobalAssetId globalSourceHeightMapAssetId, const std::vector<std::vector<qsf::GlobalAssetId>>& globalTerrainChunkAssetIds);
		bool loadFromAsset(qsf::GlobalAssetId globalTerrainAssetId);

		void saveByFilename(const std::string& filename);


	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		qsf::GlobalAssetId mGlobalMapAssetId;
		uint64 mEntityId;

		qsf::GlobalAssetId mColorMap;
		qsf::GlobalAssetId mNormalMap;

		qsf::GlobalAssetId mSourceBlendMap;
		qsf::GlobalAssetId mSourceColorMap;
		qsf::GlobalAssetId mSourceHeightMap;

		uint32 mTerrainChunksPerEdge;
		std::vector<std::vector<qsf::GlobalAssetId>> mOgreTerrainChunks;


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf
