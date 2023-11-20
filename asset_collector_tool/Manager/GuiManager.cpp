// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/manager/GuiManager.h"
#include <qsf_editor/plugin/Messages.h>
#include <qsf\log\LogSystem.h>
#include <qsf\QsfHelper.h>
#include <qsf_editor\EditorHelper.h>
#include <em5\plugin\Jobs.h>
#include <qsf_editor/menu/MenuBar.h>
#include <qsf\map\Map.h>
#include <qsf\map\Entity.h>
#include <qsf/map/component/EnvironmentComponent.h>
#include <qsf/renderer/compositor/DefaultCompositingComponent.h>
#include <QtWidgets\qinputdialog.h>
#include <QtWidgets\qerrormessage.h>
#include <qsf_editor_base/operation/component/SetComponentPropertyOperation.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include <asset_collector_tool\component\EditorToolsHelperComponent.h>
#include <qsf_editor_base/operation/component/CreateComponentOperation.h>

//building unit
#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include "qsf_editor/selection/layer/LayerSelectionManager.h"
#include <qsf/asset/Asset.h>
#include <qsf/prototype/helper/PrefabContent.h>
#include <qsf/prototype/BasePrototypeManager.h>
#include <qsf_editor_base/operation/entity/CreateEntityOperation.h>
#include <qsf_editor_base/operation/entity/DestroyEntityOperation.h>
#include <qsf_editor_base/operation/layer/CreateLayerOperation.h>
#include <qsf_editor_base/operation/layer/SetLayerPropertyOperation.h>
#include <qsf_editor_base/operation/data/BackupPrototypeOperationData.h>
#include <qsf/prototype/helper/PrefabContent.h>
#include <qsf/prototype/BasePrototypeManager.h>
#include <qsf/map/query/ComponentMapQuery.h>

#include <qsf/map/layer/LayerManager.h>
#include <qsf/map/layer/Layer.h>
#include <qsf/component/base/MetadataComponent.h>
#include <qsf_editor_base/operation/component/DestroyComponentOperation.h>

