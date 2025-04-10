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
#include <asset_collector_tool\view\ImageDecompiler.h>
#include <QtCore\qcoreapplication.h>
#include <asset_collector_tool\view\IndicatorView.h>
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

			//UpdateAssetStuffWhich does not belong here
			struct UpdateAsset 
			{
				uint64 mGlobalAssetId;
				uint64 mEntityId; //needed?
				float Time = 0.f;
			};
			UpdateAsset mUpdateAsset;
			void StartTimerUpdateAsset(uint64 GlobalAssetId, uint64 EntityId);
			void WaitUntilUpdateAsset(const qsf::JobArguments& jobArguments);
		private:
			qsf::JobProxy mAssetReadyForUpdate;
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
			void onPushTutorials();

			void ExecuteNewContextAction(QAction *action);
			void ShowDebugGui();
			void ShowScenarioScriptToolView();
			void ShowImageDecoderView();
			void ShowIndicatorView();
			private:
			uint64 BuildComponentHolder();
			void GUIManager::buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select);
			uint64 GUIManager::BuildEntity(glm::vec3 position, std::string layher);
			qsf::editor::View* DebugToolView;
			qsf::editor::View* ScenarioScriptToolView;
			qsf::editor::View* ImageDecompilerView;
			qsf::editor::View* IndicatorView;
			void SetFarClipping();
			float OldFarClippingValue;
		};


	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
	} // editor
} // user

