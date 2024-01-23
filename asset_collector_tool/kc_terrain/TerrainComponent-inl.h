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
		qsf::TerrainComponent(prototype),
		// Connected to the CAMP reflection system
		mHeightMapSize(1025),
		mColorMapSize(1024),
		mBlendMapSize(1024),
		//mTerrainChunksPerEdge(16),
		mTerrainWorldSize(1500.0f),
		mSkirtSize(2.0f),
		mMaxPixelError(8.0f),
		// Internal only
		mOgreTerrainGlobalOptions(nullptr),
		mOgreTerrainGroup(nullptr),
		mIsEditing(false),
		mColorMap(qsf::getUninitialized<uint64>()),
		mPos(glm::vec3(0,0,0)),
		mInitDone(false),
		mTerrainMinHeight(0.f),
		mTerrainMaxHeight(0.f),
		automaticly_created("generated when using terrain edit tools.Used for saving and loading terrains"),
		uneditabld("------------------------------------"),
		mDoNotLoadNextTime(false),
		mBlendAndHeightMapSize(1024),
		mCustomImportData(nullptr),
		mDelete(false),
		kc_mTerrainChunksPerEdge(16)
	{
		// Nothing to do in here
	}

	inline uint32 TerrainComponent::getHeightMapSize() const
	{
		return mHeightMapSize;
	}

	inline uint32 TerrainComponent::getColorMapSize() const
	{
		return mColorMapSize;
	}

	inline uint32 TerrainComponent::getBlendMapSize() const
	{
		return mBlendMapSize;
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

	inline bool TerrainComponent::getEditing() const
	{
		return mIsEditing;
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
