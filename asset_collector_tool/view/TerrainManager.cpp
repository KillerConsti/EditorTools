// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/DebugUnitView.h"
#include "ui_DebugUnitView.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)


#include <qsf_editor/EditorHelper.h>
#include <qsf/QsfHelper.h>
#include <qsf/log/LogSystem.h>

//Assets 
#include <qsf/asset/helper/AssetDependencyCollector.h>
#include <qsf/asset/AssetProxy.h>
#include <qsf/asset/project/AssetPackage.h>
#include <qsf/asset/project/Project.h>
#include <qsf_editor/asset/AssetEditHelper.h>
#include <qsf_editor/asset/import/AssetImportManager.h>

//access mods
#include <em5/modding/ModSystem.h>
#include <em5\EM5Helper.h>
#include <em5/plugin/Plugin.h>

//open url
#include <qsf\platform\PlatformSystem.h>

#include <qsf\message\MessageSystem.h>
#include <qsf/log/LogSystem.h>


#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include "qsf_editor/selection/layer/LayerSelectionManager.h"
#include <qsf/asset/Asset.h>
#include <qsf/prototype/helper/PrefabContent.h>
#include <qsf/prototype/BasePrototypeManager.h>

#include <qsf/renderer/mesh/MeshComponent.h>
#include <qsf/renderer/debug/DebugBoxComponent.h>
#include <qsf/prototype/helper/PrefabContent.h>

//ops

#include <qsf_editor_base/operation/CompoundOperation.h>
#include <qsf_editor_base/operation/entity/CreateEntityOperation.h>
#include <qsf_editor_base/operation/entity/DestroyEntityOperation.h>
#include <qsf_editor_base/operation/layer/CreateLayerOperation.h>
#include <qsf_editor_base/operation/layer/SetLayerPropertyOperation.h>
#include <qsf_editor_base/operation/data/BackupPrototypeOperationData.h>
#include <qsf_editor_base/operation/data/BackupComponentOperationData.h>
#include <qsf_editor_base/operation/component/CreateComponentOperation.h>
#include <qsf_editor_base/operation/component/DestroyComponentOperation.h>
#include <qsf_editor_base/operation/component/SetComponentPropertyOperation.h>
#include <qsf_editor/operation/entity/EntityOperationHelper.h>

#include <qsf/map/map.h>
#include <qsf_editor/map/MapHelper.h>
#include <qsf/map/EntityHelper.h>
#include <qsf/prototype/Prototype.h>
#include <qsf\map\Entity.h>
#include <qsf\component\base\MetadataComponent.h>
#include <qsf\file\FileStream.h>
#include <qsf\file\FileHelper.h>

