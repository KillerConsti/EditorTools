// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/qsf_editor/tools/TerrainTexturingToolbox.h"
#include <asset_collector_tool\view\indicator\TerrainTexturingTool.h>
#include <asset_collector_tool\view\indicator\OldTerrainTexturingTool.h>
#include "ui_TerrainTexturingToolbox.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

#include <qsf_editor/operation/utility/RebuildGuiOperation.h>
#include <qsf_editor/application/manager/CameraManager.h>
#include <qsf_editor/EditorHelper.h>

#include <qsf_editor_base/operation/entity/CreateEntityOperation.h>
#include <qsf_editor_base/operation/entity/DestroyEntityOperation.h>
#include <qsf_editor_base/operation/component/CreateComponentOperation.h>
#include <qsf_editor_base/operation/component/DestroyComponentOperation.h>
#include <qsf_editor_base/operation/component/SetComponentPropertyOperation.h>

#include <qsf/map/Map.h>
#include <qsf/map/Entity.h>
#include <qsf/selection/EntitySelectionManager.h>
#include <qsf/QsfHelper.h>
#include <qsf\component\base\MetadataComponent.h>
#include <em5\plugin\Jobs.h>
#include <qsf/debug/request/CircleDebugDrawRequest.h>
#include <qsf/math/CoordinateSystem.h>
#include <qsf/debug/DebugDrawLifetimeData.h>
#include <qsf/input/InputSystem.h>
#include <qsf/input/device/MouseDevice.h>
#include <qsf/map/query/RayMapQuery.h>
#include <qsf/map/query/GroundMapQuery.h>
#include <em5/application/Application.h>
#include "em5/game/groundmap/GroundMaps.h"
#include "qsf/application/WindowApplication.h"
#include "qsf/window/WindowSystem.h"
#include <qsf/window/Window.h>
#include <qsf/renderer/window/RenderWindow.h>
#include <qsf/application/Application.h>
#include <qsf_editor/application/Application.h>
#include <qsf/renderer/RendererSystem.h>
#include <qsf/map/query/ComponentMapQuery.h>
#include <qsf/renderer/component/CameraComponent.h>
#include <qsf/renderer/utility/CameraControlComponent.h>
#include <qsf/renderer/helper/RendererHelper.h>
#include <qsf/component/base/TransformComponent.h>
#include <qsf/math/Random.h>
#include <qsf/map/layer/LayerManager.h>
#include <qsf/map/layer/Layer.h>
#include <qsf/math/EulerAngles.h>
#include <qsf_editor/map/MapHelper.h>
#include <qsf/map/EntityHelper.h>
#include <qsf/selection/SelectionManager.h>
#include <qsf_editor/application/Application.h>
#include <qsf_editor/selection/entity/EntitySelectionManager.h>


#include <qsf_editor_base/user/User.h>
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

#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include "qsf_editor/selection/layer/LayerSelectionManager.h"
#include <qsf/asset/Asset.h>
#include <qsf/prototype/helper/PrefabContent.h>
#include <qsf/prototype/BasePrototypeManager.h>
#include <qsf/input/device/KeyboardDevice.h>

#include <qsf/renderer/mesh/MeshComponent.h>
#include <qsf/renderer/debug/DebugBoxComponent.h>
#include <qsf_editor/editmode/EditMode.h>
#include <qsf_editor/renderer/RenderView.h>
#include "qsf/renderer/window/RenderWindow.h"
#include <qsf/math/Convert.h>
#include <ogre\Ogre.h>
#include <OGRE/OgreRay.h>
#include <qsf/math/Plane.h>
#include <qsf/math/Math.h>
#include <qsf_editor/asset/terrain/TerrainEditManager.h>
#include <qsf\log\LogSystem.h>

#include <qsf_editor/view/utility/ToolboxView.h>
#include <ogre\Terrain\OgreTerrainGroup.h>
#include <qsf\message\MessageSystem.h>
#include <../../plugin_api/external/qt/include/QtWidgets/qfiledialog.h>
#include <../../plugin_api/external/qt/include/QtWidgets/qcolordialog.h>
#include <qsf/plugin/PluginSystem.h>
#include "qsf/plugin/QsfAssetTypes.h"
#include "qsf/asset/Asset.h"
#include "qsf/asset/AssetSystem.h"
#include < qsf/asset/type/AssetTypeManager.h>
#include <em5/plugin/Plugin.h>
#include <../../plugin_api/external/qt/include/QtWidgets/qtablewidget.h>
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
			mColor(Qt::cyan),
			OldTerrain(glm::vec2(-1, -1))
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

		bool TerrainTexturingToolbox::onStartup(qsf::editor::ToolboxView & toolboxView)
		{
			if (mUITerrainTexturingToolbox != nullptr)

				mUITerrainTexturingToolbox->setupUi(toolboxView.widget());
			if (mUITerrainTexturingToolbox == nullptr) //shouldnt happen
				return false;
			connect(mUITerrainTexturingToolbox->pushButton, SIGNAL(clicked(bool)), this, SLOT(onPushSaveMap(bool)));
			connect(mUITerrainTexturingToolbox->pushButton_2, SIGNAL(clicked(bool)), this, SLOT(onSetSaveDirectory(bool)));
			connect(mUITerrainTexturingToolbox->pushButtonSelect, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
			//connect(mUITerrainTexturingToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(onMinimumSliderChanged(int)));
			connect(mUITerrainTexturingToolbox->horizontalSlider_2, SIGNAL(valueChanged(int)), this, SLOT(onRadiusSliderChanged(int)));

			//connect(mUITerrainTexturingToolbox->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
			connect(mUITerrainTexturingToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(OnBrushIntensitySliderChanged(int)));
			connect(mUITerrainTexturingToolbox->CopyFromQSFTerrain, SIGNAL(clicked(bool)), this, SLOT(onCopyFromQSFTerrain(bool)));
			//Editmodes


			connect(mUITerrainTexturingToolbox->comboBox, SIGNAL(currentIndexChanged(int)), SLOT(onChangeBrushType(int)));
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

		QColor TerrainTexturingToolbox::getColor()
		{
			return mColor;
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
			for(size_t t = 0; t< 6; t++)
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
			QSF_ASSET.getAssetsOfType(TypeId, AssetList,&std::string("terrain_layer"));

			auto ListWidget = mUITerrainTexturingToolbox->listWidget;
			ListWidget->clear();
			for (auto a : AssetList)
			{
				ListWidget->addItem(a->getName().c_str());
				ListWidget->sortItems();
				m_AssetList.push_back(std::pair<std::string,std::string>(a->getName(),qsf::AssetProxy(a->getGlobalAssetId()).getLocalAssetName()));
			}
		}

		std::string TerrainTexturingToolbox::GetLayerColor()
		{
			if(mUITerrainTexturingToolbox->listWidget->currentItem() == nullptr)
				return "";
			return GetLocalAssetNameFromBaseName(mUITerrainTexturingToolbox->listWidget->currentItem()->text().toStdString());
		}

		std::string TerrainTexturingToolbox::GetLocalAssetNameFromBaseName(std::string BaseAssetName)
		{
			for (auto a : m_AssetList)
			{
				if(a.first == BaseAssetName)
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

		TerrainTexturingToolbox::TerrainEditMode2 TerrainTexturingToolbox::GetEditMode()
		{
			return mMode;
		}










		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
