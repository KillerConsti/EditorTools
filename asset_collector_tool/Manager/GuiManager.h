// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <qsf/message/MessageProxy.h>
#include <qsf/job/JobProxy.h>
#include <QtCore\qobject.h>
#include <QtWidgets\qaction.h>
#include <qsf_editor_base/operation/CompoundOperation.h>

#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>

#include <asset_collector_tool\view\DebugUnitView.h>
#include <asset_collector_tool\view\indicator\ScenarioScriptTool.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace qsf
{
	namespace editor
	{
		class AssetEditHelper;
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
		*   this is a wrapper to have a better "view" menu
		*
		*  @note
		*    - The UI is created via source code
		*/
		class GUIManager : QObject
		{
			Q_OBJECT

		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			/**
		
			/**
			*  @brief
			*    Destructor
			*/
			static GUIManager* instance;
			GUIManager();
			~GUIManager();

			static void init();

		private:
			void onPreNewEmptyMap(const qsf::MessageParameters& parameters);
			qsf::MessageProxy mOnPreNewEmptyMapMessageProxy;
			qsf::JobProxy mWaitUntilEditorReady;
			void WaitUntilEditorReady(const qsf::JobArguments& jobArguments);
			bool ModifyMenuBar();
			QAction* KC_MenuAction;
			QMenuBar* OldMenuBar;
			void RemoveFog();
			float SetGlobalGlossiness();
			std::pair<float,float> OldFogValue;
			private Q_SLOTS:

			void onPushSelectButton2();
			void ExecuteNewContextAction(QAction *action);
			void ShowDebugGui();
			void ShowScenarioScriptToolView();
			private:
			uint64 BuildComponentHolder();
			void GUIManager::buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select);
			uint64 GUIManager::BuildEntity(glm::vec3 position, std::string layher);

			qsf::editor::View* DebugToolView;
			qsf::editor::View* ScenarioScriptToolView;
		};


	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
	} // editor
} // user

