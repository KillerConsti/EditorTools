// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "qsf/base/Context.h"
#include <asset_collector_tool\kc_terrain\TerrainMaterialGenerator.h>

//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ogre
{
	class TerrainGlobalOptions;
}


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
	*    Static terrain context context
	*/
	class TerrainContext : public qsf::Context
	{


	//[-------------------------------------------------------]
	//[ Public static methods                                 ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Add a context reference
		*
		*  @note
		*    - If this is the first reference, the internal global terrain options are created automatically
		*/
		void addContextReference(uint64 ColorMap);

		/**
		*  @brief
		*    Release a context reference
		*
		*  @note
		*    - If this is the last reference, the internal global terrain options are destroyed automatically
		*/
		void releaseContextReference();

		/**
		*  @brief
		*    Return the global OGRE terrain options instance
		*
		*  @return
		*    The global OGRE terrain options instance, can be a null pointer, do not destroy the instance
		*/
		inline Ogre::TerrainGlobalOptions* getOgreTerrainGlobalOptions();

		kc_terrain::TerrainMaterialGenerator* GetMaterialGenerator();
	//[-------------------------------------------------------]
	//[ Private static data                                   ]
	//[-------------------------------------------------------]
	private:
		static uint32					   mContextCounter;	///< Terrain context counter
		static Ogre::TerrainGlobalOptions* mTerrainGlobals;	///< OGRE terrain globals instance, can be a null pointer
		kc_terrain::TerrainMaterialGenerator* mGenerator;

	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf


//[-------------------------------------------------------]
//[ Implementation                                        ]
//[-------------------------------------------------------]
#include <asset_collector_tool\kc_terrain\TerrainContext-inl.h>
