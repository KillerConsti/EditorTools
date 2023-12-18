// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/qsf_editor/tools/TerrainEditToolbox.h"
#include <asset_collector_tool\view\indicator\TerrainEditTool.h>
#include "ui_TerrainEditToolbox.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

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
		const uint32 TerrainEditToolbox::PLUGINABLE_ID = qsf::StringHash("qsf::editor::TerrainEditToolbox");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]


		TerrainEditToolbox::TerrainEditToolbox(qsf::editor::ToolManager * toolManager) :
		qsf::editor::Tool(toolManager),
			mUITerrainEditToolbox(new Ui::TerrainEditToolbox()),
			mMode(Set),
			mSavepath("")
		{
			
		}

		TerrainEditToolbox::~TerrainEditToolbox()
		{
		}

		float TerrainEditToolbox::GetBrushRadius()
		{
			return ((float)mUITerrainEditToolbox->horizontalSlider_2->value() /10.f);
		}

		float TerrainEditToolbox::GetHeight()
		{
			try
			{
			return boost::lexical_cast<float>(mUITerrainEditToolbox->lineEdit_2->text().toStdString());
			}
			catch (const std::exception&)
			{
			QSF_LOG_PRINTS(ERROR,"Cant read height")
			}
			return 0.1f;
		}

		TerrainEditToolbox::BrushShape TerrainEditToolbox::GetBrushShape()
		{
			switch (mUITerrainEditToolbox->comboBox->currentIndex())
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

		int TerrainEditToolbox::GetBrushIntensity()
		{
			return (mUITerrainEditToolbox->horizontalSlider->value());
		}

		bool TerrainEditToolbox::onStartup(qsf::editor::ToolboxView & toolboxView)
		{
		if(mUITerrainEditToolbox != nullptr)

			mUITerrainEditToolbox->setupUi(toolboxView.widget());
		if (mUITerrainEditToolbox == nullptr) //shouldnt happen
			return false;
		connect(mUITerrainEditToolbox->pushButton, SIGNAL(clicked(bool)), this, SLOT(onPushSaveMap(bool)));
		connect(mUITerrainEditToolbox->pushButton_2, SIGNAL(clicked(bool)), this, SLOT(onSetSaveDirectory(bool)));
		connect(mUITerrainEditToolbox->pushButtonSelect, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
		//connect(mUITerrainEditToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(onMinimumSliderChanged(int)));
		connect(mUITerrainEditToolbox->horizontalSlider_2, SIGNAL(valueChanged(int)), this, SLOT(onRadiusSliderChanged(int)));

		//connect(mUITerrainEditToolbox->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
		connect(mUITerrainEditToolbox->horizontalSlider, SIGNAL(valueChanged(int)),this,SLOT(OnBrushIntensitySliderChanged(int)));

		//Editmodes

		connect(mUITerrainEditToolbox->toolButton, SIGNAL(clicked(bool)), this, SLOT(onPushSetHeight(bool)));
		connect(mUITerrainEditToolbox->toolButton_2, SIGNAL(clicked(bool)), this, SLOT(onPushRaiseLower(bool)));
		connect(mUITerrainEditToolbox->toolButton_3, SIGNAL(clicked(bool)), this, SLOT(onPushSmooth(bool)));
		connect(mUITerrainEditToolbox->LoadButton, SIGNAL(clicked(bool)), this, SLOT(onPushLoadMap(bool)));
		connect(mUITerrainEditToolbox->comboBox,SIGNAL(currentIndexChanged(int)),SLOT(onChangeBrushType(int)));
		onPushSetHeight(true);
		InitSavePath();
			return true;
		}

		void TerrainEditToolbox::retranslateUi(qsf::editor::ToolboxView & toolboxView)
		{
			mUITerrainEditToolbox->retranslateUi(toolboxView.widget());
		}

		void TerrainEditToolbox::onShutdown(qsf::editor::ToolboxView & toolboxView)
		{
			QSF_LOG_PRINTS(INFO,"TET is shutdowned")
				qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode() == editModeManager.get<TerrainEditTool>())
			{
				editModeManager.selectEditModeByPointer(editModeManager.getPreviousEditMode(), editModeManager.getToolWhichSelectedEditMode());
			}
		}

		


		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		void TerrainEditToolbox::onPushSelectButton(const bool pressed)
		{

			qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode() == editModeManager.get<TerrainEditTool>())
			{
				QSF_LOG_PRINTS(INFO,"Allready in editmode");
			}

			QSF_EDITOR_EDITMODE_MANAGER.selectEditMode<TerrainEditTool>(this);
		}


		void TerrainEditToolbox::onUndoOperationExecuted(const qsf::editor::base::Operation& operation)
		{
		
		}

		void TerrainEditToolbox::onRedoOperationExecuted(const qsf::editor::base::Operation& operation)
		{
		
		
		
		}



		void TerrainEditToolbox::OnBrushIntensitySliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUITerrainEditToolbox->horizontalSlider->value();
			mUITerrainEditToolbox->lineEdit->setText(ss.str().c_str());
		}

		void TerrainEditToolbox::onRadiusSliderChanged(const int value)
		{
			std::stringstream ss;
			ss << 1.0*mUITerrainEditToolbox->horizontalSlider_2->value() / 10.0;
			mUITerrainEditToolbox->lineEdit_5->setText(ss.str().c_str());
		}

		void TerrainEditToolbox::onPushSetHeight(const bool pressed)
		{
			mUITerrainEditToolbox->toolButton->setChecked(true);
			mUITerrainEditToolbox->toolButton_2->setChecked(false);
			mUITerrainEditToolbox->toolButton_3->setChecked(false);
			mUITerrainEditToolbox->lineEdit_2->setEnabled(true);
			mUITerrainEditToolbox->horizontalSlider->setEnabled(false);
			mUITerrainEditToolbox->lineEdit->setEnabled(false);
			mMode = Set;
		}

		void TerrainEditToolbox::onPushRaiseLower(const bool pressed)
		{
				mUITerrainEditToolbox->toolButton->setChecked(false);
			mUITerrainEditToolbox->toolButton_2->setChecked(true);
			mUITerrainEditToolbox->toolButton_3->setChecked(false);
			mUITerrainEditToolbox->lineEdit_2->setEnabled(false);
			mUITerrainEditToolbox->horizontalSlider->setEnabled(true);
			mUITerrainEditToolbox->lineEdit->setEnabled(true);

			mMode = Raise;
		}

		void TerrainEditToolbox::onPushSmooth(const bool pressed)
		{
				mUITerrainEditToolbox->toolButton->setChecked(false);
			mUITerrainEditToolbox->toolButton_2->setChecked(false);
			mUITerrainEditToolbox->toolButton_3->setChecked(true);
			mUITerrainEditToolbox->lineEdit_2->setEnabled(false);
			mUITerrainEditToolbox->horizontalSlider->setEnabled(true);
			mUITerrainEditToolbox->lineEdit->setEnabled(true);
			mMode = Smooth;
		}

		void TerrainEditToolbox::onSetSaveDirectory(const bool pressed)
		{
			QWidget* qtw = new QWidget();
			auto fileName = QFileDialog::getExistingDirectory(qtw,
				tr("Set Save Directory"), "");
			QSF_LOG_PRINTS(INFO, fileName.toStdString())
			mSavepath = fileName.toStdString();

			std::ofstream ofs(path + "plugin_settings.txt",std::ofstream::trunc);

			ofs << fileName.toStdString();

			ofs.close();

		}


		std::string TerrainEditToolbox::GetSavePath()
		{
			return mSavepath;
		}

		std::string TerrainEditToolbox::InitSavePath()
		{
			for (auto a : QSF_PLUGIN.getPlugins())
			{
				if(a->getFilename().find("asset_collector_tool.dll") != std::string::npos)
				{
				path = a->getFilename();
				path.erase(path.end()-24,path.end());
				

				}
			}
			std::ifstream myfile(path + "plugin_settings.txt");
			std::string line;
			if (myfile.is_open())
			{
				while (std::getline(myfile, line))
				{
					mSavepath = line;
					QSF_LOG_PRINTS(INFO,"Savepath is"<< mSavepath)
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

		void TerrainEditToolbox::onPushLoadMap(const bool pressed)
		{
			QString filter = "File Description (*.tif)";
			auto path = mSavepath.c_str();
			auto Dialog = QFileDialog::getOpenFileName(nullptr,QString("Heightmap to open"),QString(path),filter);
			if(Dialog.size() < 1)
			return;
			std::string Dia = Dialog.toStdString();
			Dia.erase(0,std::string(path).size()+1);
			QSF_LOG_PRINTS(INFO, Dia << " path " << path);
			if(QSF_EDITOR_EDITMODE_MANAGER.getSelectedEditMode() != nullptr && QSF_EDITOR_EDITMODE_MANAGER.get<TerrainEditTool>() == QSF_EDITOR_EDITMODE_MANAGER.getSelectedEditMode())
				QSF_EDITOR_EDITMODE_MANAGER.get<TerrainEditTool>()->LoadMap(Dia,path);
		}

		void TerrainEditToolbox::onChangeBrushType(const int Type)
		{

		}

		void TerrainEditToolbox::onPushSaveMap(const bool pressed)
		{
				
			QSF_MESSAGE.emitMessage(qsf::MessageConfiguration("kc::save_heightmap"));
		}

		TerrainEditToolbox::TerrainEditMode2 TerrainEditToolbox::GetEditMode()
		{
			return mMode;
		}

		void TerrainEditToolbox::SetHeight(float NewHeight)
		{
			std::string Heightset = boost::lexical_cast<std::string>(NewHeight);
			std::string rounded = Heightset.substr(0, Heightset.find(".") + 3);
			mUITerrainEditToolbox->lineEdit_2->setText(rounded.c_str());
		}

		


		




		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
