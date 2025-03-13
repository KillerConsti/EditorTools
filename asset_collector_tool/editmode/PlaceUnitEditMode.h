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
#include <qsf/debug/request/CompoundDebugDrawRequest.h>

#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include "qsf_editor/editmode/EditMode.h"
#include "qsf_editor/editmode/EditModeManager.h"
#include <qsf/renderer/terrain/TerrainComponent.h>
#include <asset_collector_tool\qsf_editor\tools\TerrainEditToolbox.h>
#include <qsf/message/MessageProxy.h>

#include <qsf_editor/asset/AssetEditHelper.h>
#include <asset_collector_tool\kc_terrain\TerrainComponent.h>
#include <asset_collector_tool\extern\include\Magick++.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class PlaceUnitEditMode;
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

//mouse key not working

//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	class GeneralFunctions
	{
	public:
		static void buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation& compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select);
		static uint64 BuildEntity(glm::vec3 position, qsf::StringHash shash);
	};
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
		class PlaceUnitEditMode : public qsf::editor::EditMode
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::PlaceUnitEditMode" unique pluginable view ID


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
			PlaceUnitEditMode(qsf::editor::EditModeManager* editModeManager);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~PlaceUnitEditMode();
			uint64 GetPrototypeId();
			void SetPrototypeId(uint64 PrototypeId);
			//[-------------------------------------------------------]
			//[ Protected virtual qsf::editor::View methods           ]
			//[-------------------------------------------------------]
		protected:
			//virtual void retranslateUi() override;
			//virtual void changeVisibility(bool visible) override;
			virtual bool evaluateBrushPosition(const QPoint& mousePosition, glm::vec3& position);

			//[-------------------------------------------------------]
			//[ Protected virtual QWidget methods                     ]
			//[-------------------------------------------------------]
		protected:
			//virtual void showEvent(QShowEvent* qShowEvent) override;
			//virtual void hideEvent(QHideEvent* qHideEvent) override;

			void PaintJob(const qsf::JobArguments& jobArguments);
			qsf::JobProxy PaintJobProxy;
			qsf::CompoundDebugDrawRequest DebugRequsts;
			uint32 mDetailViewSingleTrack;
			//void 
			//inline virtual void mousePressEvent(QMouseEvent& qMouseEvent) override;
			inline virtual void mouseMoveEvent(QMouseEvent& qMouseEvent) override;
			//[-------------------------------------------------------]
			//[ Private methods                                       ]
			//[-------------------------------------------------------]
		private:

			glm::vec3 PlaceUnitEditMode::getPositionUnderMouse();
			glm::vec3 PlaceUnitEditMode::getPositionUnderMouse(glm::vec2 Pos);
			bool eventFilter(QObject* obj, QEvent* event);


		private:

			enum State
			{
				Outside, //we left edit window
				Inside,
				Not_on_Map, //we didnt hit any terrain or walkable thing
				Visible //entity is still at pos 0,0,0
			};
			glm::vec3 oldmouspoint;
			glm::vec3 yo_mousepoint;

			State mState;

			virtual bool onStartup(EditMode* previousEditMode) override;
			virtual void onShutdown(EditMode* nextEditMode) override;

			uint64 mPrototypeId;
			glm::vec2 LastMousePos;
			glm::vec3 mOldPos;
			bool mLastButtonState;
			uint64 GetOrCreateEntity();
			uint64 mEntityId;
			 boost::container::flat_set<uint64> mIgnoreYourself;
			 void SetText(qsf::Component* whatDidWeHit);
			 bool CheckNewTerrain();
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
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::PlaceUnitEditMode)
