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
#include <asset_collector_tool\qsf_editor\tools\TerrainEditColorMapToolbox.h>
#include <qsf/message/MessageProxy.h>
#include <asset_collector_tool\extern\include\Magick++.h>
#include <qsf/debug/request/CompoundDebugDrawRequest.h>
#include <qsf/math/Color4.h>

//todo speed up by doing direct conversions instead of using for and while -loops

//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class TerrainEditColorMap;
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
		class TerrainEditmodeColorMap : public qsf::editor::EditMode
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::TerrainEditmodeColorMap" unique pluginable view ID


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
			TerrainEditmodeColorMap(qsf::editor::EditModeManager* editModeManager);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~TerrainEditmodeColorMap();

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
			void RaiseTerrain(glm::vec2 Mappoint);
			bool RaisePoint(glm::vec2 Mappoint, float Intensity);
			int timer;

			qsf::Color4 GetOldColor(glm::vec2 &MapPoint);
			//Mappoint and x,y Terrain
			qsf::Color4 GetOldColorFromSmallMaps(glm::vec2 &Mappoint,int x,int y);
			//void 
			//[-------------------------------------------------------]
			//[ Private methods                                       ]
			//[-------------------------------------------------------]
		private:
			bool IsActive;
			qsf::DebugDrawProxy			mDebugDrawProxy; ///< Debug draw proxy for text output
			qsf::CompoundDebugDrawRequest DebugRequsts;
			uint32 mDetailViewSingleTrack;

			float MoveDelta;
			glm::vec3 OldPos;
			glm::vec3 TerrainEditmodeColorMap::getPositionUnderMouse();
			std::string GetCurrentTimeForFileName();
			inline virtual void mouseMoveEvent(QMouseEvent& qMouseEvent) override;

			Magick::Image* image;

			struct Terrains
			{
				long x;
				long y;
				Terrains(long _x, long _y)
				{
					x = _x;
					y = _y;
				}
				bool operator<(const Terrains& a) const
				{
					return (x < a.x);
				}
			};
			std::vector<Terrains> NeedUpdates;

			struct PaintMap
			{
				glm::vec2 Pixel;

				qsf::Color4 NewColor;
				//Small map needs this infos
				int xTerrain;
				int yTerrain;
			};

			std::vector<PaintMap*> PaintedPixels;
			void SetUseSplitMaps(bool use);
			bool GetUseSplitMaps();
		private:
			glm::vec3 oldmouspoint;
			glm::vec3 yo_mousepoint;
			bool mouseisvalid;
			boost::container::flat_set <uint64> CreatedUnits;
			bool WasPressed;

			float Radius;


			//terrains shape
			float partsize;
			bool mIsInsideVisible;
			float percentage;
			float Offset;
			float ColorMapSize;
			float Scale;
			int mParts;
			qsf::WeakPtr<kc_terrain::TerrainComponent> TerrainMaster;
			// return a relative point from a world point (notice that z axis is mirrored)
			glm::vec2 ConvertWorldPointToRelativePoint(glm::vec2 WorldPoint);
			// return a worldpoint point from a mappoint (heighmap which is like 1024² or 2048²)
			void UpdateTerrains();
			void UpdateTerrainsForSmallMaps();
			TerrainEditColorMapToolbox* TerrainEditGUI;
			void generateMaterial();
			virtual bool onStartup(EditMode* previousEditMode) override;
			virtual void onShutdown(EditMode* nextEditMode) override;

			bool mUseSplitMaps;

			bool SplitMapInSmallMaps();
			void ChangeMaterialToUseSmallMaps(int x , int y);
			//IMG and Local Asset Name
			std::vector<std::pair<Magick::Image*,std::string>> m_SmallImages;
			std::pair<Magick::Image*,std::string> GetSmallImageByTerrainId(int x, int y);
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
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::TerrainEditmodeColorMap)
