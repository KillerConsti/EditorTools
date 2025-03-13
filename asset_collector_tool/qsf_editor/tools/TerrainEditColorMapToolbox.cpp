// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/qsf_editor/tools/TerrainEditColorMapToolbox.h"
#include <asset_collector_tool\view\indicator\TerrainEditmodeColorMap.h>
#include <ui_TerrainEditColorMapToolbox.h>
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
#include <QtWidgets\qinputdialog.h>
#include <asset_collector_tool\Manager\Settingsmanager.h>
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
		const uint32 TerrainEditColorMapToolbox::PLUGINABLE_ID = qsf::StringHash("qsf::editor::TerrainEditColorMapToolbox");

		TerrainEditColorMapToolbox* TerrainEditColorMapToolbox::instance = nullptr;
		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]


		TerrainEditColorMapToolbox::TerrainEditColorMapToolbox(qsf::editor::ToolManager * toolManager) :
			qsf::editor::Tool(toolManager),
			mUITerrainEditColorMapToolbox(new Ui::TerrainEditColorMapToolbox()),
			mMode(Set),
			path(""),
			mtoolboxView(nullptr),
			m_unlocked(false)
		{
			instance = this;
		}

		TerrainEditColorMapToolbox::~TerrainEditColorMapToolbox()
		{
			instance = nullptr;
		}

		float TerrainEditColorMapToolbox::GetBrushRadius()
		{
			return ((float)mUITerrainEditColorMapToolbox->horizontalSlider_2->value() / 10.f);
		}

		float TerrainEditColorMapToolbox::GetHeight()
		{
			try
			{
				return boost::lexical_cast<float>(mUITerrainEditColorMapToolbox->lineEdit_2->text().toStdString());
			}
			catch (const std::exception&)
			{
				QSF_LOG_PRINTS(ERROR, "Cant read height")
			}
			return 0.1f;
		}

		TerrainEditColorMapToolbox::BrushShape TerrainEditColorMapToolbox::GetBrushShape()
		{
			switch (mUITerrainEditColorMapToolbox->comboBox->currentIndex())
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

		int TerrainEditColorMapToolbox::GetBrushIntensity()
		{
			return (mUITerrainEditColorMapToolbox->horizontalSlider->value());
		}

		QColor TerrainEditColorMapToolbox::GetSelectedColor()
		{
			QColor originColor = mUITerrainEditColorMapToolbox->label_6->palette().color(QPalette::ColorRole::Background);
			//int r, g, b;
			//originColor.getRgb(&r, &g, &b);
			if(mUITerrainEditColorMapToolbox->label_6->text() != "Alpha")
			return originColor;
			else
			{
				return QColor(0,0,0,0);
			}
		}

		bool TerrainEditColorMapToolbox::IsUnlocked()
		{
			return m_unlocked;
		}

		bool TerrainEditColorMapToolbox::onStartup(qsf::editor::ToolboxView & toolboxView)
		{
			if (mUITerrainEditColorMapToolbox != nullptr)
			
				mUITerrainEditColorMapToolbox->setupUi(toolboxView.widget());
			mtoolboxView = &toolboxView;
			if(mUITerrainEditColorMapToolbox == nullptr) //shouldnt happen
			return false;
			connect(mUITerrainEditColorMapToolbox->pushButtonSelect, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
			//connect(mUITerrainEditColorMapToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(onMinimumSliderChanged(int)));
			connect(mUITerrainEditColorMapToolbox->horizontalSlider_2, SIGNAL(valueChanged(int)), this, SLOT(onRadiusSliderChanged(int)));

			//connect(mUITerrainEditColorMapToolbox->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
			connect(mUITerrainEditColorMapToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(OnBrushIntensitySliderChanged(int)));

			//Editmodes
			connect(mUITerrainEditColorMapToolbox->pick_a_color_button, SIGNAL(clicked(bool)), this, SLOT(onPushPickColor(bool)));

			connect(mUITerrainEditColorMapToolbox->comboBox, SIGNAL(currentIndexChanged(int)),this, SLOT(onChangeBrushType(int)));
			connect(mUITerrainEditColorMapToolbox->use_alpha, SIGNAL(clicked(bool)),this, SLOT(onuse_alpha(bool)));

			connect(mUITerrainEditColorMapToolbox->copyMap, SIGNAL(clicked(bool)), this, SLOT(onPushCopyButton(bool)));
			InitSavePath();
			if (CheckIfUnlocked())
			{
				m_unlocked = true;
				mUITerrainEditColorMapToolbox->unlock_full_mode->setHidden(true);
			}
			else
			{	
				QSF_LOG_PRINTS(INFO,"full mode is not yet unlocked")
					
				connect(mUITerrainEditColorMapToolbox->unlock_full_mode, SIGNAL(clicked(bool)), this, SLOT(OnPushUnlockFullMode(bool)));
			}
			


			QPalette pal = mUITerrainEditColorMapToolbox->label_6->palette();
			mUITerrainEditColorMapToolbox->label_6->setAutoFillBackground(true);
			pal.setColor(mUITerrainEditColorMapToolbox->label_6->backgroundRole(), Qt::green);
			mUITerrainEditColorMapToolbox->label_6->setPalette(pal);
			
			return true;
		}

		void TerrainEditColorMapToolbox::retranslateUi(qsf::editor::ToolboxView & toolboxView)
		{
			mUITerrainEditColorMapToolbox->retranslateUi(toolboxView.widget());
			LoadSettings();
		}

		void TerrainEditColorMapToolbox::onShutdown(qsf::editor::ToolboxView & toolboxView)
		{
			QSF_LOG_PRINTS(INFO, "TerrainEditColorMapToolbox is shutdowned")
				qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode()  ==  editModeManager.get<TerrainEditmodeColorMap>())
			{
				QSF_LOG_PRINTS(INFO, "disabled edit mode?")
				//editModeManager.get<TerrainEditmodeColorMap>()->~TerrainEditmodeColorMap();
				editModeManager.selectEditModeByPointer(editModeManager.getPreviousEditMode(), editModeManager.getToolWhichSelectedEditMode());
				//hopefully we trigger shutdown
				editModeManager.forgetAboutPreviousEditMode();
			}
			disconnect(mUITerrainEditColorMapToolbox->pushButtonSelect, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
			//connect(mUITerrainEditColorMapToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(onMinimumSliderChanged(int)));
			disconnect(mUITerrainEditColorMapToolbox->horizontalSlider_2, SIGNAL(valueChanged(int)), this, SLOT(onRadiusSliderChanged(int)));

			//connect(mUITerrainEditColorMapToolbox->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
			disconnect(mUITerrainEditColorMapToolbox->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(OnBrushIntensitySliderChanged(int)));

			//Editmodes
			disconnect(mUITerrainEditColorMapToolbox->pick_a_color_button, SIGNAL(clicked(bool)), this, SLOT(onPushPickColor(bool)));

			disconnect(mUITerrainEditColorMapToolbox->comboBox, SIGNAL(currentIndexChanged(int)),this, SLOT(onChangeBrushType(int)));
			disconnect(mUITerrainEditColorMapToolbox->use_alpha, SIGNAL(clicked(bool)),this, SLOT(onuse_alpha(bool)));
			disconnect(mUITerrainEditColorMapToolbox->unlock_full_mode, SIGNAL(clicked(bool)), this, SLOT(OnPushUnlockFullMode(bool)));
			SaveSettings();
		}




		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		void TerrainEditColorMapToolbox::onPushSelectButton(const bool pressed)
		{

			qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode() == editModeManager.get<TerrainEditmodeColorMap>())
			{
				QSF_LOG_PRINTS(INFO, "Allready in editmode");
			}

			QSF_EDITOR_EDITMODE_MANAGER.selectEditMode<TerrainEditmodeColorMap>(this);
		}


		void TerrainEditColorMapToolbox::onuse_alpha(const bool pressed)
		{
			QPalette pal = mUITerrainEditColorMapToolbox->label_6->palette();
			mUITerrainEditColorMapToolbox->label_6->setText("Alpha");
			pal.setColor(mUITerrainEditColorMapToolbox->label_6->backgroundRole(), QColor(0,0,0,0));
			mUITerrainEditColorMapToolbox->label_6->setPalette(pal);
			mUITerrainEditColorMapToolbox->label_6->setStyleSheet("background - color: rgba(0,0,0,0)");
			mUITerrainEditColorMapToolbox->label_6->setAutoFillBackground(false);
		}

		void TerrainEditColorMapToolbox::onUndoOperationExecuted(const qsf::editor::base::Operation& operation)
		{

		}

		void TerrainEditColorMapToolbox::onRedoOperationExecuted(const qsf::editor::base::Operation& operation)
		{

		}



		void TerrainEditColorMapToolbox::OnBrushIntensitySliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUITerrainEditColorMapToolbox->horizontalSlider->value();
			mUITerrainEditColorMapToolbox->lineEdit->setText(ss.str().c_str());
		}

		void TerrainEditColorMapToolbox::onRadiusSliderChanged(const int value)
		{
			std::stringstream ss;
			ss << 1.0*mUITerrainEditColorMapToolbox->horizontalSlider_2->value() / 10.0;
			mUITerrainEditColorMapToolbox->lineEdit_5->setText(ss.str().c_str());
		}


		void TerrainEditColorMapToolbox::onPushPickColor(const bool pressed)
		{
			mUITerrainEditColorMapToolbox->pick_a_color_button->setChecked(false);
			if(mtoolboxView == nullptr)
			return;
			QColor originColor  = mUITerrainEditColorMapToolbox->label_6->palette().color(QPalette::ColorRole::Background);

			QColorDialog* dlg = new QColorDialog(nullptr);
			dlg->setWindowTitle("Choose a Color");
			//dlg.setOptions(options);
			dlg->setCurrentColor(originColor);
			dlg->exec();
			if(dlg->result() != QDialog::Accepted)
			return;
			QColor color = dlg->selectedColor();
			QPalette pal = mUITerrainEditColorMapToolbox->label_6->palette();
			mUITerrainEditColorMapToolbox->label_6->setAutoFillBackground(true);
			pal.setColor(mUITerrainEditColorMapToolbox->label_6->backgroundRole(), color);
			int r,g,b;
			color.getRgb(&r,&g,&b);
			std::string ColorsAsString = boost::lexical_cast<std::string>(r) + ","+ boost::lexical_cast<std::string>(g) +","+ boost::lexical_cast<std::string>(b);
			ColorsAsString = "background-color: rgb("+ColorsAsString+")";
			mUITerrainEditColorMapToolbox->label_6->setStyleSheet(ColorsAsString.c_str());
			mUITerrainEditColorMapToolbox->label_6->setPalette(pal);
			mUITerrainEditColorMapToolbox->label_6->setText("");
		}



		void TerrainEditColorMapToolbox::onPushCopyButton(const bool pressed)
		{
			auto mAssetEditHelper = std::shared_ptr<qsf::editor::AssetEditHelper>(new qsf::editor::AssetEditHelper());
			qsf::editor::EditModeManager& editModeManager = QSF_EDITOR_EDITMODE_MANAGER;
			if (editModeManager.getSelectedEditMode() != nullptr && editModeManager.getSelectedEditMode() == editModeManager.get<TerrainEditmodeColorMap>())
			{
				//delegate
				editModeManager.get<TerrainEditmodeColorMap>()-> CopyOldMap();
				return;
			}
			//QSF_LOG_PRINTS(INFO,"You have to set it running first")

		}


		void TerrainEditColorMapToolbox::InitSavePath()
		{
			for (auto a : QSF_PLUGIN.getPlugins())
			{
				if (a->getFilename().find("asset_collector_tool.dll") != std::string::npos)
				{
					path = a->getFilename();
					path.erase(path.end() - 24, path.end());


				}
			}
			return;
		}

		void TerrainEditColorMapToolbox::onChangeBrushType(const int Type)
		{

		}

		void TerrainEditColorMapToolbox::onPushSaveMap(const bool pressed)
		{

			QSF_MESSAGE.emitMessage(qsf::MessageConfiguration("kc::save_heightmap"));
		}

		void TerrainEditColorMapToolbox::OnPushUnlockFullMode(const bool pressed)
		{
			QInputDialog *dialog = new QInputDialog();
			dialog->setInputMode(QInputDialog::InputMode::IntInput);
			dialog->setWindowTitle("Unlock Full Mode");
			dialog->setLabelText("This is hidden behind a password, because several steps are required to get it to work. You'll find out how to this at the forum. Please enter the password");
			dialog->setTextValue("Password");
			int ret = dialog->exec();
			QString text = dialog->textValue();
			if (ret == QDialog::Accepted && !text.isEmpty() && text == "KC_Terrain") 
			{
				m_unlocked = true;
				std::ofstream ofs(path + "plugin_unlocked.txt", std::ofstream::trunc);

				ofs << "KC_Terrain";

				ofs.close();
				mUITerrainEditColorMapToolbox->unlock_full_mode->setHidden(true);
			}
		}

		bool TerrainEditColorMapToolbox::CheckIfUnlocked()
		{
			std::ifstream myfile(path + "plugin_unlocked.txt");
			std::string line;
			if (myfile.is_open())
			{
				while (std::getline(myfile, line))
				{
					line;
					break;

				}
				myfile.close();
			}
			if(line == "KC_Terrain")
			return true;
			return false;
		}

		void TerrainEditColorMapToolbox::LoadSettings()
		{
			auto Setman = user::editor::Settingsmanager::instance;
			mUITerrainEditColorMapToolbox->horizontalSlider->setValue(Setman->GetBrushIntensity());
			mUITerrainEditColorMapToolbox->horizontalSlider_2->setValue(Setman->GetBrushSize());
			mUITerrainEditColorMapToolbox->comboBox->setCurrentIndex(Setman->GetBrushShape());

			onRadiusSliderChanged(Setman->GetBrushSize());
			OnBrushIntensitySliderChanged(Setman->GetBrushIntensity());
			onChangeBrushType(Setman->GetBrushShape());

			std::stringstream ss;
			ss << 1.0*mUITerrainEditColorMapToolbox->horizontalSlider_2->value() / 10.0;
			mUITerrainEditColorMapToolbox->lineEdit_5->setText(ss.str().c_str());


			std::stringstream sst;
			sst << mUITerrainEditColorMapToolbox->horizontalSlider->value();
			mUITerrainEditColorMapToolbox->lineEdit->setText(sst.str().c_str());

			switch (Setman->GetBrushShape())
			{
			case 0:

				mUITerrainEditColorMapToolbox->comboBox->setCurrentIndex(0);
				return;
			case 1:

				mUITerrainEditColorMapToolbox->comboBox->setCurrentIndex(1);
				return;
			case 2:
				mUITerrainEditColorMapToolbox->comboBox->setCurrentIndex(2);
				return;
			default:
				mUITerrainEditColorMapToolbox->comboBox->setCurrentIndex(3);
				return;
			}
		}

		void TerrainEditColorMapToolbox::SaveSettings()
		{
			auto Setman = user::editor::Settingsmanager::instance;

			Setman->SetBrushIntensity(mUITerrainEditColorMapToolbox->horizontalSlider->value());
			Setman->SetBrushSize(mUITerrainEditColorMapToolbox->horizontalSlider_2->value());
			Setman->SetBrushShape(mUITerrainEditColorMapToolbox->comboBox->currentIndex());
		}

		bool TerrainEditColorMapToolbox::GetMirrorX()
		{
			return mUITerrainEditColorMapToolbox->MirrorX->isChecked();
		}

		bool TerrainEditColorMapToolbox::GetMirrorY()
		{
		 return mUITerrainEditColorMapToolbox->MirrorY->isChecked();
		}

		TerrainEditColorMapToolbox * TerrainEditColorMapToolbox::GetInstance()
		{
			return instance;
		}


		TerrainEditColorMapToolbox::TerrainEditMode2 TerrainEditColorMapToolbox::GetEditMode()
		{
			return mMode;
		}

		void TerrainEditColorMapToolbox::SetHeight(float NewHeight)
		{
			std::string Heightset = boost::lexical_cast<std::string>(NewHeight);
			std::string rounded = Heightset.substr(0, Heightset.find(".") + 3);
			mUITerrainEditColorMapToolbox->lineEdit_2->setText(rounded.c_str());
		}









		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
