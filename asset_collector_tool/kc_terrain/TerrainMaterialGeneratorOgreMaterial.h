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
#include <ogre\OgreShadowCameraSetupPSSM.h>
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
	class TerrainMaterialGeneratorOgreMaterial : public Ogre::TerrainMaterialGenerator
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
		//[ Public virtual Ogre::TerrainMaterialGeneratorOgreMaterial::Profile methods ]
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
		public:
			void createMaterial(const Ogre::String& matName, const Ogre::Terrain* ogreTerrain);
			void CreateOgreMaterial(const Ogre::Terrain* Terrain);
			enum TechniqueType
			{
				HIGH_LOD,
				LOW_LOD,
				RENDER_COMPOSITE_MAP
			};
			void addTechnique(const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
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

				//Shader stuff
				class /*_OgreTerrainExport*/ ShaderHelper : public Ogre::TerrainAlloc
				{
				public:
					ShaderHelper() : mShadowSamplerStartHi(0), mShadowSamplerStartLo(0) {}
					virtual ~ShaderHelper() {}
					virtual Ogre::HighLevelGpuProgramPtr generateVertexProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					virtual Ogre::HighLevelGpuProgramPtr generateFragmentProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					virtual void updateParams(const Profile* prof, const Ogre::MaterialPtr& mat, const Ogre::Terrain* terrain, bool compositeMap);
				protected:
					virtual Ogre::String getVertexProgramName(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					virtual Ogre::String getFragmentProgramName(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					virtual Ogre::HighLevelGpuProgramPtr createVertexProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt) = 0;
					virtual Ogre::HighLevelGpuProgramPtr createFragmentProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt) = 0;
					virtual void generateVertexProgramSource(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					virtual void generateFragmentProgramSource(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					virtual void generateVpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream) = 0;
					virtual void generateFpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream) = 0;
					virtual void generateVpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream) = 0;
					virtual void generateFpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream) = 0;
					virtual void generateVpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream) = 0;
					virtual void generateFpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream) = 0;
					virtual void defaultVpParams(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, const Ogre::HighLevelGpuProgramPtr& prog);
					virtual void defaultFpParams(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, const Ogre::HighLevelGpuProgramPtr& prog);
					virtual void updateVpParams(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, const Ogre::GpuProgramParametersSharedPtr& params);
					virtual void updateFpParams(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, const Ogre::GpuProgramParametersSharedPtr& params);
					static Ogre::String getChannel(Ogre::uint idx);

					size_t mShadowSamplerStartHi;
					size_t mShadowSamplerStartLo;
				};

				/// Utility class to help with generating shaders for Cg / HLSL.
				class /*_OgreTerrainExport*/ ShaderHelperCg : public ShaderHelper
				{
				protected:
					Ogre::HighLevelGpuProgramPtr createVertexProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					Ogre::HighLevelGpuProgramPtr createFragmentProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					void generateVpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateVpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream);
					void generateFpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream);
					void generateVpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					Ogre::uint generateVpDynamicShadowsParams(Ogre::uint texCoordStart, const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateVpDynamicShadows(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadowsHelpers(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadowsParams(Ogre::uint* texCoord, Ogre::uint* sampler, const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadows(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
				};

				/*class  ShaderHelperHLSL : public ShaderHelperCg
				{
				protected:
					Ogre::HighLevelGpuProgramPtr createVertexProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					Ogre::HighLevelGpuProgramPtr createFragmentProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					void generateVpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateVpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream);
					void generateFpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream);
					void generateVpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					Ogre::uint generateVpDynamicShadowsParams(Ogre::uint texCoordStart, const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateVpDynamicShadows(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadowsHelpers(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadowsParams(Ogre::uint* texCoord, Ogre::uint* sampler, const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadows(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
				};

				/// Utility class to help with generating shaders for GLSL.
				class  ShaderHelperGLSL : public ShaderHelper
				{
				protected:
					Ogre::HighLevelGpuProgramPtr createVertexProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					Ogre::HighLevelGpuProgramPtr createFragmentProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					void generateVpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateVpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream);
					void generateFpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream);
					void generateVpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					Ogre::uint generateVpDynamicShadowsParams(Ogre::uint texCoordStart, const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateVpDynamicShadows(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadowsHelpers(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadowsParams(Ogre::uint* texCoord, Ogre::uint* sampler, const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadows(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
				};

				/// Utility class to help with generating shaders for GLSL ES.
				class ShaderHelperGLSLES : public ShaderHelper
				{
				protected:
					Ogre::HighLevelGpuProgramPtr createVertexProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					Ogre::HighLevelGpuProgramPtr createFragmentProgram(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt);
					void generateVpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpHeader(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateVpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream);
					void generateFpLayer(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::uint layer, Ogre::StringStream& outStream);
					void generateVpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpFooter(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					Ogre::uint generateVpDynamicShadowsParams(Ogre::uint texCoordStart, const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateVpDynamicShadows(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadowsHelpers(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadowsParams(Ogre::uint* texCoord, Ogre::uint* sampler, const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
					void generateFpDynamicShadows(const Profile* prof, const Ogre::Terrain* terrain, Profile::TechniqueType tt, Ogre::StringStream& outStream);
				};*/

				ShaderHelper* mShaderGen;
				bool mLayerNormalMappingEnabled;
				bool mLayerParallaxMappingEnabled;
				bool mLayerSpecularMappingEnabled;
				bool mGlobalColourMapEnabled;
				bool mLightmapEnabled;
				bool mCompositeMapEnabled;
				bool mReceiveDynamicShadows;
			   Ogre::PSSMShadowCameraSetup* mPSSM;
				bool mDepthShadows;
				bool mLowLodShadows;
				bool mSM3Available;
				bool mSM4Available;
				Ogre::String mShaderLanguage;

				bool isShadowingEnabled(Profile::TechniqueType tt, const Ogre::Terrain* terrain) const;
				Ogre::PSSMShadowCameraSetup* getReceiveDynamicShadowsPSSM() const { return mPSSM; }
				void setReceiveDynamicShadowsPSSM(Ogre::PSSMShadowCameraSetup* pssmSettings);
				bool getReceiveDynamicShadowsDepth() const { return mDepthShadows; }
				bool _isSM3Available() const { return mSM3Available; }
				bool _isSM4Available() const { return mSM4Available; }
				Ogre::String _getShaderLanguage() const { return mShaderLanguage; }
				bool isGlobalColourMapEnabled() const { return mGlobalColourMapEnabled; }
				/** Whether to support a global colour map over the terrain in the shader,
				if it's present (default true).
				*/
				void setGlobalColourMapEnabled(bool enabled);
				/** Whether to support a light map over the terrain in the shader,
				if it's present (default true).
				*/
				bool isLightmapEnabled() const { return mLightmapEnabled; }
				/** Whether to support a light map over the terrain in the shader,
				if it's present (default true).*/
				bool isCompositeMapEnabled() const { return mCompositeMapEnabled; }
				bool getReceiveDynamicShadowsEnabled() const { return mReceiveDynamicShadows; }
		};


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	public:
		/**
		*  @brief
		*    Default constructor
		*/
		TerrainMaterialGeneratorOgreMaterial(uint64 ColorMAp);

		/**
		*  @brief
		*    Destructor
		*/
		virtual ~TerrainMaterialGeneratorOgreMaterial();
		void RefreshMaterial( const Ogre::Terrain* ogreTerrain);
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
