// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "qsf/asset/AssetSystemTypes.h"

#include <OGRE/OgreGpuProgramParams.h>
#include <OGRE/Terrain/OgreTerrainPrerequisites.h>
#include <OGRE/Terrain/OgreTerrainMaterialGenerator.h>
#include <qsf\asset\AssetProxy.h>
#undef TRANSPARENT	// Header hell: The OGRE headers include some nasty macros


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
	*    OGRE terrain material generator
	*
	*  @remarks
	*    Some notes about certain OGRE terrain features which we don't use
	*    - Light map - which we don't use because those are far to expensive to calculate during runtime (we had quite some issues)
	*    - Composite map - which we don't use because those are far to expensive to calculate during runtime (we had quite some issues)
	*/
	class TerrainMaterialGenerator : public Ogre::TerrainMaterialGenerator
	{


	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	public:
		class Profile : public Ogre::TerrainMaterialGenerator::Profile
		{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			Profile(Ogre::TerrainMaterialGenerator* parent, const Ogre::String& name, const Ogre::String& desc,uint64 mColorMap);
			virtual ~Profile();


		//[-------------------------------------------------------]
		//[ Public virtual Ogre::TerrainMaterialGenerator::Profile methods ]
		//[-------------------------------------------------------]
		public:
			virtual bool isVertexCompressionSupported() const override;
			virtual Ogre::MaterialPtr generate(const Ogre::Terrain* ogreTerrain) override;
			virtual Ogre::MaterialPtr generateForCompositeMap(const Ogre::Terrain* ogreTerrain) override;
			virtual void setLightmapEnabled(bool enabled) override;
			virtual Ogre::uint8 getMaxLayers(const Ogre::Terrain* ogreTerrain) const override;
			virtual void updateParams(const Ogre::MaterialPtr& ogreMaterial, const Ogre::Terrain* ogreTerrain) override;
			virtual void updateParamsForCompositeMap(const Ogre::MaterialPtr& ogreMaterial, const Ogre::Terrain* ogreTerrain) override;
			virtual void requestOptions(Ogre::Terrain* ogreTerrain) override;
			void SetColorMap(uint64 ColorMap);
			void SetBlendMaps(std::vector<std::string> Blendtextures);

			void CreateEditableColorMap(const Ogre::Terrain* ogreTerrain);
			
		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			void createMaterial(const Ogre::String& matName, const Ogre::Terrain* ogreTerrain);
			void CreateOgreMaterial(const Ogre::Terrain* Terrain);

			

		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			
			Ogre::String mMatNameComp;
			uint64 m_profil_ColorMap;
			//up to 6 blendtextures
			std::vector<std::string> m_Blendtextures; 
			public: 
				Ogre::String mMatName;
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		TerrainMaterialGenerator(uint64 ColorMAp);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TerrainMaterialGenerator();
		void RefreshMaterial( const Ogre::Terrain* ogreTerrain);
		void ChangeColorMapToSmallerMap(Ogre::Terrain* Terrain, int x, int y, int parts_per_line);
		void UpdateColorMap(const Ogre::Terrain* ogreTerrain);
	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	private:
		void onAssetsMounted(const qsf::Assets& assets);
		uint64 mColorMap;

	};


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
} // qsf
