// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <qsf_editor/view/View.h>

#include <camp/userobject.hpp>
#include <qsf/job/JobProxy.h>
#include <qsf/debug/DebugDrawProxy.h>


#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include "qsf_editor/editmode/EditMode.h"
#include "qsf_editor/editmode/EditModeManager.h"
#include <asset_collector_tool\kc_terrain\TerrainComponent.h>
#include <asset_collector_tool\qsf_editor\tools\TerrainTexturingToolbox.h>
#include <qsf/message/MessageProxy.h>
#include <qsf/debug/request/CompoundDebugDrawRequest.h>
#include <qsf\debug\request\RectangleDebugDrawRequest.h>

#include <qsf_editor/asset/AssetEditHelper.h>
#include <asset_collector_tool\extern\include\Magick++.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class TerrainTexturingTool;
}
namespace qsf
{
	namespace editor
	{
		namespace base
		{
			class Operation;
		}
	}
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Indicator view class
		*
		*  @note
		*    - The UI is created via source code
		*/
		class TerrainTexturingTool : public qsf::editor::EditMode
		{


		//[-------------------------------------------------------]
		//[ Qt definitions (MOC)                                  ]
		//[-------------------------------------------------------]
			Q_OBJECT	// All files using the Q_OBJECT macro need to be compiled using the Meta-Object Compiler (MOC) of Qt, else slots won't work!
				// (VisualStudio: Header file -> Right click -> Properties -> "Custom Build Tool")


		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		public:
			static const uint32 PLUGINABLE_ID;	///< "user::editor::TerrainTexturingTool" unique pluginable view ID


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] viewManager
			*    Optional pointer to the view manager this view should be registered to, can be a null pointer
			*  @param[in] qWidgetParent
			*    Pointer to parent Qt widget, can be a null pointer (in this case you're responsible for destroying this view instance)
			*/
			TerrainTexturingTool(qsf::editor::EditModeManager* editModeManager);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~TerrainTexturingTool();
			void ReplaceLayer(int LayerId, std::string NewMaterial);
		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		protected:
			//virtual void retranslateUi() override;
			//virtual void changeVisibility(bool visible) override;
			virtual bool evaluateBrushPosition(const QPoint& mousePosition, glm::vec3& position);
			kc_terrain::TerrainComponent*				   mTerrainComponent;
		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]
		protected:
			//virtual void showEvent(QShowEvent* qShowEvent) override;
			//virtual void hideEvent(QHideEvent* qHideEvent) override;

			void PaintJob(const qsf::JobArguments& jobArguments);
			qsf::JobProxy PaintJobProxy;
			void SetHeight(glm::vec2 MapPoint);

			void IncreaseHeight(glm::vec2 Point,float NewHeight);
			float GetCustomIntensity(float distancetoMidpoint, TerrainTexturingToolbox::TerrainEditMode2 Mode);
			void RaiseTerrain(glm::vec2 Mappoint);
			void RaisePoint(glm::vec2 Mappoint, float Intensity);
			int timer;
			
			//void 
		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			bool IsActive;
			qsf::DebugDrawProxy			mDebugDrawProxy; ///< Debug draw proxy for text output


			float MoveDelta;
			glm::vec3 OldPos;
			glm::vec3 TerrainTexturingTool::getPositionUnderMouse();
			qsf::MessageProxy		mSaveMapProxy;
			void SaveMap(const qsf::MessageParameters& parameters);

			qsf::MessageProxy		mCopyQSFTerrain;
			void CopyQSFTerrain(const qsf::MessageParameters& parameters);

			qsf::MessageProxy		mClearLayer;
			void ClearLayer(const qsf::MessageParameters& parameters);

			qsf::MessageProxy		mReplaceGroundLayer;
			void ReplaceGroundLayer(const qsf::MessageParameters& parameters);

			//when loading we may need to pay attention to "y"-Axis as it is flipped
			void SaveTheFuckingMap();
			std::string GetCurrentTimeForFileName();
			inline virtual void mouseMoveEvent(QMouseEvent& qMouseEvent) override;

			struct LayerData
			{
				std::vector<glm::vec3> Data;
				std::string LayerName;
				uint16 AffectedPoints = 0;
				int OriginLayer;
				bool operator<(const LayerData& a) const
				{
					return (AffectedPoints > a.AffectedPoints);
				}
			};
		private:
			glm::vec3 oldmouspoint;
			glm::vec3 yo_mousepoint;
			bool mouseisvalid;
			boost::container::flat_set <uint64> CreatedUnits;
			bool WasPressed;
			qsf::CompoundDebugDrawRequest DebugRequsts;
			uint32 mDetailViewSingleTrack;
			float Radius;
			qsf::RectangleDebugDrawRequest* mRectAngleRequest;
			unsigned int		mChunkDrawRequestId;	///< Debug draw request IDs for chunk visualisation

			void UpdateChunkDebugDrawg(glm::vec3 worldpos,int x ,int y);
			int SelectedChunk_x;
			int SelectedChunk_y;
			//terrains shape
			float partsize;
			float mHeight;
			bool mIsInsideVisible;
			float percentage;
			float Offset;
			float BlendMapSize;
			float Scale;
			int mParts;
			qsf::WeakPtr<kc_terrain::TerrainComponent> TerrainMaster;

			void WriteTerrainTextureList(bool mousewasvalid);

			int onAddNewTerrain(std::string BlendMapName,int x,int y);
			bool TerrainTextureAllreadyExists(std::string CheckMe, std::vector<std::string>& ToCheck);
			bool TerrainLayerIsEmpty(Ogre::Terrain* Terrain,int layer);
			//x and y represent terrain index
			//z is set by updateterrain method when we applay some texture - so programm knows it needs to go higher
			glm::vec3 m_NeedUpdatingTerrainList;
			std::string GetSelectedLayerColor();
			int GetBlendMapWithTextureName(int xTerrain,int yTerrain);
			uint8 TMG_getMaxLayers(const Ogre::Terrain* ogreTerrain) const;
			
			struct kc_vec3
			{

				uint16 x;
				uint16 y;
				float Intensity;
				kc_vec3(uint16 _x, uint16 _y, float _Intensity)
				{
					x =_x;
					y =_y;
					Intensity = _Intensity;
				}
				bool operator<(const kc_vec3& a) const
				{
					return (x*1000+y < a.x*1000+a.y);
				}
			};


			std::vector<kc_vec3> AffectedPoints[16][16];
			// return a relative point from a world point (notice that z axis is mirrored)
			glm::vec2 ConvertWorldPointToRelativePoint(glm::vec2 WorldPoint);
			// return a worldpoint point from a mappoint (heighmap which is like 1024� or 2048�)
			glm::vec2 ConvertMappointToWorldPoint(glm::vec2 WorldPoint);
			void UpdateTerrains();
			TerrainTexturingToolbox* TerrainEditGUI;
			std::shared_ptr<qsf::editor::AssetEditHelper>	 mAssetEditHelper;
			virtual bool onStartup(EditMode* previousEditMode) override;
			virtual void onShutdown(EditMode* nextEditMode) override;


			struct NewMixedIntensities
			{
				float IntensityLayer1 =0;
				float IntensityLayer2 =0;
				float IntensityLayer3 =0;
				float IntensityLayer4 =0;
				float IntensityLayer5 =0; 
			};

			void MixIntensitiesTerrain(NewMixedIntensities* NMI, int LayerId, float NewIntensity);

			//saveData
			void WriteTerrainTextureJson(qsf::editor::AssetEditHelper* IAP);
		//[-------------------------------------------------------]
		//[ CAMP reflection system                                ]
		//[-------------------------------------------------------]
			QSF_CAMP_RTTI()	// Only adds the virtual method "campClassId()", nothing more


		};


	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
	} // editor
} // user


//[-------------------------------------------------------]
//[ CAMP reflection system                                ]
//[-------------------------------------------------------]
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::TerrainTexturingTool)
