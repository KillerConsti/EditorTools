// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]

#include <asset_collector_tool\kc_terrain\TerrainComponent.h>



//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace kc_terrain
{


	//[-------------------------------------------------------]
	//[ Classes                                               ]
	//[-------------------------------------------------------]
	/**
	a helper class to load our own terrain 
	so we might make it more readable :D
	*/
	class TerrainScaler
	{
	public:
	//pls perform a reload after scaling

	//you have to validate 2^n+1
	static bool ScaleHeightMap(int NewSize,kc_terrain::TerrainComponent* TC);

	//shall be 2^n for LOD
	static bool ScaleColorMap(int NewSize, kc_terrain::TerrainComponent* TC);
	static bool ScaleTerrainTextures(int NewSize, kc_terrain::TerrainComponent* TC);

	//this gets more complicated <-> changing chunk values
	static bool IncreaseChunks(int NewSize, kc_terrain::TerrainComponent* TC);
	static bool DecreaseChunks(int NewSize, kc_terrain::TerrainComponent* TC);

	struct LayerData
	{
		std::string BaseLayer = "";
		std::vector<std::pair<std::string, std::vector<glm::vec3>>> BlendMaps;
		int xTerrain;
		int yTerrain;
	};

	static LayerData ScanLayers(Ogre::Terrain* OI,int PixelsPerSite);
	//stolen :D
	static LayerData MixLayers(LayerData L1,LayerData L2,int OffsetX,int OffsetY,int ChunkSize);
	static bool WriteNewChunkData(std::vector<LayerData> ResultData, kc_terrain::TerrainComponent* TC,int NumberOfParts);
	//-1 not included
	//0 BaseLayer 
	//1-5 higher layers
	static int IsIncluded(LayerData SearchLayers, std::string TextureName);
	static void UpdateAsset(qsf::GlobalAssetId uid);


	
	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf




