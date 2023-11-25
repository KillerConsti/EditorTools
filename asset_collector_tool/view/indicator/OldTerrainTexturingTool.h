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
#include <qsf/renderer/terrain/TerrainComponent.h>
#include <asset_collector_tool\qsf_editor\tools\TerrainTexturingToolbox.h>
#include <qsf/message/MessageProxy.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class OldTerrainTexturingTool;
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
		class OldTerrainTexturingTool : public qsf::editor::EditMode
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::OldTerrainTexturingTool" unique pluginable view ID


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
			OldTerrainTexturingTool(qsf::editor::EditModeManager* editModeManager);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~OldTerrainTexturingTool();

		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		protected:
			//virtual void retranslateUi() override;
			//virtual void changeVisibility(bool visible) override;
			virtual bool evaluateBrushPosition(const QPoint& mousePosition, glm::vec3& position);
			qsf::TerrainComponent*				   mTerrainComponent;
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
			glm::vec3 OldTerrainTexturingTool::getPositionUnderMouse();
			qsf::MessageProxy		mSaveMapProxy;
			void SaveMap(const qsf::MessageParameters& parameters);
			void SaveTheFuckingMap();
			std::string GetCurrentTimeForFileName();
			float ReadValue(glm::vec2);
			inline virtual void mousePressEvent(QMouseEvent& qMouseEvent) override;
			inline virtual void mouseMoveEvent(QMouseEvent& qMouseEvent) override;


		private:
			glm::vec3 oldmouspoint;
			glm::vec3 yo_mousepoint;
			bool mouseisvalid;
			boost::container::flat_set <uint64> CreatedUnits;
			bool WasPressed;
			
			float Radius;
			

			//terrains shape
			float partsize;
			float mHeight;
			bool mIsInsideVisible;
			float percentage;
			float Offset;
			float Heighmapsize;
			float Scale;
			int mParts;
			qsf::WeakPtr<qsf::TerrainComponent> TerrainMaster;

			void WriteTerrainTextureList();
			uint64 GetSelectedLayerColor();
			uint64 mSelectedLayerColor;
			int GetBlendMapWithTextureName(int xTerrain,int yTerrain);
			uint8 TMG_getMaxLayers(const Ogre::Terrain* ogreTerrain) const;
			void LoadOldMap();
			
			std::vector<glm::vec2> AffectedPoints[8][8];
			// return a relative point from a world point (notice that z axis is mirrored)
			glm::vec2 ConvertWorldPointToRelativePoint(glm::vec2 WorldPoint);
			// return a worldpoint point from a mappoint (heighmap which is like 1024² or 2048²)
			glm::vec2 ConvertMappointToWorldPoint(glm::vec2 WorldPoint);
			void UpdateTerrains();
			TerrainTexturingToolbox* TerrainEditGUI;

			virtual bool onStartup(EditMode* previousEditMode) override;
			virtual void onShutdown(EditMode* nextEditMode) override;

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
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::OldTerrainTexturingTool)
