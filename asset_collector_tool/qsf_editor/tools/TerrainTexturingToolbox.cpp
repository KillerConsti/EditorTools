// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/qsf_editor/tools/TerrainTexturingToolbox.h"
#include <asset_collector_tool\view\indicator\TerrainTexturingTool.h>
#include "ui_TerrainTexturingToolbox.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

#include <qsf_editor/EditorHelper.h>

#include <qsf/map/Map.h>
#include <qsf/map/Entity.h>
#include <qsf/QsfHelper.h>
#include <em5\plugin\Jobs.h>

#include <qsf\log\LogSystem.h>

#include <qsf\message\MessageSystem.h>
#include <QtWidgets/qfiledialog.h>
#include <qsf/plugin/PluginSystem.h>
#include "qsf/plugin/QsfAssetTypes.h"
#include "qsf/asset/Asset.h"
#include "qsf/asset/AssetSystem.h"
#include < qsf/asset/type/AssetTypeManager.h>
#include <em5/plugin/Plugin.h>
#include <QtWidgets\qmenu.h>
#include <qsf_editor/view/utility/ToolboxView.h>
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
		const uint32 TerrainTexturingToolbox::PLUGINABLE_ID = qsf::StringHash("qsf::editor::TerrainTexturingToolbox");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]


		TerrainTexturingToolbox::TerrainTexturingToolbox(qsf::editor::ToolManager * toolManager) :
			qsf::editor::Tool(toolManager),
			mUITerrainTexturingToolbox(new Ui::TerrainTexturingToolbox()),
			mMode(Set),
			mSavepath(""),
			OldTerrain(glm::vec2(-1, -1)),
			mEraseMode(true)
		{

		}

		TerrainTexturingToolbox::~TerrainTexturingToolbox()
		{
		}

		float TerrainTexturingToolbox::GetBrushRadius()
		{
			return ((float)mUITerrainTexturingToolbox->horizontalSlider_2->value() / 10.f);
		}



		TerrainTexturingToolbox::BrushShape TerrainTexturingToolbox::GetBrushShape()
		{
			switch (mUITerrainTexturingToolbox->comboBox->currentIndex())
			{
			case 0:
				return Dome;
			case 1:
				return Cone;
			case 2:
				return Circle;
			default:
				return Quad;
			}
		}

		int TerrainTexturingToolbox::GetBrushIntensity()
		{
			return (mUITerrainTexturingToolbox->horizontalSlider->value());
		}

		bool TerrainTexturingToolbox::IsInEraseMode()
		{
			return mEraseMode;
		}

		bool TerrainTexturingToolbox::MirrorX()
		{
			return mUITerrainTexturingToolbox->MirrorX->isChecked();
		}

		bool TerrainTexturingToolbox::MirrorY()
		{
			return mUITerrainTexturingToolbox->MirrorY->isChecked();
		}

		bool TerrainTexturingToolbox::onStartup(qsf::editor::ToolboxView & toolboxView)
		{

			if (mUITerrainTexturingToolbox != nullptr)

				mUITerrainTexturingToolbox->setupUi(toolboxView.widget());
			if (mUITerrainTexturingToolbox == nullptr) //shouldnt happen
				return false;
			ChangeMode(true);
			QObject::connect(mUITerrainTexturingToolbox->pushButton, SIGNAL(clicked(bool)), this, SLOT(onPushSaveMap(bool)));
			QObject::connect(mUITerrainTexturingToolbox->pushButton_2, SIGNAL(clicked(bool)), this, SLOT(onSetSaveDirectory(bool)));
			QObject::connect(mUITerrainTexturingToolbox->pushButtonSelect, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
			//connect(mUITerrainTexturingToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(onMinimumSliderChanged(int)));
			QObject::connect(mUITerrainTexturingToolbox->horizontalSlider_2, SIGNAL(valueChanged(int)), this, SLOT(onRadiusSliderChanged(int)));

			//connect(mUITerrainTexturingToolbox->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
			QObject::connect(mUITerrainTexturingToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(OnBrushIntensitySliderChanged(int)));
			QObject::connect(mUITerrainTexturingToolbox->CopyFromQSFTerrain, SIGNAL(clicked(bool)), this, SLOT(onCopyFromQSFTerrain(bool)));
			//Editmodes
			QObject::connect(mUITerrainTexturingToolbox->checkBox_erase, SIGNAL(clicked(bool)), this, SLOT(onPushEraseMode(bool)));
			QObject::connect(mUITerrainTexturingToolbox->checkBox_combined, SIGNAL(clicked(bool)), this, SLOT(onPushCombinedMode(bool)));
			QObject::connect(mUITerrainTexturingToolbox->comboBox, SIGNAL(currentIndexChanged(int)), SLOT(onChangeBrushType(int)));
			InitSavePath();
			UpdateTerrainList();

			auto TableWidget = mUITerrainTexturingToolbox->tableWidget;
			TableWidget->setRowCount(6);
			TableWidget->setColumnCount(3);
			TableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
			TableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
			TableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
			TableWidget->verticalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
			TableWidget->verticalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
			TableWidget->verticalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
			TableWidget->verticalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
			TableWidget->verticalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
			TableWidget->verticalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
			QTableWidgetItem *newItem = new QTableWidgetItem("0");
			TableWidget->setItem(0, 0, newItem);
			QTableWidgetItem *newItem2 = new QTableWidgetItem("1");
			TableWidget->setItem(1, 0, newItem2);
			QTableWidgetItem *newItem3 = new QTableWidgetItem("2");
			TableWidget->setItem(2, 0, newItem3);
			QTableWidgetItem *newItem4 = new QTableWidgetItem("3");
			TableWidget->setItem(3, 0, newItem4);
			QTableWidgetItem *newItem5 = new QTableWidgetItem("4");
			TableWidget->setItem(4, 0, newItem5);
			QTableWidgetItem *newItem6 = new QTableWidgetItem("5");
			TableWidget->setItem(5, 0, newItem6);

			mUITerrainTexturingToolbox->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
			if (!connect(mUITerrainTexturingToolbox->listWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &))))
				QSF_LOG_PRINTS(INFO, "Slot connection treewidget custom context Menu failed")
				/*if (!connect(mUITerrainTexturingToolbox->tableWidget, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(TerrainTexturingToolbox::onitemChanged(QTableWidgetItem*))))
				{
					QSF_LOG_PRINTS(INFO, "couldnt bind item changed")
				}*/
				return true;
		}

		void TerrainTexturingToolbox::retranslateUi(qsf::editor::ToolboxView & toolboxView)
		{
			mUITerrainTexturingToolbox->retranslateUi(toolboxView.widget());
		}

		void TerrainTexturingToolbox::onShutdown(qsf::editor::ToolboxView & toolboxView)
		{
			QSF_LOG_PRINTS(INFO, "TET is shutdowned")
				qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode() == editModeManager.get<TerrainTexturingTool>())
			{
				editModeManager.selectEditModeByPointer(editModeManager.getPreviousEditMode(), editModeManager.getToolWhichSelectedEditMode());
			}
			disconnect(mUITerrainTexturingToolbox->listWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));
		}





		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		void TerrainTexturingToolbox::onPushSelectButton(const bool pressed)
		{

			qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode() == editModeManager.get<TerrainTexturingTool>())
			{
				QSF_LOG_PRINTS(INFO, "Allready in editmode");
			}

			QSF_EDITOR_EDITMODE_MANAGER.selectEditMode<TerrainTexturingTool>(this);
		}


		void TerrainTexturingToolbox::onUndoOperationExecuted(const qsf::editor::base::Operation& operation)
		{

		}

		void TerrainTexturingToolbox::onRedoOperationExecuted(const qsf::editor::base::Operation& operation)
		{



		}



		void TerrainTexturingToolbox::OnBrushIntensitySliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUITerrainTexturingToolbox->horizontalSlider->value();
			mUITerrainTexturingToolbox->lineEdit->setText(ss.str().c_str());
		}

		void TerrainTexturingToolbox::onRadiusSliderChanged(const int value)
		{
			std::stringstream ss;
			ss << 1.0*mUITerrainTexturingToolbox->horizontalSlider_2->value() / 10.0;
			mUITerrainTexturingToolbox->lineEdit_5->setText(ss.str().c_str());
		}




		void TerrainTexturingToolbox::onSetSaveDirectory(const bool pressed)
		{
			QWidget* qtw = new QWidget();
			auto fileName = QFileDialog::getExistingDirectory(qtw,
				tr("Set Save Directory"), "");
			QSF_LOG_PRINTS(INFO, fileName.toStdString())
				mSavepath = fileName.toStdString();

			std::ofstream ofs(path + "plugin_settings.txt", std::ofstream::trunc);

			ofs << fileName.toStdString();

			ofs.close();

		}



		std::string TerrainTexturingToolbox::GetSavePath()
		{
			return mSavepath;
		}

		std::string TerrainTexturingToolbox::InitSavePath()
		{
			for (auto a : QSF_PLUGIN.getPlugins())
			{
				if (a->getFilename().find("asset_collector_tool.dll") != std::string::npos)
				{
					path = a->getFilename();
					path.erase(path.end() - 24, path.end());


				}
			}
			std::ifstream myfile(path + "plugin_settings.txt");
			std::string line;
			if (myfile.is_open())
			{
				while (std::getline(myfile, line))
				{
					mSavepath = line;
					QSF_LOG_PRINTS(INFO, "Savepath is" << mSavepath)
						break;

				}
				myfile.close();
			}
			else
			{
				std::ofstream myfile(path + "plugin_settings.txt");
				if (myfile.is_open())
				{
					myfile << path;
					myfile.close();
				}
			}
			return std::string();
		}


		void TerrainTexturingToolbox::SetCurrentTerrainData(std::vector<std::pair<std::string, int>> Data, int xTerrain, int yTerrain)
		{
			//Old
				/*auto ListWidget = mUITerrainTexturingToolbox->listWidget;
				if (OldTerrain.x != xTerrain && OldTerrain.y != yTerrain)
				{
					ListWidget->clear();
					for (auto d : Data)
						ListWidget->addItem(d.c_str());
					OldTerrain = glm::vec2(xTerrain, yTerrain);
				}*/
				//NEW
			auto TableWidget = mUITerrainTexturingToolbox->tableWidget;
			int row = 0;
			for (size_t t = 0; t < 6; t++)
			{
				if (Data.size() <= t)
				{
					//set items empty
					QTableWidgetItem *newItem = TableWidget->item(row, 1);
					if (newItem == nullptr)
					{
						newItem = new QTableWidgetItem("");
						TableWidget->setItem(row, 1, newItem);
					}
					else
					{
						newItem->setText("");
					}
					QTableWidgetItem *countItem = TableWidget->item(row, 2);
					if (countItem == nullptr)
					{
						countItem = new QTableWidgetItem("");
						TableWidget->setItem(row, 2, countItem);
					}
					else
					{
						countItem->setText("");
					}
				}
				else
				{
					auto a = Data.at(t);
					QTableWidgetItem *newItem = TableWidget->item(row, 1);
					if (newItem == nullptr)
					{
						newItem = new QTableWidgetItem(qsf::AssetProxy(a.first).getAsset()->getName().c_str());
						TableWidget->setItem(row, 1, newItem);
					}
					else
					{
						newItem->setText(qsf::AssetProxy(a.first).getAsset()->getName().c_str());
					}

					// update count
					QTableWidgetItem *countItem = TableWidget->item(row, 2);
					if (countItem == nullptr)
					{
						countItem = new QTableWidgetItem(boost::lexical_cast<std::string>(a.second).c_str());
						TableWidget->setItem(row, 2, countItem);
					}
					else
					{
						countItem->setText(boost::lexical_cast<std::string>(a.second).c_str());
					}
				}

				row++;
			}
		}

		void TerrainTexturingToolbox::UpdateTerrainList()
		{
			m_AssetList.clear();
			uint64 TypeId = 0;
			for (auto a : QSF_ASSET.getAssetTypeManager().getAssetTypeMap()) //find asset type
			{
				if ("material" == a.second->getTypeName())
				{
					TypeId = a.second->getTypeId();
					break;
				}
			}
			qsf::Assets AssetList;
			QSF_ASSET.getAssetsOfType(TypeId, AssetList, &std::string("terrain_layer"));

			auto ListWidget = mUITerrainTexturingToolbox->listWidget;
			ListWidget->clear();
			for (auto a : AssetList)
			{
				ListWidget->addItem(a->getName().c_str());
				ListWidget->sortItems();
				m_AssetList.push_back(std::pair<std::string, std::string>(a->getName(), qsf::AssetProxy(a->getGlobalAssetId()).getLocalAssetName()));
			}
		}

		std::string TerrainTexturingToolbox::GetLayerColor()
		{
			if (mUITerrainTexturingToolbox->listWidget->currentItem() == nullptr)
				return "";
			return GetLocalAssetNameFromBaseName(mUITerrainTexturingToolbox->listWidget->currentItem()->text().toStdString());
		}

		std::string TerrainTexturingToolbox::GetLocalAssetNameFromBaseName(std::string BaseAssetName)
		{
			for (auto a : m_AssetList)
			{
				if (a.first == BaseAssetName)
					return a.second;
			}
			return std::string();
		}


		void TerrainTexturingToolbox::onChangeBrushType(const int Type)
		{

		}

		void TerrainTexturingToolbox::onPushSaveMap(const bool pressed)
		{

			QSF_MESSAGE.emitMessage(qsf::MessageConfiguration("kc::save_heightmap"));
		}

		void TerrainTexturingToolbox::onCopyFromQSFTerrain(const bool pressed)
		{
			QSF_MESSAGE.emitMessage(qsf::MessageConfiguration("kc::CopyFromQSFTerrain"));
		}

		void TerrainTexturingToolbox::ShowContextMenu(const QPoint & pos)
		{
				auto CurrItem = mUITerrainTexturingToolbox->listWidget->currentItem();
			if (CurrItem == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "ret1")
					return;
			}
			std::string Text = CurrItem->text().toStdString();
			if (Text == "")
			{
				QSF_LOG_PRINTS(INFO, "ret2")
					return;
			}
				auto Table = mUITerrainTexturingToolbox->tableWidget;
			std::vector<std::string> LayerList;
			for (size_t t = 0; t < 6; t++)
			{
				if (Table->item((int)t, 1) == nullptr)
				{
					LayerList.push_back("");
					continue;
				}
				std::string TableContent = Table->item((int)t, 1)->text().toStdString();
				if (TableContent != "")
				{
					LayerList.push_back("Layer " + boost::lexical_cast<std::string>(t) + ": " + TableContent);
				}
				else
					LayerList.push_back("");
			}
				QMenu contextMenu("Copy Data", mUITerrainTexturingToolbox->listWidget);
				
				QAction* action_desc = new QAction("Replace Layer", mUITerrainTexturingToolbox->listWidget);
				contextMenu.addAction(action_desc);
				
				contextMenu.addSeparator();
				QAction* action1 = new QAction(LayerList.at(0).c_str(), mUITerrainTexturingToolbox->listWidget);
			if (LayerList.at(0) != "")
				contextMenu.addAction(action1);
				QAction* action2 = new QAction(LayerList.at(1).c_str(), mUITerrainTexturingToolbox->listWidget);
			if (LayerList.at(1) != "")
				contextMenu.addAction(action2);
				QAction* action3 = new QAction(LayerList.at(2).c_str(), mUITerrainTexturingToolbox->listWidget);
			if (LayerList.at(2) != "")
				contextMenu.addAction(action3);
				QAction* action4 = new QAction(LayerList.at(3).c_str(), mUITerrainTexturingToolbox->listWidget);
			if (LayerList.at(3) != "")
				contextMenu.addAction(action4);
				QAction* action5 = new QAction(LayerList.at(4).c_str(), mUITerrainTexturingToolbox->listWidget);
			if (LayerList.at(4) != "")
				contextMenu.addAction(action5);
				QAction* action6 = new QAction(LayerList.at(5).c_str(), mUITerrainTexturingToolbox->listWidget);
			if (LayerList.at(5) != "")
				contextMenu.addAction(action6);
				auto Action = contextMenu.exec(mUITerrainTexturingToolbox->listWidget->mapToGlobal(pos));
				//Replace Layer actions
				int id = -1;
				if (Action == action1)
				{
				id = 0;
				}
			if (Action == action2)
			{
				id =1;
			}
			if (Action == action3)
			{
			id = 2;
			}
			if (Action == action4)
			{
			id = 3;
			}
			if (Action == action5)
			{
			id =4;
			}
			if (Action == action6)
			{
			id= 5;
			}
			if(id == -1)
			return;
			std::string NewLayer = LayerList.at(id);
			NewLayer.erase(0,9);
				QSF_LOG_PRINTS(INFO,GetLocalAssetNameFromBaseName(NewLayer))
				qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode() == editModeManager.get<TerrainTexturingTool>())
			{
				//QSF_LOG_PRINTS(INFO,"got a nice mode")
					static_cast<TerrainTexturingTool*>(editModeManager.get<TerrainTexturingTool>())->ReplaceLayer(id, GetLocalAssetNameFromBaseName(Text));
			}
		}

		void TerrainTexturingToolbox::onPushEraseMode(const bool pressed)
		{
			ChangeMode(true);
		}

		void TerrainTexturingToolbox::onPushCombinedMode(const bool pressed)
		{
			ChangeMode(false);
		}

		void TerrainTexturingToolbox::ChangeMode(bool NewMode)
		{
			mEraseMode = NewMode;
			mUITerrainTexturingToolbox->checkBox_erase->setChecked(NewMode);
			mUITerrainTexturingToolbox->checkBox_combined->setChecked(!NewMode);
			mUITerrainTexturingToolbox->horizontalSlider->setEnabled(!NewMode);
			QSF_LOG_PRINTS(INFO,"Mode changed "<< NewMode)

		}





		TerrainTexturingToolbox::TerrainEditMode2 TerrainTexturingToolbox::GetEditMode()
		{
			return mMode;
		}






		



		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