#include <qsf/asset/AssetSystem.h>
#include <qsf/asset/type/AssetType.h>
#include < qsf/asset/type/AssetTypeManager.h>
#include <boost\foreach.hpp>
#include <algorithm>
#include <QtWidgets\qmenu.h>
#include <QtGui\qclipboard.h>
#include <QtWidgets\qfiledialog.h>
#include <QtCore\qstandardpaths.h>
#include <fstream>
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{

		using boost::lexical_cast;
		using boost::bad_lexical_cast;
		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		const uint32 DebugUnitView::PLUGINABLE_ID = qsf::StringHash("user::editor::DebugUnitView");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		DebugUnitView::DebugUnitView(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
		KC_AbstractView(viewManager,qWidgetParent),
			//View(viewManager, qWidgetParent),
			mUiDebugUnitView(nullptr)
		{
			// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
			addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);

		}

		DebugUnitView::~DebugUnitView()
		{

			// Destroy the UI view instance
			if (nullptr != mUiDebugUnitView)
			{
				QSF_LOG.NewMessage.disconnect(boost::bind(&DebugUnitView::OnMessageArrives, this, _1));
				disconnect(mUiDebugUnitView->treeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));
				delete mUiDebugUnitView;
			}
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		void DebugUnitView::retranslateUi()
		{
			// Retranslate the content of the UI, this automatically clears the previous content
			mUiDebugUnitView->retranslateUi(this);
		}

		void DebugUnitView::changeVisibility(bool visible)
		{
			// Lazy evaluation: If the view is shown the first time, create its content
			if (visible && nullptr == mUiDebugUnitView)
			{
				// Setup the view content
				QWidget* contentWidget = new QWidget(this);
				{

					// Load content to widget
					mUiDebugUnitView = new Ui::DebugUnitView();
					mUiDebugUnitView->setupUi(contentWidget);
				}

				// Set content to view
				setWidget(contentWidget);
				// Connect Qt signals/slots
				connect(mUiDebugUnitView->pushCheckUnit, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
				connect(mUiDebugUnitView->loadexternallog, SIGNAL(clicked(bool)), this, SLOT(OnLoadLogFile(bool)));
				connect(mUiDebugUnitView->searhprototype, SIGNAL(clicked(bool)), this, SLOT(OnPushSearchForPrototype(bool)));
				connect(mUiDebugUnitView->export_2, SIGNAL(clicked(bool)), this, SLOT(OnSaveLogFile(bool)));

				mUiDebugUnitView->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

				if (!connect(mUiDebugUnitView->treeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &))))
					QSF_LOG_PRINTS(INFO, "Slot connection treewidget custom context Menu failed")

				//connect(mUiDebugUnitView->comboBoxType, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
				auto con2 = &QSF_LOG.NewMessage.connect(boost::bind(&DebugUnitView::OnMessageArrives, this, _1), boost::signals2::at_back);
				if (con2 == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "Log System Tracker failed to activate ")
				}
				else
				{
					QSF_LOG_PRINTS(INFO, "Log System Tracker activated ")
				}
				mUiDebugUnitView->treeWidget->setColumnCount(4);
				addTreeRoot("Missing Components", "");
				addTreeRoot("Missing Textures", "");
				addTreeRoot("Missing Materials", "");
				addTreeRoot("Missing Prototypes", "This is hard to fix... be carefull");
				addTreeRoot("Broken Meshs (false Id)", "");
			}
			else if (!visible && nullptr == mUiDebugUnitView)
			{
				QSF_LOG.NewMessage.disconnect(boost::bind(&DebugUnitView::OnMessageArrives, this, _1));
			}

		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void DebugUnitView::rebuildGui()
		{
		}



		void DebugUnitView::OnMessageArrives(const qsf::LogMessage& Message)
		{
			if (Message.severityLevel != qsf::LogMessage::ERROR && Message.severityLevel != qsf::LogMessage::WARNING)
				return;
			//QSF_LOG_PRINTS(INFO,"\\nError Message arrived")
			if (Message.message.find("[QSF error] Unknown QSF component class ID") != std::string::npos)
			{
				MissingComponentMessage(Message.message);
			}
			else if (Message.message.find("qsf::Material::getMaterialTextures() failed to load the material property texture") != std::string::npos && Message.message.find("OGRE EXCEPTION(6:FileNotFoundException)") != std::string::npos)
			{
				MissingTextureMessage(Message.message);
			}
			else
				if (Message.message.find("[QSF error] QSF failed to create OGRE entity") != std::string::npos && Message.message.find("with mesh") != std::string::npos && Message.message.find("Exception caught: OGRE EXCEPTION(6:FileNotFoundException)") != std::string::npos)
				{
					//[QSF error] QSF failed to create OGRE entity "6301475433588001582" with mesh "47696". Exception caught: OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 47696 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)
					//[QSF error] QSF failed to create OGRE entity "3363940995457887785" with mesh "47707". Exception caught: OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 47707 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)
					BrokenMeshMessage(Message.message);
				}
				else if (Message.message.find("is referencing the base prototype") != std::string::npos && Message.message.find("but the base prototype cannot be found in the prefab") != std::string::npos)
				{
					//[QSF warning] Prefab asset em5/prefab/beaverfield_vehicles/fd_bcfd_tanker52.json: QSF noticed that prototype 6084894884677994421 is referencing the base prototype 3710961177554599144, but the base prototype cannot be found in the prefab "em5/prefab/beaverfield_vehicles/fd_bffd_ladder18" (global asset ID 18667)
					 MissingPrototypeMessage(Message.message);
				}
			
		}

		void DebugUnitView::OnPrototypeIdChanged()
		{
		}

		void DebugUnitView::OnPushSearchForPrototype(const bool pressed)
		{
			auto Text = mUiDebugUnitView->line_edit_searchprototype->text().toStdString();
			try
			{
				auto Id = boost::lexical_cast<uint64>(Text);
				bool match = false;
				uint64 TypeId = 0;
				for (auto a : QSF_ASSET.getAssetTypeManager().getAssetTypeMap()) //find asset type
				{
					if ("prefab" == a.second->getTypeName())
					{
						TypeId = a.second->getTypeId();
						match = true;
						break;
					}
				}
				qsf::Assets AssetList;
				QSF_ASSET.getAssetsOfType(TypeId, AssetList);

				for (auto a : AssetList)
				{
					if (JsonContainsPrototypeSingleSearch(qsf::AssetProxy(a->getGlobalAssetId()).getLocalAssetName() + ".json", Id))
					{
						QSF_LOG_PRINTS(INFO,"prototype belongs to: "<< qsf::AssetProxy(a->getGlobalAssetId()).getLocalAssetName() + ".json")
						return;
					}
				}

			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO,e.what())
				return;
			}
			QSF_LOG_PRINTS(INFO,"Could not find prototype: "<< Text)
		}

		void DebugUnitView::OnLoadLogFile(const bool pressed)
		{
			//clear everything
			MissingPrototypes.clear();
			MissingComponents.clear();
			MissingTextures.clear();
			BrokenMeshs.clear();
			UpdateBrokenMeshs();
			UpdateMissingComponents();
			UpdateMissingPrototypes();
			UpdateMissingTextures();

			//open dialog at log direction
			auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString();
			path= path+ "/../EMERGENCY 5/log";
			auto path2 = QString(path.c_str());
			QString filter = "Log File (*.log)";
			auto Dialog = QFileDialog::getOpenFileName(nullptr, QString("Logfile to open"), path2, filter);
			if (Dialog.size() < 1)
				return;
			std::string Dia = Dialog.toStdString();
			QSF_LOG_PRINTS(INFO,"DebugUnitView - Read Logfile: "<< Dia)

			//read stream
				std::fstream newfile;
				newfile.open(Dia, std::ios::in); //open a file to perform read operation using file object
			if (newfile.is_open()) {   //checking whether the file is open
				std::string tp;
				while (getline(newfile, tp)) { //read data from file object and put it into string.
					readfile_ParseLine(tp);
				}
				newfile.close(); //close the file object.
			}

		}

		void user::editor::DebugUnitView::readfile_ParseLine(std::string input)
		{
			if (input.find("[QSF error] Unknown QSF component class ID") != std::string::npos)
			{
				MissingComponentMessage(input);
			}
			else if (input.find("qsf::Material::getMaterialTextures() failed to load the material property texture") != std::string::npos && input.find("OGRE EXCEPTION(6:FileNotFoundException)") != std::string::npos)
			{
				MissingTextureMessage(input);
			}
			else
				if (input.find("[QSF error] QSF failed to create OGRE entity") != std::string::npos && input.find("with mesh") != std::string::npos && input.find("Exception caught: OGRE EXCEPTION(6:FileNotFoundException)") != std::string::npos)
				{
					//[QSF error] QSF failed to create OGRE entity "6301475433588001582" with mesh "47696". Exception caught: OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 47696 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)
					//[QSF error] QSF failed to create OGRE entity "3363940995457887785" with mesh "47707". Exception caught: OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 47707 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)
					BrokenMeshMessage(input);
				}
				else if (input.find("is referencing the base prototype") != std::string::npos && input.find("but the base prototype cannot be found in the prefab") != std::string::npos)
				{
					//[QSF warning] Prefab asset em5/prefab/beaverfield_vehicles/fd_bcfd_tanker52.json: QSF noticed that prototype 6084894884677994421 is referencing the base prototype 3710961177554599144, but the base prototype cannot be found in the prefab "em5/prefab/beaverfield_vehicles/fd_bffd_ladder18" (global asset ID 18667)
					MissingPrototypeMessage(input);
				}
		}

		void DebugUnitView::OnSaveLogFile(const bool pressed)
		{
			/*
			thats the stuff we want to sort
			std::vector<_BrokenMeshs> BrokenMeshs;
			*/
			 
			 //make it top down thats a lot easier
			QString fileName =
				QFileDialog::getSaveFileName(this, tr("Save Error Report"),
					"untitled.csv", tr("CSV (*.csv)"));

			// you may forget this:
			if (fileName == "")
			return;
				//ui.scopeWindow->saveImage(fileName);
				QSF_LOG_PRINTS(INFO,"Filename "<< fileName.toStdString());


				//analyse subtrees?
				auto MissingComponentssize = MissingComponents.size();
				auto MissingTexturesssize = MissingTextures.size();
				auto MissingPrototypessize = MissingPrototypes.size();
				auto BrokenMeshssize = BrokenMeshs.size();
				std::ofstream  myFile(fileName.toStdString());
				//if (!ofstream .is_open()) throw std::runtime_error("Could not open file");
				myFile << "Missing Materials; ; ;\n";
				myFile << "GlobalAssetId;MaterialId;MaterialName\n";
				for (size_t t = 0; t < MissingTextures.size(); t++)
				{
					myFile << MissingTextures.at(t).GlobalAssetId << ";" << MissingTextures.at(t).MaterialId << ";"<< MissingTextures.at(t).MaterialName << "\n";
				}
				myFile << "\n";
				myFile << "Missing Prototypes; ; ;\n";
				myFile << "Source Prototype (dont exist);Source Asset Name;Target Prototype (exists and links to not existing prototype);Target Asset Name;List of all references\n";
				int x =0;
				while (true)
				{
					std::string returnstring ="";
					if(WriteDebugLineMissingPrototype(returnstring,(int)x))
					{
						myFile << returnstring;
					}
					else
					{
					break;
					}
					x++;

				}
				myFile << "\n";
				myFile << "Missing Components; ; ;\n";
				myFile << "ComponentId ;Component Name (broken/missing) ;Prefab using it;PrototType using it (inside prefab)\n";
				for (size_t t = 0; t < MissingComponentssize; t++)
				{
					auto item = MissingComponents.at(t);
					myFile << item.ComponentId <<","<<item.BrokenComponentName << ","<< item.Prefab << item.PrototTypeId<<"\n";
				}
				//Broken Meshs
				myFile << "\n";
				myFile << "Broken Meshs (global asset id is not a mesh); ; ;\n";
				myFile << "Mesh GlobalAssetId ;Current Faulty Mesh ;Guess What Might Be Better ;PrototType using it (inside prefab); Other Prefabs using it ; Prototype Id\n";
				for (size_t t = 0; t < BrokenMeshssize; t++)
				{
					auto item = BrokenMeshs.at(t);
					myFile << item.Mesh_GlobalAssetId << ";" << item.CurrentFaultyMesh << ";" << item.GuessWhatMightBeBetter << "\n";
					for (size_t j = 0; j < item.PrefabUsingIt.size(); j++)
					{
						auto prefab = item.PrefabUsingIt.at(j);
						myFile << " ; ; ; ;"<< prefab.first << " ; "<< prefab.second << "\n";
					}
				}
				myFile << "\n";


				myFile.close();
				QSF_LOG_PRINTS(INFO, "Done " << fileName.toStdString());
		}

		void DebugUnitView::ShowContextMenu(const QPoint & pos)
		{
			auto CurrentItem = mUiDebugUnitView->treeWidget->currentItem();
			if(CurrentItem == nullptr)
			return;
			QMenu contextMenu("Copy Data", mUiDebugUnitView->treeWidget);
			QAction action1("Copy Data to Clipboard", mUiDebugUnitView->treeWidget);
			contextMenu.addAction(&action1);
			connect(&contextMenu, SIGNAL(triggered(QAction*)), SLOT(ExecutContextMenu(QAction*)));
			contextMenu.exec(mUiDebugUnitView->treeWidget->mapToGlobal(pos));
		}

		void DebugUnitView::ExecutContextMenu(QAction * action)
		{
			action->data().toString().toStdString();

			if (action->text().toStdString() == "Copy Data to Clipboard")
				{
					int column = mUiDebugUnitView->treeWidget->currentColumn();
					QClipboard *clipboard = QGuiApplication::clipboard();
					QString originalText = clipboard->text();
					// etc.
					auto CurrentItemText = mUiDebugUnitView->treeWidget->currentItem()->text(column);
					clipboard->setText(CurrentItemText);
				}
		}


		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]
		void DebugUnitView::showEvent(QShowEvent* qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);

			// Perform a GUI rebuild to ensure the GUI content is up-to-date
			rebuildGui();
			//boost::signals2::signal<void(const LogMessage&)> NewMessage;

			// Connect Qt signals/slots
			//connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &DebugUnitView::onUndoOperationExecuted);
			//connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &DebugUnitView::onRedoOperationExecuted);
		}

		void DebugUnitView::hideEvent(QHideEvent* qHideEvent)
		{
			// Call the base implementation
			View::hideEvent(qHideEvent);
			// Disconnect Qt signals/slots
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &DebugUnitView::onUndoOperationExecuted);
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &DebugUnitView::onRedoOperationExecuted);
		}


		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		void DebugUnitView::onPushSelectButton(const bool pressed)
		{

			QSF_LOG_PRINTS(INFO, "Pushed Check Unit button")
				//CreateProtoUnit();
				if (!MissingComponents.empty())
				{
					bool match = false;
					uint64 TypeId = 0;
					for (auto a : QSF_ASSET.getAssetTypeManager().getAssetTypeMap()) //find asset type
					{
						if ("prefab" == a.second->getTypeName())
						{
							TypeId = a.second->getTypeId();
							match = true;
							break;
						}
					}
					qsf::Assets AssetList;
					QSF_ASSET.getAssetsOfType(TypeId, AssetList);

					for (auto a : AssetList)
					{
						JsonContainsPrototypeFaster(qsf::AssetProxy(a->getGlobalAssetId()).getLocalAssetName() + ".json");
					}
					UpdateMissingComponents();
				}
			//textures
			if (!MissingTextures.empty())
			{
				bool match = false;
				uint64 TypeId = 0;
				for (auto a : QSF_ASSET.getAssetTypeManager().getAssetTypeMap()) //find asset type
				{
					if ("material" == a.second->getTypeName())
					{
						TypeId = a.second->getTypeId();
						match = true;
						break;
					}
				}
				qsf::Assets AssetList;
				QSF_ASSET.getAssetsOfType(TypeId, AssetList);

				for (auto a : AssetList)
				{
					MaterialContainsPrototypeFaster(qsf::AssetProxy(a->getGlobalAssetId()).getLocalAssetName() + ".material");
				}
				UpdateMissingTextures();
			}
			if (!BrokenMeshs.empty())
			{
				bool match = false;
				uint64 TypeId = 0;
				for (auto a : QSF_ASSET.getAssetTypeManager().getAssetTypeMap()) //find asset type
				{
					if ("prefab" == a.second->getTypeName())
					{
						TypeId = a.second->getTypeId();
						match = true;
						break;
					}
				}
				qsf::Assets AssetList;
				QSF_ASSET.getAssetsOfType(TypeId, AssetList);

				for (auto a : AssetList)
				{
					JsonContainsBrokenMesh(qsf::AssetProxy(a->getGlobalAssetId()).getLocalAssetName() + ".json");
				}
				UpdateBrokenMeshs();
			}
			if (!MissingPrototypes.empty())
			{
					for (size_t t = 0; t < MissingPrototypes.size(); t++)
					{
					 //Reset Counter
						MissingPrototypes.at(t).RefCounter_source.clear();
						MissingPrototypes.at(t).RefCounter_target.clear();
					}
				bool match = false;
				uint64 TypeId = 0;
				for (auto a : QSF_ASSET.getAssetTypeManager().getAssetTypeMap()) //find asset type
				{
					if ("prefab" == a.second->getTypeName())
					{
						TypeId = a.second->getTypeId();
						match = true;
						break;
					}
				}
				qsf::Assets AssetList;
				QSF_ASSET.getAssetsOfType(TypeId, AssetList);
				for (auto a : AssetList)
				{
					FindPrototypeAndReferencesFromPrefabs(qsf::AssetProxy(a->getGlobalAssetId()).getLocalAssetName() + ".json");
				}
				UpdateMissingPrototypes();
			}
		}

		


	

		void DebugUnitView::MissingComponentMessage(std::string Message)
		{
			//return;

				//[QSF error] Unknown QSF component class ID "1099556652" requested by prototype 8620101463746913612. Exception caught : Unknown component ID
			auto MessageCopy = Message;
			auto Split_0 = MessageCopy.find("[QSF error] Unknown QSF component class ID \"");
			if (Split_0 == std::string::npos)
			{
				QSF_LOG_PRINTS(INFO, "Splitter0 is not working")
					return;
			}
			//QSF_LOG_PRINTS(INFO, "Splitter0 is  working")
			MessageCopy.erase(0, Split_0 + 44);
			//QSF_LOG_PRINTS(INFO, "Asfter Split 0 " <<MessageCopy)
			auto MessageCopy2 = MessageCopy;
			auto SplitId = MessageCopy2.find("\" requested by prototype ");
			if (SplitId == std::string::npos)
			{
				QSF_LOG_PRINTS(INFO, "Splitter1 is not working")
					return;
			}
			//QSF_LOG_PRINTS(INFO, "Splitter1 is  working")
			auto PrototypeId2 = MessageCopy2.erase(SplitId, MessageCopy2.size() - 1);
			SplitId = SplitId + 25;
			//QSF_LOG_PRINTS(INFO,"Asfter Split 1" << PrototypeId2)
			auto SplitId2 = MessageCopy.find(". Exception caught");
			if (SplitId2 == std::string::npos)
			{
				QSF_LOG_PRINTS(INFO, "Splitter2 is not working")
					return;
			}
			//QSF_LOG_PRINTS(INFO, "Splitter2 is  working")
			MessageCopy.erase(SplitId2, MessageCopy.size() - 1);
			//QSF_LOG_PRINTS(INFO, MessageCopy)
			MessageCopy.erase(0, SplitId);

			/*

					// Prefab asset not found, so it can't be a prefab but e.g. really just a prototype
			}*/

			for (auto a : MissingComponents)
			{
				if (a.PrototTypeId == MessageCopy && a.ComponentId == PrototypeId2)
					return;
			}
			//not includeded so push it back
			_MissingComponent* MC = &_MissingComponent();
			MC->PrototTypeId = MessageCopy;
			MC->ComponentId = PrototypeId2;
			MissingComponents.push_back(*MC);
			UpdateMissingComponents();

		}

		void DebugUnitView::MissingTextureMessage(std::string Message)
		{
			/*
			[QSF error] qsf::Material::getMaterialTextures() failed to load the material property texture: "OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 11098 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)"
			*/
			auto Split0 = Message.find("Cannot locate resource ") + 23;
			Message.erase(0, Split0);
			Split0 = Message.find(" in resource group");
			Message.erase(Split0, Message.size() - 1);
			//QSF_LOG_PRINTS(INFO, "Missing Texture Message arrived"<< Message)
			for (size_t t = 0; t < MissingTextures.size(); t++)
			{
				if (MissingTextures.at(t).GlobalAssetId == Message) //allready exist
					return;
			}
			_MissingTextures MT = _MissingTextures();
			MT.GlobalAssetId = Message;
			MissingTextures.push_back(MT);
			UpdateMissingTextures();

		}

		void DebugUnitView::MissingMaterialMessage(std::string Message)
		{
			QSF_LOG_PRINTS(INFO, "MissingMaterial Mesaage arrived")
		}

		void DebugUnitView::BrokenMeshMessage(std::string Message)
		{
			//[QSF error] QSF failed to create OGRE entity "6301475433588001582" with mesh "47696". Exception caught: OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 47696 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)
			auto Split0 = Message.find("with mesh") + 11;
			auto Split2 = Message.find("Exception caught") - 3;
			Message.erase(Split2, Message.size() - 1);
			Message.erase(0, Split0);
			for (size_t t = 0; t < BrokenMeshs.size(); t++)
			{
				if (BrokenMeshs.at(t).CurrentFaultyMesh == Message) //allready exist
				{
					return;
				}
			}
			_BrokenMeshs BT = _BrokenMeshs();
			BT.CurrentFaultyMesh = Message;
			BrokenMeshs.push_back(BT);
			UpdateBrokenMeshs();
			//QSF_LOG_PRINTS(INFO,"update meshs")
		}

		void DebugUnitView::MissingPrototypeMessage(std::string Message)
		{
			//[QSF warning] Prefab asset em5/prefab/beaverfield_vehicles/fd_bcfd_tanker52.json: QSF noticed that prototype 6084894884677994421 is referencing the base prototype 3710961177554599144, but the base prototype cannot be found in the prefab "em5/prefab/beaverfield_vehicles/fd_bffd_ladder18" (global asset ID 18667)
			auto Split0 = Message.find("[QSF warning] Prefab asset") + 27;
			auto Split1 = Message.find("QSF noticed that prototype")-2;
			auto Split2 = Split1 +28;
			auto Split3 = Message.find(" is referencing the base prototype ");
			auto Split4 = Split3+35;
			auto Split5 = Message.find(", but the base prototype cannot be found in the prefab");
			auto Split6 = Split5+56;
			auto Split7 = Message.find("(global asset ID")-2;

			_MissingPrototypes MP = _MissingPrototypes();
			MP.GlobalAssetIdTarget = Message.substr(Split0,Split1-Split0);
			MP.PrototypeIdTarget = Message.substr(Split2,Split3-Split2);
			MP.PrototypeIdSource = Message.substr(Split4,Split5-Split4);
			MP.GlobalAssetIdSource = Message.substr(Split6,Split7-Split6);
			//QSF_LOG_PRINTS(INFO," a "<< MP.GlobalAssetIdTarget << " b " << MP.GlobalAssetIdSource)
			//QSF_LOG_PRINTS(INFO," c "<< MP.PrototypeIdSource << " d " << MP.PrototypeIdTarget)
			/*QSF_LOG_PRINTS(INFO,"")
				QSF_LOG_PRINTS(INFO, "GlobalAssetIdTarget "<< MP.GlobalAssetIdTarget)
				QSF_LOG_PRINTS(INFO, "GlobalAssetIdSource "<< MP.GlobalAssetIdSource)
				QSF_LOG_PRINTS(INFO, "PrototypeIdSource "<< MP.PrototypeIdSource)
				QSF_LOG_PRINTS(INFO, "PrototypeIdTarget "<< MP.PrototypeIdTarget)*/
				for (auto a : MissingPrototypes)
				{
					if(a.GlobalAssetIdSource == MP.GlobalAssetIdSource && a.GlobalAssetIdTarget == MP.GlobalAssetIdTarget && a.PrototypeIdSource == MP.PrototypeIdSource && a.PrototypeIdTarget == a.PrototypeIdTarget)
					return;
				}
			MissingPrototypes.push_back(MP);
			UpdateMissingPrototypes();
		}

		QTreeWidgetItem * DebugUnitView::GetItemAndClearTree(std::string RootName)
		{
			QTreeWidgetItem *item = nullptr;
			for (int i = 0; i < mUiDebugUnitView->treeWidget->topLevelItemCount(); ++i)
			{

				item = mUiDebugUnitView->treeWidget->topLevelItem(i);
				if (item->data(0, 0).toString().toStdString() == RootName)
				{
					break;
				}
				// Do something with item ...
			}
			if (item == nullptr)
				return nullptr;
			for (size_t t = item->childCount(); t > 0; t--)
			{
				item->removeChild(item->takeChild((int)t - 1));
			}
			return item;
		}





		bool DebugUnitView::JsonContainsPrototypeFaster(std::string path)
		{
			bool FoundAnything = false;
			int xt = 0;
			try
			{
				boost::property_tree::ptree	mRootTree;
				qsf::FileStream file(path, qsf::File::READ_MODE);
				//QSF_LOG_PRINTS(INFO,"path "<< path)
				qsf::FileHelper::readJson(file, mRootTree);

				for (boost::property_tree::ptree::value_type &stationInfos : mRootTree.get_child("Prototypes"))
				{
					xt++;
					//QSF_LOG_PRINTS(INFO, stationInfos.first)
					for (size_t t = 0; t < MissingComponents.size(); t++)
					{
						if (MissingComponents.at(t).Prefab != "")
							continue;
						if (stationInfos.first == boost::lexical_cast<std::string>(MissingComponents.at(t).PrototTypeId))
						{
							MissingComponents.at(t).Prefab = path;
							//QSF_LOG_PRINTS(INFO, "found")
								//return true;
							FoundAnything = true;
							BOOST_FOREACH(boost::property_tree::ptree::value_type const&v, stationInfos.second) {
								if (qsf::StringHash(v.first) == boost::lexical_cast<uint32>(MissingComponents.at(t).ComponentId))
								{
									MissingComponents.at(t).BrokenComponentName = v.first;
									//QSF_LOG_PRINTS(INFO, v.first);
								}
							}
						}
					}



				}
			}

			catch (const std::exception&) {
				//QSF_LOG_PRINTS(INFO, e.what())
			}
			//QSF_LOG_PRINTS(INFO,xt);
			return FoundAnything;
		}

		bool DebugUnitView::JsonContainsPrototypeSingleSearch(std::string path,uint64 prototype)
		{
			std::string proto = boost::lexical_cast<std::string>(prototype);
			try
			{
				boost::property_tree::ptree	mRootTree;
				qsf::FileStream file(path, qsf::File::READ_MODE);
				qsf::FileHelper::readJson(file, mRootTree);

				for (boost::property_tree::ptree::value_type &stationInfos : mRootTree.get_child("Prototypes"))
				{
						if (stationInfos.first == proto)
						{
							return true;
						}
					}
			}

			catch (const std::exception&) {
				//QSF_LOG_PRINTS(INFO, e.what())
			}
			//QSF_LOG_PRINTS(INFO,xt);
			return false;
		}


		bool DebugUnitView::JsonContainsBrokenMesh(std::string path)

		{
			//QSF_LOG_PRINTS(INFO, path)
			bool FoundAnything = false;
			int xt = 0;
			try
			{
				boost::property_tree::ptree	mRootTree;
				qsf::FileStream file(path, qsf::File::READ_MODE);
				//QSF_LOG_PRINTS(INFO,"path "<< path)
				qsf::FileHelper::readJson(file, mRootTree);

				for (boost::property_tree::ptree::value_type &stationInfos : mRootTree.get_child("Prototypes"))
				{
					//Prototypes -> MeshComponent<->SkinnableMeshComponent <--> TintableMeshComponent
					auto MC = stationInfos.second.get_child_optional("qsf::MeshComponent");
					auto MC2 = stationInfos.second.get_child_optional("qsf::compositing::SkinnableMeshComponent");
					auto MC3 = stationInfos.second.get_child_optional("qsf::compositing::TintableMeshComponent");
					bool InList = false;
					if (MC.get_ptr() == nullptr)
					{
						if(MC2.get_ptr() != nullptr)
						MC== MC2;
						else if(MC3.get_ptr() != nullptr)
						{
							MC = MC3;
						}
					}
					
					if (MC.get_ptr() != nullptr)
					{
						for (size_t t = 0; t < BrokenMeshs.size(); t++)
						{
							std::string defaultval = "";
							BOOST_FOREACH(boost::property_tree::ptree::value_type const&ve, MC.get()) {
							if(ve.first == "Mesh")
								defaultval = ve.second.get_value<std::string>();
							}
							
							if (BrokenMeshs.at(t).CurrentFaultyMesh == defaultval)
							{
								if (!BrokenMeshAllreadyInList((int)t, BrokenMeshs.at(t).CurrentFaultyMesh,stationInfos.first))
								{
									auto pair = std::pair<std::string,std::string>(path,stationInfos.first);
									BrokenMeshs.at(t).PrefabUsingIt.push_back(pair);
								}
							}
						}
					}
				}
			}
			catch (const std::exception&) {
				//QSF_LOG_PRINTS(INFO, e.what())
			}
			//QSF_LOG_PRINTS(INFO,xt);
			return FoundAnything;
		}

		bool DebugUnitView::MaterialContainsPrototypeFaster(std::string path)
		{
			//QSF_LOG_PRINTS(INFO, path)
			bool FoundAnything = false;
			int xt = 0;
			try
			{
				boost::property_tree::ptree	mRootTree;
				qsf::FileStream file(path, qsf::File::READ_MODE);
				//QSF_LOG_PRINTS(INFO,"path "<< path)
				qsf::FileHelper::readJson(file, mRootTree);

				BOOST_FOREACH(boost::property_tree::ptree::value_type const&v, mRootTree.get_child("Material")) {
					for (size_t t = 0; t < MissingTextures.size(); t++)
					{
						if (qsf::StringHash(v.second.data() == MissingTextures.at(t).GlobalAssetId))
						{
							MissingTextures.at(t).MaterialName = path;
							FoundAnything = true;
							MissingTextures.at(t).MaterialId = v.first;
							//QSF_LOG_PRINTS(INFO, v.first);
						}
					}


				}
			}

			catch (const std::exception&) {
				//QSF_LOG_PRINTS(INFO, e.what())
			}
			//QSF_LOG_PRINTS(INFO,xt);
			return FoundAnything;
		}

		bool DebugUnitView::BrokenMeshAllreadyInList(int iterator, std::string CompareString,std::string Prototype)
		{
			//QSF_LOG_PRINTS(INFO, "success");
			for (size_t j = 0; j < BrokenMeshs.at(iterator).PrefabUsingIt.size(); j++)
			{
				if (BrokenMeshs.at(iterator).PrefabUsingIt.at(j).first == CompareString && BrokenMeshs.at(iterator).PrefabUsingIt.at(j).second == Prototype)
					return true;
			}
			return false;
		}

		bool user::editor::DebugUnitView::FindPrototypeAndReferencesFromPrefabs(std::string path)

			{
			
				//notice this might needs some fine tuning
				bool FoundAnything = false;
				int xt = 0;
				try
				{
					boost::property_tree::ptree	mRootTree;
					qsf::FileStream file(path, qsf::File::READ_MODE);
					//QSF_LOG_PRINTS(INFO,"path "<< path)
					qsf::FileHelper::readJson(file, mRootTree);

					for (boost::property_tree::ptree::value_type &stationInfos : mRootTree.get_child("Prototypes"))
					{
						//we are just interested in lining prototypes
						auto prefabId = stationInfos.second.get<std::string>("qsf::MetadataComponent.Prefab","18446744073709551615");
						auto ProtoId = stationInfos.second.get<std::string>("qsf::MetadataComponent.Prototype", "");
						if(prefabId == "18446744073709551615")
						continue;
						auto AssetName = GetLocalAssetName(prefabId);
						if(AssetName == "" || ProtoId == "")
						continue;
						//SF_LOG_PRINTS(INFO,"prefabId" << prefabId << " " << ProtoId)
						for (size_t t = 0; t < MissingPrototypes.size(); t++)
						{
							if (MissingPrototypes.at(t).PrototypeIdSource == ProtoId)
							{

								MissingPrototypes.at(t).RefCounter_source.push_back(AssetName);
								//MissingPrototypes.at(t).ReferenceCounter_source++;
							}
							else if (MissingPrototypes.at(t).PrototypeIdTarget == ProtoId)
							{
									MissingPrototypes.at(t).RefCounter_target.push_back(AssetName);
							}
						}
					}
				}
				catch (const std::exception&) {
					//QSF_LOG_PRINTS(INFO, e.what())
				}
				//QSF_LOG_PRINTS(INFO,xt);
				return FoundAnything;
		}

		void DebugUnitView::addTreeRoot(QString name, QString description)
		{
			// QTreeWidgetItem(QTreeWidget * parent, int type = Type)
			QTreeWidgetItem *treeItem = new QTreeWidgetItem(mUiDebugUnitView->treeWidget);

			// QTreeWidgetItem::setText(int column, const QString & text)
			treeItem->setText(0, name);
			treeItem->setText(1, description);
			//addTreeChild(treeItem, "1", "yolo");
			//TrainTrackTool::addTreeChild(treeItem, QString(name + "A"), "Child_first");
			//TrainTrackTool::addTreeChild(treeItem, QString(name + "B"), "Child_second");
		}


		QTreeWidgetItem* DebugUnitView::addTreeChild(QTreeWidgetItem * parent, QString name, QString description, QString AdditionalInfos, QString BrokenComponentName)
		{
			QTreeWidgetItem *treeItem = new QTreeWidgetItem();

			// QTreeWidgetItem::setText(int column, const QString & text)
			treeItem->setText(0, name);
			treeItem->setText(1, description);
			if (AdditionalInfos != "")
				treeItem->setText(2, AdditionalInfos);
			if (BrokenComponentName != "")
				treeItem->setText(3, BrokenComponentName);
			parent->addChild(treeItem);
			return parent->child(parent->childCount() - 1); //return last child
		}

		void DebugUnitView::UpdateBrokenMeshs()
		{
			auto Item = GetItemAndClearTree("Broken Meshs (false Id)");
			if (Item == nullptr)
			{
				QSF_LOG_PRINTS(INFO,"cant resolve tree")
				return;
			}
			//QSF_LOG_PRINTS(INFO,"UpdateBrokenMeshs "<< BrokenMeshs.size())
			std::sort(BrokenMeshs.begin(), BrokenMeshs.end());//,compareByLength);
			for (auto a : BrokenMeshs)
			{
				auto node = addTreeChild(Item, a.CurrentFaultyMesh.c_str(), a.GuessWhatMightBeBetter.c_str());
				for (auto b : a.PrefabUsingIt)
				{
					addTreeChild(node, "Prefab: ", b.first.c_str(), "Prototype: ",b.second.c_str());
				}
			}
		}

		void DebugUnitView::UpdateMissingComponents()
		{
			auto Item = GetItemAndClearTree("Missing Components");
			if (Item == nullptr)
				return;

			//rebuild
			std::sort(MissingComponents.begin(), MissingComponents.end());//,compareByLength);
			for (auto a : MissingComponents)
				addTreeChild(Item, a.ComponentId.c_str(), a.PrototTypeId.c_str(), a.Prefab.c_str(), a.BrokenComponentName.c_str());
		}

		void DebugUnitView::UpdateMissingTextures()
		{
			auto Item = GetItemAndClearTree("Missing Textures");
			if (Item == nullptr)
				return;
			//rebuild
			std::sort(MissingTextures.begin(), MissingTextures.end());//,compareByLength);
			for (auto a : MissingTextures)
				addTreeChild(Item, a.GlobalAssetId.c_str(), a.MaterialId.c_str(), a.MaterialName.c_str());
		}

		void user::editor::DebugUnitView::UpdateMissingPrototypes()
		{
			auto Item = GetItemAndClearTree("Missing Prototypes");
			if (Item == nullptr)
				return;
			//rebuild
			std::sort(MissingPrototypes.begin(), MissingPrototypes.end());//,compareByLength);
			for (auto a : MissingPrototypes)
			{
				auto child = addTreeChild(Item, a.GlobalAssetIdSource.c_str(),a.PrototypeIdSource.c_str(),a.GlobalAssetIdTarget.c_str(),a.PrototypeIdTarget.c_str());
				for (auto b : a.RefCounter_source)
				{
					addTreeChild(child,"Source Prototype References: ",b.c_str());
				}
				for (auto b : a.RefCounter_target)
				{
					addTreeChild(child, "Target Prototype References: ", b.c_str());
				}
			}
		}

		bool user::editor::DebugUnitView::WriteDebugLineMissingPrototype(std::string& returnstring , int index)
		{
			int index2 = 0;
			for (size_t t = 0; t < MissingPrototypes.size();t++)
			{
				if (index2 == index)
				{
					auto item = MissingPrototypes.at(t);
					returnstring = "\""+item.PrototypeIdSource+"\";"+item.GlobalAssetIdSource+";\""+item.PrototypeIdTarget+"\";"+item.GlobalAssetIdTarget+"; ;\n";
					return true;
				}
				index2++;
				for (size_t j = 0; j < MissingPrototypes.at(t).RefCounter_source.size(); j++)
				{
					if (index2 == index)
					{
						returnstring = " ; ; ; ; " + MissingPrototypes.at(t).RefCounter_source.at(j) +" ; \n";
						return true;
					}
					index2++;
				}
			}
			return false;
		}

		std::string user::editor::DebugUnitView::GetLocalAssetName(std::string GlobalAssetId)
		{
			try
			{
				auto proxy_2 = boost::lexical_cast<uint64>(GlobalAssetId);
				if (qsf::AssetProxy(proxy_2).getAsset() == nullptr)
				return "";
				return qsf::AssetProxy(proxy_2).getLocalAssetName();
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO,e.what());
				return "";
			}
			return "";
		}

		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
