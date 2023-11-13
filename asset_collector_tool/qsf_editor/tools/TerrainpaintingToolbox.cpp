// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/qsf_editor/tools/TerrainpaintingToolbox.h"
#include <asset_collector_tool\view\indicator\TerrainPaintTool.h>
#include "ui_TerrainpaintingToolbox.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

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

#include <em5/plugin/Plugin.h>
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
		const uint32 TerrainpaintingToolbox::PLUGINABLE_ID = qsf::StringHash("qsf::editor::TerrainpaintingToolbox");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]


		TerrainpaintingToolbox::TerrainpaintingToolbox(qsf::editor::ToolManager * toolManager) :
			qsf::editor::Tool(toolManager),
			mUITerrainpaintingToolbox(new Ui::TerrainpaintingToolbox()),
			mMode(Set),
			mSavepath(""),
			mColor(Qt::cyan),
			OldTerrain(glm::vec2(-1, -1))
		{

		}

		TerrainpaintingToolbox::~TerrainpaintingToolbox()
		{
		}

		float TerrainpaintingToolbox::GetBrushRadius()
		{
			return ((float)mUITerrainpaintingToolbox->horizontalSlider_2->value() / 10.f);
		}



		TerrainpaintingToolbox::BrushShape TerrainpaintingToolbox::GetBrushShape()
		{
			switch (mUITerrainpaintingToolbox->comboBox->currentIndex())
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

		int TerrainpaintingToolbox::GetBrushIntensity()
		{
			return (mUITerrainpaintingToolbox->horizontalSlider->value());
		}

		bool TerrainpaintingToolbox::onStartup(qsf::editor::ToolboxView & toolboxView)
		{
			if (mUITerrainpaintingToolbox != nullptr)

				mUITerrainpaintingToolbox->setupUi(toolboxView.widget());
			if (mUITerrainpaintingToolbox == nullptr) //shouldnt happen
				return false;
			connect(mUITerrainpaintingToolbox->pushButton, SIGNAL(clicked(bool)), this, SLOT(onPushSaveMap(bool)));
			connect(mUITerrainpaintingToolbox->pushButton_2, SIGNAL(clicked(bool)), this, SLOT(onSetSaveDirectory(bool)));
			connect(mUITerrainpaintingToolbox->pushButtonSelect, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
			//connect(mUITerrainpaintingToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(onMinimumSliderChanged(int)));
			connect(mUITerrainpaintingToolbox->horizontalSlider_2, SIGNAL(valueChanged(int)), this, SLOT(onRadiusSliderChanged(int)));

			//connect(mUITerrainpaintingToolbox->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
			connect(mUITerrainpaintingToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(OnBrushIntensitySliderChanged(int)));

			//Editmodes


			connect(mUITerrainpaintingToolbox->comboBox, SIGNAL(currentIndexChanged(int)), SLOT(onChangeBrushType(int)));
			InitSavePath();
			return true;
		}

		void TerrainpaintingToolbox::retranslateUi(qsf::editor::ToolboxView & toolboxView)
		{
			mUITerrainpaintingToolbox->retranslateUi(toolboxView.widget());
		}

		void TerrainpaintingToolbox::onShutdown(qsf::editor::ToolboxView & toolboxView)
		{
			QSF_LOG_PRINTS(INFO, "TET is shutdowned")
				qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode() == editModeManager.get<TerrainPaintTool>())
			{
				editModeManager.selectEditModeByPointer(editModeManager.getPreviousEditMode(), editModeManager.getToolWhichSelectedEditMode());
			}
		}




		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		void TerrainpaintingToolbox::onPushSelectButton(const bool pressed)
		{

			qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode() == editModeManager.get<TerrainPaintTool>())
			{
				QSF_LOG_PRINTS(INFO, "Allready in editmode");
			}

			QSF_EDITOR_EDITMODE_MANAGER.selectEditMode<TerrainPaintTool>(this);
		}


		void TerrainpaintingToolbox::onUndoOperationExecuted(const qsf::editor::base::Operation& operation)
		{

		}

		void TerrainpaintingToolbox::onRedoOperationExecuted(const qsf::editor::base::Operation& operation)
		{



		}



		void TerrainpaintingToolbox::OnBrushIntensitySliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUITerrainpaintingToolbox->horizontalSlider->value();
			mUITerrainpaintingToolbox->lineEdit->setText(ss.str().c_str());
		}

		void TerrainpaintingToolbox::onRadiusSliderChanged(const int value)
		{
			std::stringstream ss;
			ss << 1.0*mUITerrainpaintingToolbox->horizontalSlider_2->value() / 10.0;
			mUITerrainpaintingToolbox->lineEdit_5->setText(ss.str().c_str());
		}




		void TerrainpaintingToolbox::onSetSaveDirectory(const bool pressed)
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


		std::string TerrainpaintingToolbox::GetSavePath()
		{
			return mSavepath;
		}

		std::string TerrainpaintingToolbox::InitSavePath()
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

		QColor TerrainpaintingToolbox::getColor()
		{
			return mColor;
		}

		void TerrainpaintingToolbox::SetCurrentTerrainData(std::vector<std::string> Data, int xTerrain, int yTerrain)
		{
			auto ListWidget = mUITerrainpaintingToolbox->listWidget;
			if (OldTerrain.x != xTerrain && OldTerrain.y != yTerrain)
			{
				ListWidget->clear();
				for (auto d : Data)
					ListWidget->addItem(d.c_str());
				OldTerrain = glm::vec2(xTerrain, yTerrain);
			}
		}

		std::string TerrainpaintingToolbox::GetLayerColor()
		{
			if(mUITerrainpaintingToolbox->listWidget->currentItem() == nullptr)
				return "";
			return mUITerrainpaintingToolbox->listWidget->currentItem()->text().toStdString();
		}

		void TerrainpaintingToolbox::onChangeBrushType(const int Type)
		{

		}

		void TerrainpaintingToolbox::onPushSaveMap(const bool pressed)
		{

			QSF_MESSAGE.emitMessage(qsf::MessageConfiguration("kc::save_heightmap"));
		}

		TerrainpaintingToolbox::TerrainEditMode2 TerrainpaintingToolbox::GetEditMode()
		{
			return mMode;
		}










		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
