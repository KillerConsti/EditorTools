// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/KC_AbstractView.h"

//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{

		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		const uint32 KC_AbstractView::PLUGINABLE_ID = qsf::StringHash("user::editor::KC_AbstractView");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		KC_AbstractView::KC_AbstractView(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
			View(viewManager, qWidgetParent)
		{
			// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
			addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);

		}

		KC_AbstractView::~KC_AbstractView()
		{

		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		void KC_AbstractView::retranslateUi()
		{
		}

		void KC_AbstractView::changeVisibility(bool visible)
		{

		}






		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]
		void KC_AbstractView::showEvent(QShowEvent* qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);
			
		}

		void KC_AbstractView::hideEvent(QHideEvent* qHideEvent)
		{
			// Call the base implementation
			View::hideEvent(qHideEvent);
			// Disconnect Qt signals/slots
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &KC_AbstractView::onUndoOperationExecuted);
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &KC_AbstractView::onRedoOperationExecuted);
		}


		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
