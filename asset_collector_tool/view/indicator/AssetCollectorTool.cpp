// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/indicator/AssetCollectorTool.h"
#include "ui_AssetCollectorTool.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)


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
#include <qsf/asset/AssetSystem.h>
#include <qsf_editor/asset/AssetSystemHelper.h>
#include <qsf/asset/type/AssetType.h>
#include <qsf/asset/type/AssetTypeManager.h>

#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include "qsf_editor/selection/layer/LayerSelectionManager.h"
#include <qsf/prototype/helper/PrefabContent.h>
#include <qsf/prototype/BasePrototypeManager.h>
#include <qsf/prototype/Prototype.h>

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
#include <qsf/map/layer/LayerManager.h>
#include <qsf/map/layer/Layer.h>
#include <qsf\map\Map.h>
#include <qsf\map\Entity.h>
#include <qsf/component/link/LinkComponent.h>
#include <../qsf_compositing/qsf_compositing/component/SkinnableMeshComponent.h>
#include <../qsf_compositing/qsf_compositing/component/TintableMeshComponent.h>
#include <../qsf_compositing/qsf_compositing/component/MaterialLightAnimationComponent.h>
#include <qsf/renderer/flare/FlareComponent.h>
#include <qsf/renderer/light/LightComponent.h>
#include <qsf_game/component/base/LightControllerComponent.h>
#include <qsf/renderer/light/LightAnimationComponent.h>

#include <qsf\worker\ThreadPool.h>
#include <QtWidgets\qmessagebox.h>

