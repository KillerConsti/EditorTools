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
#include < qsf/asset/type/AssetTypeManager.h>
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
			}
			//Helper->submit();
			QSF_LOG_PRINTS(INFO, "done")
				QApplication::restoreOverrideCursor();
		}

		void AssetCollectorTool::onPushHelpButton(const bool pressed)
		{
			QSF_PLATFORM.openUrlExternal("http://www.emergency-forum.de/index.php?thread/66525-asset-collector-tool-for-content-creators/");
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
			std::vector<uint64> CompleteList = ReadAssetDependeciesAndFilter(Assets);
			//copy and paste

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
		for (auto a : AssetsIn)
		{
			auto ADC = qsf::AssetDependencyCollector(a->getGlobalAssetId());
			std::vector<uint64> EntityList;
			ADC.collectUniqueRecursiveAssetDependenciesWithDeriverance(EntityList);
			CompleteList.insert(CompleteList.end(), EntityList.begin(), EntityList.end());
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




	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
} // editor
} // user
