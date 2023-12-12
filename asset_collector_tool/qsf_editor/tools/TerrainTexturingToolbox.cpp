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

		void TerrainTexturingToolbox::SetCurrentTerrainData(std::vector<std::string> Data, int xTerrain, int yTerrain)
		{
			auto ListWidget = mUITerrainTexturingToolbox->listWidget;
			if (OldTerrain.x != xTerrain && OldTerrain.y != yTerrain)
			{
				ListWidget->clear();
				for (auto d : Data)
					ListWidget->addItem(d.c_str());
				OldTerrain = glm::vec2(xTerrain, yTerrain);
			}
		}

		std::string TerrainTexturingToolbox::GetLayerColor()
		{
			if(mUITerrainTexturingToolbox->listWidget->currentItem() == nullptr)
				return "";
			return mUITerrainTexturingToolbox->listWidget->currentItem()->text().toStdString();
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
