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
	class TerrainLoader
	{
	public:
	static int LoadTerrain(kc_terrain::TerrainComponent* TC);
	static bool LoadTerrainTextures(kc_terrain::TerrainComponent* TC);
	static bool LoadTerrainHeigtMap(kc_terrain::TerrainComponent* TC);
	static bool LoadColorMaps(kc_terrain::TerrainComponent* TC); 

	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf




