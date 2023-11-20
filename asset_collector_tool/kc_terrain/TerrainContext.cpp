// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool\kc_terrain\TerrainContext.h>
#include <asset_collector_tool\kc_terrain\TerrainMaterialGenerator.h>
#include "qsf/log/LogSystem.h"

#include <OGRE/Terrain/OgreTerrain.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace kc_terrain
{


	//[-------------------------------------------------------]
	//[ Private static data                                   ]
	//[-------------------------------------------------------]
	uint32						TerrainContext::mContextCounter  = 0;
	Ogre::TerrainGlobalOptions* TerrainContext::mTerrainGlobals = nullptr;


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	void TerrainContext::addContextReference(uint64 ColorMap)
	{
		// Check context
		if (!mContextCounter)
		{
			QSF_LOG_PRINTS(INFO, "QSF OGRE global terrain options initialization");
			mTerrainGlobals = new Ogre::TerrainGlobalOptions();

			{ // Terrain material generator
				Ogre::TerrainMaterialGeneratorPtr terrainMaterialGeneratorPtr;
				mGenerator = OGRE_NEW TerrainMaterialGenerator(ColorMap);
				terrainMaterialGeneratorPtr.bind(mGenerator);
				mTerrainGlobals->setDefaultMaterialGenerator(terrainMaterialGeneratorPtr);
				mTerrainGlobals->setQueryFlags(0);	// Don't allow it to e.g. pick the terrain
			}
		}
		++mContextCounter;
	}

	void TerrainContext::releaseContextReference()
	{
		// Check context
		QSF_ASSERT(mContextCounter > 0, "QSF: More terrain context references are released as where added in the first place", QSF_REACT_NONE);
		--mContextCounter;
		if (0 == mContextCounter)
		{
			QSF_LOG_PRINTS(INFO, "QSF OGRE global terrain options de-initialization");
			delete mTerrainGlobals;
			mTerrainGlobals = nullptr;
			//mGenerator;
		}
	}

	 kc_terrain::TerrainMaterialGenerator* TerrainContext::GetMaterialGenerator()
	{
		return mGenerator;
	}
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf
