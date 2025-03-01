// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "qsf/renderer/component/RendererComponent.h"
#include "qsf/reflection/type/CampQsfAssetProxy.h"
#include <asset_collector_tool\kc_terrain\TerrainDefinition.h>
#include <asset_collector_tool\kc_terrain\TerrainContext.h>
#include <qsf/renderer/terrain/TerrainComponent.h>
#include <ogre\Terrain\OgreTerrain.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ogre
{
	class Ray;
	class Terrain;
	class TerrainGlobalOptions;
	class TerrainGroup;
}
namespace qsf
{
	class ParameterGroup;
	class CameraComponent;
	class RenderWindow;
	class TerrainDefinition;
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
	*    Terrain component class
	*
	*  @remarks
	*    == OGRE terrain global color map ==
	*    Inside the OGRE "dat" terrain format, OGRE can store the global terrain color map.
	*
	*    Sadly, this global color map is stored as uncompressed RGB without mipmaps. While this is fine for editing, it's far from optimal for best possible loading times and runtime performance.
	*    Additionally, we're using the alpha channel of the global terrain color map to be able to cut holes into the terrain. This information would get lost when storing the global color map
	*    directly inside the OGRE "dat" terrain format. In retail build, we noticed random crashes when using the global terrain color map stored inside the OGRE "dat" terrain format.
	*
	*    In order to overcome all those issues with the global OGRE terrain color map, we just don't store it inside the OGRE "dat" terrain format. When loading the OGRE terrain, we provide it
	*    with an optimized cached asset global color map. For editing the terrain color map, we keep the uncompressed source asset as tif. It's the job of the editor to generate the optimized
	*    cached asset global color map.
	*/
	class TerrainComponent : public qsf::RendererComponent
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		static const uint32 COMPONENT_ID;				///< "qsf::TerrainComponent" unique component ID
		static const uint32 TERRAIN_WORLD_SIZE;			///< "TerrainWorldSize" unique class property ID inside the class
		static const uint32 SKIRT_SIZE;					///< "SkirtSize" unique class property ID inside the class
		static const uint32 MAX_PIXEL_ERROR;			///< "MaxPixelError" unique class property ID inside the class
		static const uint32 TERRAIN_ASSET;				///< "TerrainAsset" unique class property ID inside the class
		static const uint8  RENDER_QUEUE_GROUP_ID;


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Constructor
		*
		*  @param[in] prototype
		*    The prototype this component is in, no null pointer allowed
		*/
		inline explicit TerrainComponent(qsf::Prototype* prototype);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TerrainComponent();

		int kc_getTerrainChunksPerEdge();

		/**
		*  @brief
		*    Set the terrain chunks per edge
		*
		*  @param[in] terrainChunksPerEdge
		*    Terrain chunks per edge (requirement: 2^n, tested with 1, 2, 4, 8, 16)
		*/
		void kc_setTerrainChunksPerEdge(int terrainChunksPerEdge);

		/**
		*  @brief
		*    Get the terrain world size
		*
		*  @return
		*    Terrain size
		*
		*  @note
		*    - Connected to the CAMP reflection system
		*/
		inline float getTerrainWorldSize() const;

		/**
		*  @brief
		*    Set terrain world size
		*
		*  @param[in] terrainWorldSize
		*    Terrain size
		*
		*  @note
		*    - Connected to the CAMP reflection system
		*/
		void setTerrainWorldSize(float terrainWorldSize);

		/**
		*  @brief
		*    Get the skirt size of terrain
		*
		*  @return
		*    Skirt size
		*
		*  @note
		*    - Connected to the CAMP reflection system
		*/
		inline float getSkirtSize() const;

		/**
		*  @brief
		*    Set the skirt of terrain
		*
		*  @param[in] skirtSize
		*    Size of skirt
		*
		*  @remarks
		*    Set the skirt of terrain. This means, to build a skirt around
		*    the terrain pieces to hide cracks in terrain between different details.
		*    Don't use to high values, because that slows down the rendering because
		*    of the pixel fill rate. A value of 2 could be optimal.
		*
		*  @note
		*    - Connected to the CAMP reflection system
		*/
		void setSkirtSize(float skirtSize);

		/**
		*  @brief
		*    Get the maximum pixel error
		*
		*  @return
		*    The maximum pixel error
		*
		*  @note
		*    - Connected to the CAMP reflection system
		*/
		inline float getMaxPixelError() const;

		/**
		*  @brief
		*    Set the maximum pixel error
		*
		*  @param[in] maxPixelError
		*    The maximum pixel error
		*
		*  @note
		*    - This is used to specify the LOD to be used
		*    - Connected to the CAMP reflection system
		*/
		void setMaxPixelError(float maxPixelError);

		/**
		*  @brief
		*    Get segments of a height map
		*
		*  @return
		*    Number of segments
		*
		*  @note
		*    - Segments are 2^n-1
		*    - TODO(np) Segment count is needed for Terrain Editing. It is only a getter
		*               So should it connected to the CAMP reflection system?
		*/
		int getTerrainSegments() const;

		inline const qsf::AssetProxy& getTerrainAsset() const;
		void setTerrainAsset(const qsf::AssetProxy& assetProxy);


		void SetPosition(glm::vec3 newpos);
		glm::vec3 getPosition();

		void SetScale(glm::vec3 Scale);
		glm::vec3 GetScale();
		glm::vec3 mPos;
		glm::vec3 mScale;


		void SetUpdate(bool yeah);
		bool GetUpdate();
		//[-------------------------------------------------------]
		//[ Collision                                             ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Find the intersection point of terrain component by a ray generated by using a given render window and a mouse position
		*
		*  @param[in]  renderWindow
		*    Render window to use for the ray generation
		*  @param[in]  xPosition
		*    X mouse position inside the render window to use for the ray generation
		*  @param[in]  yPosition
		*    Y mouse position inside the render window to use for the ray generation
		*  @param[out] position
		*    If not a null pointer, if the a hit was found, we get the position of intersecting
		*
		*  @return
		*    True if a hit with the terrain component was found
		*/
		bool getTerrainHitByRenderWindow(const qsf::RenderWindow& renderWindow, int xPosition, int yPosition, glm::vec3* position) const;

		/**
		*  @brief
		*    Find the intersection point of terrain component by a ray
		*
		*  @param[in]  ray
		*    Represent the ray for intersection // TODO(co) Which coordinate system?
		*  @param[out] position
		*    If not a null pointer, if the a hit was found, we get the position of intersecting
		*
		*  @return
		*    True if a hit with the terrain component was found
		*/
		bool getTerrainHitByRay(const Ogre::Ray& ray, glm::vec3* position) const;

		/**
		*  @brief
		*    Return whether a ray intersects the terrain bounding
		*
		*  @param[in]  ray
		*    Represent the world space ray for intersection
		*  @param[out] closestDistance
		*    If not a null pointer, receives the distance along the ray at which it intersects, not touched in case there's no hit
		*
		*  @return
		*    True if a hit with the terrain component bounding was found
		*/
		bool getTerrainHitBoundingBoxByRay(Ogre::Ray& ray, float* closestDistance = nullptr) const;

		//[-------------------------------------------------------]
		//[ Internal                                              ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Get the terrain globals
		*
		*  @return
		*    Terrain globals, can be a null pointer, do not destroy the instance
		*/
		inline Ogre::TerrainGlobalOptions* getTerrainGlobals() const;

		/**
		*  @brief
		*    Get the OGRE terrain group
		*
		*  @return
		*    OGRE terrain group, can be a null pointer, do not destroy the instance
		*/
		inline Ogre::TerrainGroup* getOgreTerrainGroup() const;
		inline Ogre::TerrainGroup* getOgreTerrainGroup2() const;
		/**
		*  @brief
		*    Get the OGRE terrain
		*
		*  @return
		*    OGRE terrain, can be a null pointer, do not destroy the instance
		*/

		/*killers stuff here*/

		void SetNewColorMap(qsf::AssetProxy NewAssetId);
		qsf::AssetProxy GetColorMap();
		qsf::AssetProxy mColorMap;

		bool mInitDone;
		Ogre::Terrain* getOgreTerrain() const;

		
		void ReloadSubTerrainMaterials(long x, long y);
		void ReloadSmallTerrainMaterial(long x, long y,qsf::GlobalAssetId MaterialId);
		void UseMiniColorMaps(int parts,int x,int y, std::string LocalAssetName);
		void RefreshMaterial(Ogre::Terrain* Ot);
		void SaveTerrain();

		//kc Camp Properties 
		//void kc_SetTerrainSize(int Size);
		//int kc_GetTerrainSize();
		int kc_TerrainSize;
		int kc_RealBlendMapSize;

		qsf::AssetProxy mHeightmap;
		qsf::AssetProxy mTexturesMap1_4;
		//6...8 unused for now we need higher layer count for them
		qsf::AssetProxy mTextureMap5_8;
		qsf::AssetProxy mLayerDescription;
		std::string uneditabld;
		std::string automaticly_created;

		void SetNewHeightMap(qsf::AssetProxy NewAssetId);
		qsf::AssetProxy GetNewHeightMap();

		void SetNewTextureMap1_4(qsf::AssetProxy NewAssetId);
		qsf::AssetProxy GetNewTextureMap1_4();
		void SetNewTextureMap5_8(qsf::AssetProxy NewAssetId);
		qsf::AssetProxy GetNewTextureMap5_8();

		float GetMinHeight();
		void SetMinHeight(float a);
		float GetMaxHeight();
		void SetMaxHeight(float a);
		float mTerrainMinHeight;
		float mTerrainMaxHeight;
		/*
		void GetMinHeight();
		void GetMaxHeight();
		*/
		void SetTerrainLayerList(qsf::AssetProxy NewAssetId);
		qsf::AssetProxy GetTerrainLayerList();
		qsf::AssetProxy mTerrainLayerList;
		void InformMaterialGeneratorAboutNewColorMap(uint64 GlobalAssetId);
		void UpdatePosition(bool up);
		bool GetUpdatePosition();

		void SetDoNotLoadNextTime(bool donotload);
		bool GetDoNotLoadNextTime();
		bool mDoNotLoadNextTime;

		//must be with capitalized letters else you 'll use wrong one
		void SetBlendMapSize(int size);
		int GetBlendtMapSize();

		void SetHeightMapSize(int size);
		int GetHeightMapSize();
	//[-------------------------------------------------------]
	//[ Protected virtual qsf::Component methods              ]
	//[-------------------------------------------------------]
	protected:
		virtual void onComponentPropertyChange(const Component& component, uint32 propertyId) override;

		//[-------------------------------------------------------]
		//[ Lifecycle                                             ]
		//[-------------------------------------------------------]
		virtual bool onStartup() override;
		virtual void onShutdown() override;


	//[-------------------------------------------------------]
	//[ Protected virtual qsf::RendererComponent methods      ]
	//[-------------------------------------------------------]
	protected:
		virtual void setOgreSceneNodeVisibility(bool visible) override;


	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		void onSettingsPropertyChanged(const qsf::ParameterGroup& parameterGroup, uint32 property);
		void updateOgreTerrainMaxPixelError();
		void onAssetsMounted(const qsf::Assets& assets);
		void onAssetsUnmounted(const qsf::Assets& assets);
		void onAssetChanged(const qsf::Asset& asset);
		void defineTerrain();
		void removeAllOgreTerrains();
		void buildMap();
		void setPosition(const glm::vec3& position,glm::vec3 Offset);
	//[-------------------------------------------------------]
	//[ Private data                                          ]
	//[-------------------------------------------------------]
	private:
		// Connected to the CAMP reflection system
		//int							mTerrainChunksPerEdge;			///< Size of chunks per edge. E.g. 8 means, there are 8x8 chunks
		int							kc_mTerrainChunksPerEdge;
		float						mTerrainWorldSize;
		float						mSkirtSize;
		float						mMaxPixelError;
		qsf::AssetProxy					mTerrainAsset;
		// Internal only
		Ogre::TerrainGlobalOptions* mOgreTerrainGlobalOptions;		///< OGRE terrain globals instance, can be a null pointer, to not destroy the instance
		Ogre::TerrainGroup*			mOgreTerrainGroup;				///< OGRE terrain group instance, can be a null pointer
		std::set<qsf::GlobalAssetId>		mGlobalTerrainAssetIds;			///< Global asset IDs that affect this component
		kc_terrain::TerrainContext* mTerrainContext;
		bool mDelete;
		bool Relead();
		int m_OldAttempt;
		int mBlendMapSize;
		int mHeightMapSize;
		int mColorMapSize;
	//[-------------------------------------------------------]
	//[ CAMP reflection system                                ]
	//[-------------------------------------------------------]
	QSF_CAMP_RTTI()	// Only adds the virtual method "campClassId()", nothing more


	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf


//[-------------------------------------------------------]
//[ Implementation                                        ]
//[-------------------------------------------------------]
#include <asset_collector_tool\kc_terrain\TerrainComponent-inl.h>


//[-------------------------------------------------------]
//[ CAMP reflection system                                ]
//[-------------------------------------------------------]
QSF_CAMP_TYPE_NONCOPYABLE(kc_terrain::TerrainComponent)