#include<iostream>
#include<future>
#include<exception>
#include <QtWidgets\qprogressbar.h>
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{
		std::string AssetCollectorTool::m_Cat = "";
		bool AssetCollectorTool::m_DeepScan = true;
		std::string AssetCollectorTool::m_IgnoreList ="";
		using boost::lexical_cast;
		using boost::bad_lexical_cast;
		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		const uint32 AssetCollectorTool::PLUGINABLE_ID = qsf::StringHash("qsf::editor::AssetCollectorTool");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		AssetCollectorTool::AssetCollectorTool(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
			View(viewManager, qWidgetParent),
			mUiAssetCollectorTool(nullptr),
			mAssetEditHelper(nullptr)
		{
			// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
			addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);

		}

		AssetCollectorTool::~AssetCollectorTool()
		{
			// Destroy the UI view instance
			if (nullptr != mUiAssetCollectorTool)
			{
				delete mUiAssetCollectorTool;
			}
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		void AssetCollectorTool::retranslateUi()
		{
			// Retranslate the content of the UI, this automatically clears the previous content
			mUiAssetCollectorTool->retranslateUi(this);
		}

		void AssetCollectorTool::changeVisibility(bool visible)
		{
			// Lazy evaluation: If the view is shown the first time, create its content
			if (visible && nullptr == mUiAssetCollectorTool)
			{
				// Setup the view content
				QWidget* contentWidget = new QWidget(this);
				{
					// Load content to widget
					mUiAssetCollectorTool = new Ui::AssetCollectorTool();
					mUiAssetCollectorTool->setupUi(contentWidget);
				}

				// Set content to view
				setWidget(contentWidget);
				// Connect Qt signals/slots
				connect(mUiAssetCollectorTool->pushButtonSelect, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
				connect(mUiAssetCollectorTool->pushButton, SIGNAL(clicked(bool)), this, SLOT(onPushCopyButton(bool)));
				connect(mUiAssetCollectorTool->pushButton_2, SIGNAL(clicked(bool)), this, SLOT(onPushHelpButton(bool)));
				//connect(mUiAssetCollectorTool->comboBoxType, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
			}
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void AssetCollectorTool::rebuildGui()
		{
			auto Project = QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackageProject();
			if (mUiAssetCollectorTool->IgnoreList->toPlainText().toStdString().find(Project->getName()) == std::string::npos)
			{
				mUiAssetCollectorTool->IgnoreList->setPlainText(mUiAssetCollectorTool->IgnoreList->toPlainText() + "\n" + QString(Project->getName().c_str()));
			}
		}

		void AssetCollectorTool::onAssetReady()
		{
			//needed for online editor
			//I dont really care to complete this function
		}




		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]
		void AssetCollectorTool::showEvent(QShowEvent* qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);

			// Perform a GUI rebuild to ensure the GUI content is up-to-date
			rebuildGui();

			// Connect Qt signals/slots
			//connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &AssetCollectorTool::onUndoOperationExecuted);
			//connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &AssetCollectorTool::onRedoOperationExecuted);
		}

		void AssetCollectorTool::hideEvent(QHideEvent* qHideEvent)
		{
			// Call the base implementation
			View::hideEvent(qHideEvent);

			// Disconnect Qt signals/slots
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &AssetCollectorTool::onUndoOperationExecuted);
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &AssetCollectorTool::onRedoOperationExecuted);
		}


		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		void AssetCollectorTool::onPushSelectButton(const bool pressed)
		{
			mUiAssetCollectorTool->textBrowser->setText("");
			mylist.clear();
			//cast the globar asset id. Use boost because this try catch construct works better then the conversion with stringstream
			std::string Content = mUiAssetCollectorTool->lineEditSelected->text().toStdString();
			uint64 GlobalAssetId;
			try
			{
				GlobalAssetId = lexical_cast<uint64>(Content);
			}
			catch (bad_lexical_cast &)
			{
				//no uint64 <-> user input is invalid
				//QSF_LOG_PRINTS(ERROR, "Your provided Global Asset Id is not a number... " << e.what())
				WorkForCategory(Content);
				//m_Cat = mUiAssetCollectorTool->lineEditSelected->text().toStdString();
				//m_DeepScan = mUiAssetCollectorTool->checkBox->isChecked();
				//m_IgnoreList = mUiAssetCollectorTool->IgnoreList->toPlainText().toStdString();
				//std::async(std::launch::async, [this] {return GiveMeCategoryStuff(mUiAssetCollectorTool); });
				//std::async(std::launch::async, [] {return GiveMeCategoryStuff(m_Cat, m_DeepScan,m_IgnoreList); });
				//std::thread t1(GiveMeCategoryStuff,m_Cat, m_DeepScan, m_IgnoreList);
				//t1.detach();
				//auto function = static_cast<int(*)(int)>(DoSthCool);
				//std::future<int>result1 =  std::async(static_cast<int(*)(int)>(DoSthCool),3,3);
				/*std::future<void>result1 =*/// std::async(std::launch::async, WorkForCategory, Content);
				return;
			}

			qsf::Asset* asset = qsf::AssetProxy(GlobalAssetId).getAsset();
			if (asset == nullptr)
			{
				QSF_LOG_PRINTS(ERROR, "your provided Global Asset Id was not found or the mod which contains this asset is not mounted")
					return;
			}
			if (!qsf::AssetProxy(GlobalAssetId).getAssetPackage()->isMounted())
			{
				//never triggered? the previous if-construct always seems to catch this case
				QSF_LOG_PRINTS(ERROR, "your provided Asset is not mounted (Mod is not loaded)")
					return;
			}
			//set the text to the UI
			mUiAssetCollectorTool->label->setText(QString(qsf::AssetProxy(GlobalAssetId).getLocalAssetName().c_str()));
			//read dependecies
			auto a = qsf::AssetDependencyCollector(GlobalAssetId);
			std::vector<uint64> CompleteList;
			a.collectUniqueRecursiveAssetDependenciesWithDeriverance(CompleteList);
			QString Text;
			if (mUiAssetCollectorTool->checkBox->isChecked())
			{
					auto scan = DeepScan(GlobalAssetId,std::vector<uint64>());


				CompleteList.insert(CompleteList.begin(), scan.begin(), scan.end());
				std::sort(CompleteList.begin(), CompleteList.end());
				CompleteList.erase(std::unique(CompleteList.begin(), CompleteList.end()), CompleteList.end());
			}


			//build the ignore list

			//filter from text
			std::vector<std::string> IgnoreModListProto;
			//mods that really exist
			//only this one is used later
			std::vector<std::string> IgnoreModListFinal;
			//Mods we have in our EM5 installation
			std::vector<std::string> Modlist;
			auto Modsystem = EM5_MOD.getMods();
			for (auto a : Modsystem)
			{
				Modlist.push_back(a.second->getName());
			}
			//not a mod but we may want to filter it
			Modlist.push_back("em5");
			std::string Input = mUiAssetCollectorTool->IgnoreList->toPlainText().toStdString();
			//each project should be written on its own line e.g.
			/*
			em5
			Feuerwehr Wuppertal
			Mod 3
			...
			*/
			// and we put them in a vector by splitting when we read the next line char \n
			boost::split(IgnoreModListProto, Input, boost::is_any_of("\n"));
			for (auto a : IgnoreModListProto)
			{
				bool found = false;
				for (auto b : Modlist)
				{
					if (b == a)
					{
						IgnoreModListFinal.push_back(a);
						found = true;
						continue;
					}
				}
				if (!found)
					QSF_LOG_PRINTS(INFO, "The mod " << a << " on your ignore list was not found");
			}


			//now go through all assets
			//this is just for the Log
			int CopiedCounter = 0;
			int IgnoreCounter = 0;
			//we do not only count but also log the asset numbers 
			std::vector<uint64> MissingAssetsCounter;
			for (auto b : CompleteList)

			{
				std::stringstream ss;
				ss << b;
				if (qsf::AssetProxy(b).getAsset() == nullptr)
				{
					MissingAssetsCounter.push_back(b);
					continue;
				}
				auto Package = qsf::AssetProxy(b).getAssetPackage();
				auto ProjectName = Package->getProject().getName();
				Text += QString(ss.str().c_str()) + " " + QString(qsf::AssetProxy(b).getLocalAssetName().c_str()) + " (" + QString(ProjectName.c_str()) + ")\n";
				//now check if it is ignored
				bool found = false;
				for (auto a : IgnoreModListFinal)
				{
					if (a == ProjectName)
					{
						found = true;
						IgnoreCounter++;
						continue;
					}
				}
				if (!found)
				{
					mylist.push_back(b);
					CopiedCounter++;
				}
			}

			//how many missing assets
			if (MissingAssetsCounter.empty())
			{
				QSF_LOG_PRINTS(INFO, "There are no missing assets");
			}
			else
			{
				std::stringstream st;
				st << MissingAssetsCounter.size();
				std::string MissingAssets = "We found " + st.str() + " missing assets: ";
				for (auto a : MissingAssetsCounter)
				{
					std::stringstream ss;
					ss << a;
					MissingAssets += ss.str() + ", ";
				}
				QSF_LOG_PRINTS(INFO, MissingAssets)
			}
			// ignored assets
			QSF_LOG_PRINTS(INFO, IgnoreCounter << " assets will be ignored")
				// copy counter
				QSF_LOG_PRINTS(INFO, CopiedCounter << " assets will be copied")
				mUiAssetCollectorTool->textBrowser->setText(Text);

		}

		void AssetCollectorTool::onPushCopyButton(const bool pressed)
		{
			if (mylist.empty())
			{
				QSF_LOG_PRINTS(INFO, "list is empty")
					return;
			}
			//get destination Project
			auto Project = QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackageProject();
			//this create stuff may be removed
			/*QString AssetpackageToCopy = mUiAssetCollectorTool->lineEdit_Asset_Package->text();
			if (!AssetpackageToCopy.isEmpty())
			{
				if (Project->getAssetPackageByName(AssetpackageToCopy.toStdString()) == nullptr)
				{
					Project->createAssetPackageByName(AssetpackageToCopy.toStdString());
					Project->addAssetPackageByName(AssetpackageToCopy.toStdString());
				}
			}*/
			QSF_LOG_PRINTS(INFO, "Start to copy into the Project: " << Project->getName())
				QSF_ASSERT(nullptr == mAssetEditHelper, "KC copy asset tool: There's already an asset edit helper instance, this can't be", QSF_REACT_NONE);
			// Try to get the locks for the assets
			QApplication::setOverrideCursor(Qt::WaitCursor);
			mAssetEditHelper = std::shared_ptr<qsf::editor::AssetEditHelper>(new qsf::editor::AssetEditHelper());
			int Counter =0;
			for (auto a : mylist)
			{

				std::string OrginalPackage;
				if (qsf::AssetProxy(a).getAsset() == nullptr)
				{
					continue;
				}
				OrginalPackage = qsf::AssetProxy(a).getAssetPackage()->getName();
				qsf::Asset* asset = nullptr;
				/*if (AssetpackageToCopy.isEmpty())
				{*/
				//QSF_LOG_PRINTS(INFO, qsf::AssetProxy(a).getLocalAssetName());
				asset = mAssetEditHelper->duplicateAsset(a, OrginalPackage, nullptr, a);
				if (asset == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "[KC] The Assetcollector tool was not able to copy the orginal asset: " << qsf::AssetProxy(a).getLocalAssetName() << " with global asset id: " << qsf::AssetProxy(a).getGlobalAssetId());
					continue;
				}
				/*}
				else
				{
					asset = mAssetEditHelper->duplicateAsset(a, AssetpackageToCopy.toStdString().c_str(), nullptr, a);
				}*/

				mAssetEditHelper->callWhenReady(boost::bind(&AssetCollectorTool::onAssetReady, this));

				//Duno maybe there is some more stuff for online editor but it isnt too clear for me
				//mAssetEditHelper->getIntermediateAssetPackage()->mount();
				//mAssetEditHelper->setAssetDatasource(a, "temp");
				//mAssetEditHelper->submit();

				mAssetEditHelper->getIntermediateAssetPackage()->unmount();

				// Finalize the fire material asset editing
				mAssetEditHelper->setAssetUploadData(asset->getGlobalAssetId(), true, true);

				mAssetEditHelper->submit();

				mAssetEditHelper.reset(new qsf::editor::AssetEditHelper());
				Counter++;
				if (Counter % 20 == 0)
				{
					QSF_LOG_PRINTS(INFO,"copied "<< Counter <<" Assets of "<< mylist.size())
				}

			}
			//Helper->submit();
			QSF_LOG_PRINTS(INFO, "done")
				QApplication::restoreOverrideCursor();
		}

		void AssetCollectorTool::onPushHelpButton(const bool pressed)
		{
			QSF_PLATFORM.openUrlExternal("https://em-hub.de//forum//thread//115-asset-collector-tool-download//");
		}

		int AssetCollectorTool::DoSthCool(int a, int b)
		{
			QSF_LOG_PRINTS(INFO,"I m here")
			return 0;
		}

		std::vector<uint64> AssetCollectorTool::GiveMeCategoryStuff(std::string category, bool DeepScan2,std::string IgnoreList)
		{
			std::vector<uint64> m_list;
			auto label = "Category: " + category;
			//mUiAssetCollectorTool22->label->setText(QString(label.c_str()));
			auto Assets = ReadCategory(category);
			if (Assets.empty())
			{
				QSF_LOG_PRINTS(INFO, "Category was found but has no prefabs within " << category)
					return m_list;
			}
			/*if (mUiAssetCollectorTool22->checkBox->isChecked())
			{
				int Rechner = (int)Assets.size() / 10;
				std::string text = "Scanning categories with deep scan gives best results but take pretty long like 10-20 Assets per Minute. You have " + boost::lexical_cast<std::string>(Assets.size()) + " Assets (" + boost::lexical_cast<std::string>(Rechner) + " mins). Editor will be frozen but you can look at your logfile";
				auto EM = new QMessageBox(nullptr);
				EM->setText(text.c_str());
				EM->setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
				int Option = EM->exec();
				if (Option != 1024) ///cancel
					return m_list;
			}*/
			std::vector<uint64> CompleteList = ReadAssetDependeciesAndFilter(Assets);
			//copy and paste
			if (DeepScan2)
			{
				std::vector<uint64> AllreadyScannedMaterials;
				std::vector<uint64> NewList;
				QSF_LOG_PRINTS(INFO, "Deepscan: we have " << Assets.size() << "assets")
					int i = 0;
				for (auto a : Assets)
				{

					auto scan = DeepScan(a->getGlobalAssetId(), AllreadyScannedMaterials);
					NewList.insert(NewList.begin(), scan.begin(), scan.end());
					i++;
					QSF_LOG_PRINTS(INFO, "scan done for " << i << " of " << Assets.size());
					//mUiAssetCollectorTool22->textBrowser->setText(boost::lexical_cast<std::string>(i).c_str());
					//QBar->setValue(i);
				}
				//insert and sort
				std::sort(NewList.begin(), NewList.end());
				NewList.erase(std::unique(NewList.begin(), NewList.end()), NewList.end());

				CompleteList.insert(CompleteList.begin(), NewList.begin(), NewList.end());
				std::sort(CompleteList.begin(), CompleteList.end());
				CompleteList.erase(std::unique(CompleteList.begin(), CompleteList.end()), CompleteList.end());
			}

			QString Text;
			//filter from text
			std::vector<std::string> IgnoreModListProto;
			//mods that really exist
			//only this one is used later
			std::vector<std::string> IgnoreModListFinal;
			//Mods we have in our EM5 installation
			std::vector<std::string> Modlist;
			auto Modsystem = EM5_MOD.getMods();
			for (auto a : Modsystem)
			{
				Modlist.push_back(a.second->getName());
			}
			//not a mod but we may want to filter it
			Modlist.push_back("em5");
			//std::string Input = mUiAssetCollectorTool22->IgnoreList->toPlainText().toStdString();
			std::string Input = IgnoreList;
			//each project should be written on its own line e.g.
			/*
			em5
			Feuerwehr Wuppertal
			Mod 3
			...
			*/
			// and we put them in a vector by splitting when we read the next line char \n
			boost::split(IgnoreModListProto, Input, boost::is_any_of("\n"));
			for (auto a : IgnoreModListProto)
			{
				bool found = false;
				for (auto b : Modlist)
				{
					if (b == a)
					{
						IgnoreModListFinal.push_back(a);
						found = true;
						continue;
					}
				}
				if (!found)
					QSF_LOG_PRINTS(INFO, "The mod " << a << " on your ignore list was not found");
			}


			//now go through all assets
			//this is just for the Log
			int CopiedCounter = 0;
			int IgnoreCounter = 0;
			//we do not only count but also log the asset numbers 
			std::vector<uint64> MissingAssetsCounter;
			for (auto b : CompleteList)

			{
				std::stringstream ss;
				ss << b;
				if (qsf::AssetProxy(b).getAsset() == nullptr)
				{
					if (b == qsf::getUninitialized<uint64>()) //happens because of textutres and material deepscan - just means nothing set
						continue;
					MissingAssetsCounter.push_back(b);
					continue;
				}
				auto Package = qsf::AssetProxy(b).getAssetPackage();
				auto ProjectName = Package->getProject().getName();
				Text += QString(ss.str().c_str()) + " " + QString(qsf::AssetProxy(b).getLocalAssetName().c_str()) + " (" + QString(ProjectName.c_str()) + ")\n";
				//now check if it is ignored
				bool found = false;
				for (auto a : IgnoreModListFinal)
				{
					if (a == ProjectName)
					{
						found = true;
						IgnoreCounter++;
						continue;
					}
				}
				if (!found)
				{
					m_list.push_back(b);
					CopiedCounter++;
				}
			}

			//how many missing assets
			if (MissingAssetsCounter.empty())
			{
				QSF_LOG_PRINTS(INFO, "There are no missing assets");
			}
			else
			{
				std::stringstream st;
				st << MissingAssetsCounter.size();
				std::string MissingAssets = "We found " + st.str() + " missing assets: ";
				for (auto a : MissingAssetsCounter)
				{
					std::stringstream ss;
					ss << a;
					MissingAssets += ss.str() + ", ";
				}
				QSF_LOG_PRINTS(INFO, MissingAssets)
			}
			// ignored assets
			QSF_LOG_PRINTS(INFO, IgnoreCounter << " assets will be ignored")
				// copy counter
				QSF_LOG_PRINTS(INFO, CopiedCounter << " assets will be copied")
				//mUiAssetCollectorTool22->textBrowser->setText(Text);
			return m_list;
		}

		void AssetCollectorTool::WorkForCategory(std::string category)
		{
			auto label = "Category: " + category;
			mUiAssetCollectorTool->label->setText(QString(label.c_str()));
			auto Assets = ReadCategory(category);
			if (Assets.empty())
			{
				QSF_LOG_PRINTS(INFO, "Category was found but has no prefabs within " << category)
				return;
			}
			if (mUiAssetCollectorTool->checkBox->isChecked())
			{
			int Rechner = (int)Assets.size() /10;
			std::string text="Scanning categories with deep scan gives best results but take pretty long like 10-20 Assets per Minute. You have "+boost::lexical_cast<std::string>(Assets.size())+" Assets ("+boost::lexical_cast<std::string>(Rechner)+" mins). Editor will be frozen but you can look at your logfile";
			auto EM = new QMessageBox(nullptr);
			EM->setText(text.c_str());
			EM->setStandardButtons(QMessageBox::Ok  | QMessageBox::Cancel);
			int Option = EM->exec();
			if(Option != 1024) ///cancel
				return;
			}
			std::vector<uint64> CompleteList = ReadAssetDependeciesAndFilter(Assets);
			//copy and paste
			if (mUiAssetCollectorTool->checkBox->isChecked())
			{	
				std::vector<uint64> AllreadyScannedMaterials;
				std::vector<uint64> NewList;
				QSF_LOG_PRINTS(INFO,"Deepscan: we have "<< Assets.size() << "assets")
				int i = 0;
				for (auto a : Assets)
				{
					
					auto scan = DeepScan(a->getGlobalAssetId(), AllreadyScannedMaterials);
					NewList.insert(NewList.begin(), scan.begin(), scan.end());
					i++;
					QSF_LOG_PRINTS(INFO,"scan done for "<<i <<" of "<< Assets.size());
					mUiAssetCollectorTool->textBrowser->setText(boost::lexical_cast<std::string>(i).c_str());
				}
				//insert and sort
				std::sort(NewList.begin(), NewList.end());
				NewList.erase(std::unique(NewList.begin(), NewList.end()), NewList.end());

				CompleteList.insert(CompleteList.begin(),NewList.begin(),NewList.end());
				std::sort(CompleteList.begin(), CompleteList.end());
				CompleteList.erase(std::unique(CompleteList.begin(), CompleteList.end()), CompleteList.end());
			}

			QString Text;
			//filter from text
			std::vector<std::string> IgnoreModListProto;
			//mods that really exist
			//only this one is used later
			std::vector<std::string> IgnoreModListFinal;
			//Mods we have in our EM5 installation
			std::vector<std::string> Modlist;
			auto Modsystem = EM5_MOD.getMods();
			for (auto a : Modsystem)
			{
				Modlist.push_back(a.second->getName());
			}
			//not a mod but we may want to filter it
			Modlist.push_back("em5");
			std::string Input = mUiAssetCollectorTool->IgnoreList->toPlainText().toStdString();
			//each project should be written on its own line e.g.
			/*
			em5
			Feuerwehr Wuppertal
			Mod 3
			...
			*/
			// and we put them in a vector by splitting when we read the next line char \n
			boost::split(IgnoreModListProto, Input, boost::is_any_of("\n"));
			for (auto a : IgnoreModListProto)
			{
				bool found = false;
				for (auto b : Modlist)
				{
					if (b == a)
					{
						IgnoreModListFinal.push_back(a);
						found = true;
						continue;
					}
				}
				if (!found)
					QSF_LOG_PRINTS(INFO, "The mod " << a << " on your ignore list was not found");
			}


			//now go through all assets
			//this is just for the Log
			int CopiedCounter = 0;
			int IgnoreCounter = 0;
			//we do not only count but also log the asset numbers 
			std::vector<uint64> MissingAssetsCounter;
			for (auto b : CompleteList)

			{
				std::stringstream ss;
				ss << b;
				if (qsf::AssetProxy(b).getAsset() == nullptr)
				{
					if(b == qsf::getUninitialized<uint64>()) //happens because of textutres and material deepscan - just means nothing set
					continue;
					MissingAssetsCounter.push_back(b);
					continue;
				}
				auto Package = qsf::AssetProxy(b).getAssetPackage();
				auto ProjectName = Package->getProject().getName();
				Text += QString(ss.str().c_str()) + " " + QString(qsf::AssetProxy(b).getLocalAssetName().c_str()) + " (" + QString(ProjectName.c_str()) + ")\n";
				//now check if it is ignored
				bool found = false;
				for (auto a : IgnoreModListFinal)
				{
					if (a == ProjectName)
					{
						found = true;
						IgnoreCounter++;
						continue;
					}
				}
				if (!found)
				{
					mylist.push_back(b);
					CopiedCounter++;
				}
			}

			//how many missing assets
			if (MissingAssetsCounter.empty())
			{
				QSF_LOG_PRINTS(INFO, "There are no missing assets");
			}
			else
			{
				std::stringstream st;
				st << MissingAssetsCounter.size();
				std::string MissingAssets = "We found " + st.str() + " missing assets: ";
				for (auto a : MissingAssetsCounter)
				{
					std::stringstream ss;
					ss << a;
					MissingAssets += ss.str() + ", ";
				}
				QSF_LOG_PRINTS(INFO, MissingAssets)
			}
			// ignored assets
			QSF_LOG_PRINTS(INFO, IgnoreCounter << " assets will be ignored")
				// copy counter
				QSF_LOG_PRINTS(INFO, CopiedCounter << " assets will be copied")
				mUiAssetCollectorTool->textBrowser->setText(Text);

		}

		

	std::vector<uint64> AssetCollectorTool::ReadAssetDependeciesAndFilter(qsf::Assets AssetsIn)
	{
		std::vector<uint64> CompleteList;
		QSF_LOG_PRINTS(INFO,"Scanning for "<< AssetsIn.size()<< " Assets ")
		int i =0;
		for (auto a : AssetsIn)
		{
			auto ADC = qsf::AssetDependencyCollector(a->getGlobalAssetId());
			std::vector<uint64> EntityList;
			ADC.collectUniqueRecursiveAssetDependenciesWithDeriverance(EntityList);
			CompleteList.insert(CompleteList.end(), EntityList.begin(), EntityList.end());
			i++;
			if(i % 10 == 0)
			QSF_LOG_PRINTS(INFO,"scanned "<< i <<" assets")
		}
		//thats a lot of work so filter now double stuff out
		std::sort(CompleteList.begin(), CompleteList.end());
		CompleteList.erase(std::unique(CompleteList.begin(), CompleteList.end()), CompleteList.end());
		return CompleteList;
	}

	qsf::Assets AssetCollectorTool::ReadCategory(std::string category)
	{
		std::vector<std::string> cats;
		qsf::editor::AssetSystemHelper().getAssetCategories(cats);
		bool match = false;
		for (auto a : cats)
		{
			if (a == category)
			{
				//QSF_LOG_PRINTS(INFO,"match of category name")
				match = true;
			}
		}
		if (!match)
		{
			QSF_LOG_PRINTS(INFO, "no match of category name - returning now.... valid categories are")
				for (auto a : cats)
				{
					QSF_LOG_PRINTS(INFO, a);
				}
			return qsf::Assets();
		}
		match = false;
		uint64 TypeId = 0;
		for (auto a : QSF_ASSET.getAssetTypeManager().getAssetTypeMap()) //find asset type
		{
			if ("prefab" == a.second->getTypeName())
			{
				TypeId = a.second->getTypeId();
				match = true;
			}
		}
		if (!match)
		{
			QSF_LOG_PRINTS(INFO, "prefab category is not here? Unknown how to fix")
				return qsf::Assets();
		}
		qsf::Assets AssetList;
		QSF_ASSET.getAssetsOfType(TypeId, AssetList, &category);
		return AssetList;
	}

	std::vector<uint64> AssetCollectorTool::DeepScan(uint64 PrefabGlobalAssetId, std::vector<uint64>& AllreadyScannedMaterials)
	{
		std::vector<uint64> ReturnVec;
		std::vector<uint64> Materials;
		if (qsf::AssetProxy(PrefabGlobalAssetId).getAsset() == nullptr || qsf::AssetProxy(PrefabGlobalAssetId).getAsset()->getTypeName() != "prefab")
		{
			return ReturnVec;
		}
		/*qsf::Entity* Entity = QSF_MAINMAP.getEntityById(BuildEntity(PrefabGlobalAssetId));
		if(Entity == nullptr)
		return ReturnVec;
		//now get all childs
		auto proxy = qsf::AssetProxy(PrefabGlobalAssetId);
		if (proxy.getAsset() == nullptr)
			return ReturnVec;
		if(proxy.getAsset()->getTypeName() != "prefab")
			return ReturnVec;
		qsf::BasePrototypeManager::UniqueIdMap uniqueIdMap;
		qsf::Prototype* prototype = QSF_MAINPROTOTYPE.getPrefabByLocalAssetId(qsf::StringHash(proxy.getLocalAssetName()));
		if (prototype == nullptr)
		{
			return ReturnVec;
		}
		QSF_MAINPROTOTYPE.buildIdMatchingMapWithGeneratedIds(*prototype, uniqueIdMap, nullptr, true);
		for (auto a : uniqueIdMap)
		{

		}*/
		std::vector<qsf::Entity*> entList;
		qsf::Entity* Entity = QSF_MAINMAP.getEntityById(BuildEntity(PrefabGlobalAssetId));
		if (Entity == nullptr)
			return ReturnVec;
		auto LC = Entity->getOrCreateComponent<qsf::LinkComponent>();
		//QSF_LOG_PRINTS(INFO,"Scanned Child size "<<LC->getChildLinks().size())
		for (auto a : LC->getChildLinks())
		{
			if(a != nullptr && &a->getEntity() != nullptr)
			entList.push_back(&a->getEntity());
		}
		std::vector<qsf::Entity*> ChildList;
		
		ChildList.insert(ChildList.begin(),entList.begin(),entList.end());
		entList.clear();
		while (!ChildList.empty()) //iterate through all children
		{
			entList.push_back(ChildList.at(0)); //push it to other list
			qsf::Entity* ent = ChildList.at(0);
			if (ent == nullptr)
			{
				ChildList.erase(ChildList.begin());
				continue;
			}
			auto LC = ent->getOrCreateComponent<qsf::LinkComponent>();
			auto NewChildren = LC->getChildLinks();
			for (auto a : NewChildren)
			{
				ChildList.push_back(&a->getEntity());
			}
			ChildList.erase(ChildList.begin());
		}
		entList.push_back(Entity);
		//QSF_LOG_PRINTS(INFO,"Childlistsize"<<entList.size())
		//
		for (auto a : entList)
		{
			//QSF_LOG_PRINTS(INFO,"scan child "<< a->getId())
		qsf::compositing::SkinnableMeshComponent* SKM = a->getComponent<qsf::compositing::SkinnableMeshComponent>();
		if (SKM != nullptr)
		{
			
			for(auto mat: SKM->getMaterials())
				Materials.push_back(mat);
			//if (SKM->getTintPalette().getAsset() != nullptr)
				ReturnVec.push_back(SKM->getTintPalette().getGlobalAssetId());
		}

		qsf::compositing::TintableMeshComponent* TMC = a->getComponent<qsf::compositing::TintableMeshComponent>();
		if(TMC != nullptr)
		{
			//if(TMC->getTintPalette().getAsset() != nullptr)
			ReturnVec.push_back(TMC->getTintPalette().getGlobalAssetId());
		}
		qsf::FlareComponent* Flare = a->getComponent<qsf::FlareComponent>();
		if(Flare != nullptr)
		{
			//if(Flare->getMaterial().getAsset() != nullptr)
			Materials.push_back(Flare->getMaterial().getGlobalAssetId());
		}
		qsf::LightComponent* Light = a->getComponent<qsf::LightComponent>();
		if (Light != nullptr)
		{
			//if (Light->getTexture().getAsset() != nullptr)
				ReturnVec.push_back(Light->getTexture().getGlobalAssetId());
		}
		qsf::game::LightControllerComponent* LCC = a->getComponent<qsf::game::LightControllerComponent>();
		if (LCC != nullptr)
		{
			//if(LCC->getLightMaterial().getAsset() != nullptr)
				Materials.push_back(LCC->getLightMaterial().getGlobalAssetId());
		}

		qsf::compositing::MaterialLightAnimationComponent* MLAC = a->getComponent<qsf::compositing::MaterialLightAnimationComponent>();
		if (MLAC != nullptr)
		{
			//if (MLAC->getMaterial().g != nullptr)
				Materials.push_back(MLAC->getMaterial().getGlobalAssetId());
		}
		
		}
		std::sort(Materials.begin(),Materials.end());
		Materials.erase(std::unique(Materials.begin(),Materials.end()),Materials.end());
		for (auto a : Materials) //this is really slow :(
		{
				if(MaterialAllreadyScanned(AllreadyScannedMaterials,a))
				continue;
				//QSF_LOG_PRINTS(INFO,"scan material "<< a)
				auto ADC = qsf::AssetDependencyCollector(a);
				std::vector<uint64> EntityList;
				ADC.collectAllDerivedAssets(EntityList);
				ReturnVec.insert(ReturnVec.end(), EntityList.begin(), EntityList.end());
				//insert itself?
				ReturnVec.push_back(a);
				//add it to allready scanned materials
				AllreadyScannedMaterials.push_back(a);
		}
		//almost done
		//QSF_LOG_PRINTS(INFO, "sort stuff now ")
		std::sort(ReturnVec.begin(), ReturnVec.end());
		ReturnVec.erase(std::unique(ReturnVec.begin(), ReturnVec.end()), ReturnVec.end());
		//sort and delete
		//QSF_LOG_PRINTS(INFO, "deep scan done")
		QSF_MAINMAP.destroyEntityById(Entity->getId());
		return ReturnVec;
	}
	bool AssetCollectorTool::MaterialAllreadyScanned(std::vector<uint64> AllreadyScannedMaterials, uint64 TestMaterial)
	{
		if(TestMaterial == qsf::getUninitialized<uint64>())
		return true;
		for (auto a : AllreadyScannedMaterials)
		{
			if(a == TestMaterial)
			return true;
		}
		return false;
	}
	//DeepScan
	uint64 AssetCollectorTool::BuildEntity(uint64 GlobalAssetId)
	{
		//Create Unit <-> this is quite long
		auto proxy = qsf::AssetProxy(GlobalAssetId);
		if(proxy.getAsset() == nullptr)
		return qsf::getUninitialized<uint64>();
		
		qsf::Prototype* prototype = QSF_MAINPROTOTYPE.getPrefabByLocalAssetId(qsf::StringHash(proxy.getLocalAssetName()));
		if (prototype == nullptr)
		{
			return qsf::getUninitialized<uint64>();
		}

		//const uint32 selectedLayerId = qsf::StringHash(layher);
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
			usedPrefabContent->processForEntityInstancing(originalPrototypes, globalPrefabAssetId, name);
		}
		//if (usedPrefabContent->getPrototypes().at(0)->getComponent<qsf::MeshComponent>() != nullptr)
		//usedPrefabContent->getPrototypes().at(0)->destroyComponent<qsf::DebugBoxComponent>();
		// Build compound operation that creates a copy of the cloned prototypes in the map
		qsf::editor::base::CompoundOperation* compoundOperation = new qsf::editor::base::CompoundOperation();
		buildInstantiateTemporaryPrototypesOperation(*compoundOperation, usedPrefabContent->getPrototypes());
		uint64 entityId = usedPrefabContent->getMainPrototype()->getId();
		QSF_EDITOR_OPERATION.push(compoundOperation);
		return entityId;
		//Unit is created now apply transform
	}


	void AssetCollectorTool::buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes)
	{
		// Cycle through link components in forward order, so that parents are created before their children
		for (size_t index = 0; index < temporaryPrototypes.size(); ++index)
		{
			qsf::Prototype& prototype = *temporaryPrototypes[index];

			// Backup the temporary prototype
			qsf::editor::base::BackupPrototypeOperationData* backupPrototypeOperationData = new qsf::editor::base::BackupPrototypeOperationData(prototype);
			compoundOperation.addOperationData(backupPrototypeOperationData);
			auto Layermanager = &QSF_MAINMAP.getLayerManager();
			
			// Create entity
			qsf::editor::base::CreateEntityOperation* createEntityOperation = new qsf::editor::base::CreateEntityOperation(prototype.getId(), Layermanager->getBaseLayer().getId(), backupPrototypeOperationData);
			compoundOperation.pushBackOperation(createEntityOperation);

			// Is this the primary prototype?
			if (index == 0)
			{
				// Set text for compound operation
				compoundOperation.setText(createEntityOperation->getText());
			}
		}

	}

	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
} // editor
} // user