//execute views
#include <qsf_editor/application/MainWindow.h>
#include <asset_collector_tool\view\DebugUnitView.h>
#include <asset_collector_tool\view\indicator\ScenarioScriptTool.h>
#include <em5\EM5Helper.h>
#include <em5\game\Game.h>
#include <QtCore\qcoreapplication.h>
#include <qsf_editor/renderer/RenderView.h>
#include "qsf/renderer/window/RenderWindow.h"
#include <qsf/renderer/component/CameraComponent.h>
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{

		GUIManager* GUIManager::instance = nullptr;




		GUIManager::GUIManager() :
			DebugToolView(nullptr),
			ScenarioScriptToolView(nullptr)
		{
			//mOnPreNewEmptyMapMessageProxy.registerAt(qsf::MessageConfiguration(qsf::editor::Messages::PRE_NEW_EMPTY_MAP), boost::bind(&GUIManager::onPreNewEmptyMap, this, _1));
			mWaitUntilEditorReady.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&GUIManager::WaitUntilEditorReady, this, _1));
		}


		GUIManager::~GUIManager()
		{
			mWaitUntilEditorReady.unregister();
			mOnPreNewEmptyMapMessageProxy.unregister();

		}

		void GUIManager::init()
		{
			if (instance == nullptr)
				instance = new GUIManager();
		}

		void GUIManager::onPreNewEmptyMap(const qsf::MessageParameters & parameters)
		{
			QSF_LOG_PRINTS(INFO, "pre new map")
		}

		void GUIManager::WaitUntilEditorReady(const qsf::JobArguments & jobArguments)
		{
			if (QSF_EDITOR_APPLICATION.getMainWindow() == nullptr)
				return;
				if(EM5_GAME.getInstance() != nullptr)
				{
				return;
				}
			if (ModifyMenuBar())
				mWaitUntilEditorReady.unregister();
		}



		bool GUIManager::ModifyMenuBar()
		{
			if (QSF_EDITOR_APPLICATION.getMainWindow() == nullptr)
				return false;
			// Nothing to do in here
			auto Bar2 = QSF_EDITOR_APPLICATION.getMainWindow()->findChildren<QMenuBar *>("");
			/*for (size_t t = 0; t < Bar2.size(); t++)
			{
				QSF_LOG_PRINTS(INFO, Bar2.at((int)t)->windowTitle().toStdString());
				auto actions = Bar2.at((int)t)->actions();
				for (auto a : actions)
				{
					QSF_LOG_PRINTS(INFO, a->text().toStdString());
					Bar2.at((int)t)->insertSeparator(a);
				}
			}*/

			if (Bar2.size() == 0)
				return false;
			auto MyBar = Bar2.at(0);
			OldMenuBar = MyBar;
			auto Action = MyBar->addAction("KC Misc");
			KC_MenuAction = Action;
			if (!MyBar->connect(Action, SIGNAL(triggered()), this, SLOT(onPushSelectButton2())))
			{
				QSF_LOG_PRINTS(INFO, "Slot binding was not succesfull")
			}

			//read old fog val

			auto EC = QSF_MAINMAP.getCoreEntity().getComponent<qsf::EnvironmentComponent>();
			if (EC == nullptr)
				return true;
			OldFogValue.first = EC->getGroundFogDensity();
			OldFogValue.second = EC->getAtmosphereFogDensity();
			return true;
			/*MyBar->addAction("KC Debug");
			for (size_t t = 0; MyBarActions.size(); t++)
			{
				QMenu* Menu = static_cast<QMenu*>(MyBarActions.at((int)t)))
			}*/

			/*for (size_t t =0; MyBarActions.size(); t++)
			{
			if(MyBarActions.at((int)t) == nullptr)
			continue;
			for(auto a : MyBarActions.at(int(t))->get)
				{
				if(a == nullptr)
				continue;
					QSF_LOG_PRINTS(INFO,a->text().toStdString())
				}
			}
				return Bar2.size() > 0;*/

				//finally load our views and menus


		}

		void GUIManager::RemoveFog()
		{
			auto EC = QSF_MAINMAP.getCoreEntity().getComponent<qsf::EnvironmentComponent>();
			if (EC == nullptr)
				return;
			if (EC->getGroundFogDensity() == OldFogValue.first)
			{
				EC->setGroundFogDensity(0.0001f);
				EC->setAtmosphereFogDensity(0.0001f);
			}
			else
			{
				EC->setGroundFogDensity(OldFogValue.first);
				EC->setAtmosphereFogDensity(OldFogValue.second);
			}
		}

		float GUIManager::SetGlobalGlossiness()
		{

			auto EC = QSF_MAINMAP.getCoreEntity().getComponent<qsf::compositing::DefaultCompositingComponent>();
			if (EC == nullptr)
				return 0.f;
			bool ok;
			float GGI = EC->getGlobalGlossinessIntensity();
			qsf::Entity* MyComponentHolder = QSF_MAINMAP.getEntityById(BuildComponentHolder());
			if (MyComponentHolder == nullptr)
			{
				//QSF_LOG_PRINTS(INFO,"MyComponentHolder is 0")
				return 0.75f;
			}
			if (MyComponentHolder->getComponent<EditorToolsHelperComponent>() != nullptr)
			{
				GGI = MyComponentHolder->getComponent<EditorToolsHelperComponent>()->GetGlobalGlossiness();
			}

			std::string inputstring = boost::lexical_cast<std::string>(GGI);
			if (inputstring.size() > 5)
			{
				inputstring.erase(4, inputstring.size() - 1);
			}
			inputstring = "New Glossiness (old: " + inputstring + " )";
			QString text = QInputDialog::getText(0, "Set Global Glossiness",
				inputstring.c_str(), QLineEdit::Normal,
				"", &ok);
			if (ok && !text.isEmpty()) {
				try
				{
					float NewValue = boost::lexical_cast<float>(text.toStdString());
					//we could connect it to editor ops but we dont
					qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
					if (MyComponentHolder->getComponent<EditorToolsHelperComponent>() == nullptr)
						compoundOperation2->pushBackOperation(new qsf::editor::base::CreateComponentOperation(MyComponentHolder->getId(), EditorToolsHelperComponent::COMPONENT_ID));
					compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(MyComponentHolder->getId(), EditorToolsHelperComponent::COMPONENT_ID, qsf::StringHash("New Global Glossiness"), NewValue));
					QSF_EDITOR_OPERATION.push(compoundOperation2);
					//EC->setAllPropertyOverrideFlags(true);
					//MyComponentHolder->getComponent<EditorToolsHelperComponent>()->SetGlobalGlossiness(NewValue);
					//MyComponentHolder->getComponent<EditorToolsHelperComponent>()->setAllPropertyOverrideFlags(true);
				}
				catch (const std::exception& e)
				{
					auto EM = new QErrorMessage(nullptr);
					EM->showMessage(e.what());
					EM->exec();
				}
			}
			return EC->getGlobalGlossinessIntensity();

		}

		void GUIManager::onPushSelectButton2()
		{
			//QSF_LOG_PRINTS(INFO, "we pressed our menubar")

			QMenu *xmenu = new QMenu();
			QMenu* submenuA = xmenu->addMenu("Editor Only Settings");
			QMenu* submenuB = xmenu->addMenu("Global Settings");
			QMenu* submenuC = xmenu->addMenu("Debug");
			//QMenu* submenuD = xmenu->addMenu("D");
			//QMenu* submenuE = xmenu->addMenu("E");
			QAction* actionA_Setup = submenuA->addAction("remove fog");
			QAction* action2_Setup = submenuA->addAction("increase far clipping");
			QAction* actionB_Setup = submenuB->addAction("Set Global Glossiness");
			QAction* actionC_Setup = submenuC->addAction("Log Debugger");
			QAction* actionC_SetupD = submenuC->addAction("Scenario Script Tool");
			//QAction* actionD_Setup = submenuD->addAction("Setup");
			//QAction* actionE_Setup = submenuE->addAction("Setup");

			/*auto childs = &OldMenuBar->findChildren<QMenu>("");
			for (size_t t=0; t < childs->size();t++)
			{
				QSF_LOG_PRINTS(INFO,"aha" << t)
			}*/
			auto x = OldMenuBar->actionGeometry(KC_MenuAction).x();
			auto y = OldMenuBar->actionGeometry(KC_MenuAction).y() + OldMenuBar->actionGeometry(KC_MenuAction).height();
			connect(xmenu, SIGNAL(triggered(QAction*)), SLOT(ExecuteNewContextAction(QAction*)));
			xmenu->exec(OldMenuBar->mapToGlobal(QPoint(x, y)));
		}

		void GUIManager::ExecuteNewContextAction(QAction * action)
		{
			action->data().toString().toStdString();
			//allready checked it out
			//auto Data = mUITrainTrackTool->treeWidget->currentItem()->data(1,0);
			//QSF_LOG_PRINTS(INFO,"Data "<< action->text().toStdString())
			if (action->text().toStdString() == "remove fog")
				RemoveFog();
			else if ("increase far clipping" == action->text().toStdString())
			SetFarClipping();
			else if (action->text().toStdString() == "Set Global Glossiness")
				SetGlobalGlossiness();
			else if (action->text().toStdString() == "Log Debugger")
			{
				ShowDebugGui();
			}
			else if (action->text().toStdString() == "Scenario Script Tool")
			{
				ShowScenarioScriptToolView();
			}
		}

		void GUIManager::ShowDebugGui()
		{
			if (DebugToolView == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Feed our own menu")
					auto Manager = &QSF_EDITOR_APPLICATION.getMainWindow()->getViewManager();
				DebugToolView = new user::editor::DebugUnitView(Manager, QSF_EDITOR_APPLICATION.getMainWindow()->topLevelWidget());
				if (DebugToolView == nullptr)
					QSF_LOG_PRINTS(INFO, "we created a nullptr")
			}
			else
			{
				QSF_SAFE_DELETE(DebugToolView);
			}
		}

		void GUIManager::ShowScenarioScriptToolView()
		{
			if (ScenarioScriptToolView == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Feed our own menu")
					auto Manager = &QSF_EDITOR_APPLICATION.getMainWindow()->getViewManager();
				ScenarioScriptToolView = new user::editor::ScenarioScriptTool(Manager, QSF_EDITOR_APPLICATION.getMainWindow()->topLevelWidget());
				if (ScenarioScriptToolView == nullptr)
					QSF_LOG_PRINTS(INFO, "we created a nullptr")
			}
			else
			{
				QSF_SAFE_DELETE(ScenarioScriptToolView);
			}
		}

		void GUIManager::SetFarClipping()
		{
			QSF_EDITOR_APPLICATION.getMainWindow()->getRenderView();
			auto renderWindow = &QSF_EDITOR_APPLICATION.getMainWindow()->getRenderView().getRenderWindow();
			auto cameraComponent = renderWindow->getCameraComponent();
			if(cameraComponent->getFarClipDistance() != 2000.f )
			{
				OldFarClippingValue = cameraComponent->getFarClipDistance();
				cameraComponent->setFarClipDistance(2000.f);
			}
			else
			{
				cameraComponent->setFarClipDistance(cameraComponent->getFarClipDistance());

			}
			}





		uint64 GUIManager::BuildEntity(glm::vec3 position, std::string layher)
		{
			//Create Unit <-> this is quite long
			qsf::Prototype* prototype = QSF_MAINPROTOTYPE.getPrefabByLocalAssetId("em5/prefab/debug/debug_box");
			if (prototype == nullptr)
			{
				return qsf::getUninitialized<uint64>();
			}

			const uint32 selectedLayerId = qsf::StringHash(layher);
			// Copy an existing entity or create a prefab instance?
			qsf::GlobalAssetId globalPrefabAssetId = qsf::getUninitialized<qsf::GlobalAssetId>();
			std::vector<const qsf::Prototype*> prototypevector;
			prototypevector.push_back(prototype);
			qsf::PrefabContent localPrefabContent;
			qsf::PrefabContent* usedPrefabContent = /*(nullptr != prefabContent) ? prefabContent :*/ &localPrefabContent;

			bool createPrefabInstance = !prototype->isEntity();
			if (createPrefabInstance)
			{
				// The prototype system stores the local prefab asset ID for any prototype loaded (at least for main prototypes of a prefab)
				// -> Get the global prefab asset ID
				const qsf::GlobalAssetId globalPrefabAssetId = QSF_MAINPROTOTYPE.getPrefabGlobalAssetIdByPrototypeId(prototype->getId());
				if (qsf::isUninitialized(globalPrefabAssetId))
				{
					// Prefab asset not found, so it can't be a prefab but e.g. really just a prototype
					createPrefabInstance = false;
				}
			}

			qsf::BasePrototypeManager::UniqueIdMap uniqueIdMap;
			QSF_MAINPROTOTYPE.buildIdMatchingMapWithGeneratedIds(*prototype, uniqueIdMap, nullptr, createPrefabInstance);
			std::vector<const qsf::Prototype*> originalPrototypes;
			usedPrefabContent->cloneFromPrototype(*prototype, uniqueIdMap, true, qsf::isInitialized(globalPrefabAssetId), &originalPrototypes);
			{ // Do some processing to apply name, position and prefab references
				const qsf::Asset* asset = &qsf::Asset(qsf::AssetProxy(qsf::StringHash("em5/prefab/debug/debug_box")).getGlobalAssetId());
				const std::string* name = (nullptr != asset) ? &asset->getName() : nullptr;
				usedPrefabContent->processForEntityInstancing(originalPrototypes, globalPrefabAssetId, name, &position);
			}
			//if (usedPrefabContent->getPrototypes().at(0)->getComponent<qsf::MeshComponent>() != nullptr)
			//usedPrefabContent->getPrototypes().at(0)->destroyComponent<qsf::DebugBoxComponent>();
			// Build compound operation that creates a copy of the cloned prototypes in the map
			qsf::editor::base::CompoundOperation* compoundOperation = new qsf::editor::base::CompoundOperation();
			buildInstantiateTemporaryPrototypesOperation(*compoundOperation, usedPrefabContent->getPrototypes(), selectedLayerId, false);
			uint64 entityId = usedPrefabContent->getMainPrototype()->getId();
			compoundOperation->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(entityId, qsf::MetadataComponent::COMPONENT_ID, qsf::MetadataComponent::NAME, "global_glossiness_setter"));
			QSF_EDITOR_OPERATION.push(compoundOperation);
			return entityId;
			//Unit is created now apply transform
		}


		uint64 GUIManager::BuildComponentHolder()
		{
			for (EditorToolsHelperComponent* EditorHelper : qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<EditorToolsHelperComponent>())
			{
				if (EditorHelper != nullptr)
					return EditorHelper->getEntityId();
			}
			auto Layermanager = &QSF_MAINMAP.getLayerManager();
			qsf::Layer* Layer = Layermanager->getLayerByName(qsf::StringHash("KC_EDITOR_TOOLS"));
			if (Layer == nullptr) //create Layer
			{
				qsf::editor::base::CreateLayerOperation* CLO = new qsf::editor::base::CreateLayerOperation(Layermanager->getRootLayer().getId(), qsf::StringHash("KC_EDITOR_TOOLS"));
				CLO->setText("KC_SINGLE_TRAINTRACKLAYER");
				QSF_EDITOR_OPERATION.push(CLO);
				qsf::editor::base::SetLayerPropertyOperation* SLP = new qsf::editor::base::SetLayerPropertyOperation(qsf::StringHash("KC_EDITOR_TOOLS"), qsf::StringHash("Name"), camp::Value("KC_EDITOR_TOOLS"));
				QSF_EDITOR_OPERATION.push(SLP);

			}
			return BuildEntity(glm::vec3(0, 50, 0), "KC_EDITOR_TOOLS");
		}

		void GUIManager::buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select)
		{
			// Cycle through link components in forward order, so that parents are created before their children
			for (size_t index = 0; index < temporaryPrototypes.size(); ++index)
			{
				qsf::Prototype& prototype = *temporaryPrototypes[index];

				// Backup the temporary prototype
				qsf::editor::base::BackupPrototypeOperationData* backupPrototypeOperationData = new qsf::editor::base::BackupPrototypeOperationData(prototype);
				compoundOperation.addOperationData(backupPrototypeOperationData);

				// Create entity
				qsf::editor::base::CreateEntityOperation* createEntityOperation = new qsf::editor::base::CreateEntityOperation(prototype.getId(), layerId, backupPrototypeOperationData);
				compoundOperation.pushBackOperation(createEntityOperation);

				// Is this the primary prototype?
				if (index == 0)
				{
					// Set text for compound operation
					compoundOperation.setText(createEntityOperation->getText());
				}
			}

			// In case there's a select operation, we have to commit it after all operations to ensure everything required is already there
			if (select)
			{
				// TODO(co) We should call "qsf::editor::EntityOperationHelper::buildSelectOperation(primaryEntityId)" to properly select everything in case we just instanced a group,
				//          at the moment this is not possible because we would need a concrete instance at this point which we don't have until the operation has been executed
				const uint64 primaryPrototypeId = temporaryPrototypes[0]->getId();
				//compoundOperation.pushBackOperation(QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>().createSelectEntityOperation(primaryPrototypeId));
			}
		}
		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
