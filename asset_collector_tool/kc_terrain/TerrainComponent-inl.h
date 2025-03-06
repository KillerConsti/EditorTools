#include "TerrainComponent.h"
// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace kc_terrain
{


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	inline TerrainComponent::TerrainComponent(qsf::Prototype* prototype) :
#ifndef NoQSFTerrain
		qsf::TerrainComponent(prototype),
#else
		qsf::RendererComponent(prototype),
#endif
		mTerrainWorldSize(1500.0f),
		mSkirtSize(2.0f),
		mMaxPixelError(8.0f),
		// Internal only
		mOgreTerrainGlobalOptions(nullptr),
		mOgreTerrainGroup(nullptr),
		mColorMap(qsf::getUninitialized<uint64>()),
		mPos(glm::vec3(0,0,0)),
		mInitDone(false),
		mTerrainMinHeight(0.f),
		mTerrainMaxHeight(0.f),
		automaticly_created("generated when using terrain edit tools.Used for saving and loading terrains"),
		uneditabld("------------------------------------"),
		mDoNotLoadNextTime(false),
		mBlendMapSize(1024),
		mHeightMapSize(1025),
		mDelete(false),
		kc_mTerrainChunksPerEdge(16),
		minTime(0.f)
	{
		// Nothing to do in here
	}



	/*inline int TerrainComponent::getTerrainChunksPerEdge() const
	{
		return mTerrainChunksPerEdge;
	}*/

	inline float TerrainComponent::getTerrainWorldSize() const
	{
		return mTerrainWorldSize;
	}

	inline float TerrainComponent::getSkirtSize() const
	{
		return mSkirtSize;
	}

	inline float TerrainComponent::getMaxPixelError() const
	{
		return mMaxPixelError;
	}

	inline Ogre::TerrainGlobalOptions* TerrainComponent::getTerrainGlobals() const
	{
		return mOgreTerrainGlobalOptions;
	}

	inline Ogre::TerrainGroup* TerrainComponent::getOgreTerrainGroup() const
	{
		return mOgreTerrainGroup;
	}

	inline Ogre::TerrainGroup* TerrainComponent::getOgreTerrainGroup2() const
	{
		return mOgreTerrainGroup;
	}

	inline const qsf::AssetProxy& TerrainComponent::getTerrainAsset() const
	{
		return mTerrainAsset;
	}



	

	inline void TerrainComponent::SetScale(glm::vec3 Scale)
	{
	}

	inline glm::vec3 TerrainComponent::GetScale()
	{
		return mScale;
	}

	inline void TerrainComponent::SetUpdate(bool yeah)
	{
		if(yeah)
		Relead();
	}

	inline bool TerrainComponent::GetUpdate()
	{
		return false;
	}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf
