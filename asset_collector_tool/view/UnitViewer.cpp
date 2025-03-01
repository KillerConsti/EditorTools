// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/UnitViewer.h"
#include "ui_UnitViewer.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)


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
#include <qsf\map\Entity.h>
#include <qsf\component\base\MetadataComponent.h>
#include <em5\component\vehicle\VehicleComponent.h>
#include <em5\component\vehicle\RoadVehicleComponent.h>
#include <em5\plugin\Jobs.h>
#include <qsf/component/link/LinkComponent.h>
#include <qsf/component/street/crossing/StreetCrossingComponent.h>
#include <qsf/debug/DebugDrawManager.h>

#include <qsf/debug/request/CircleDebugDrawRequest.h>
#include <em5\map\EntityHelper.h>
#include <qsf/component/base/TransformComponent.h>
#include <fstream>
#include <ogre\Ogre.h>

#include <qsf/renderer/component/CameraComponent.h>
#include <qsf/renderer/window/RenderWindow.h>
#include <qsf/input/InputSystem.h>
#include <qsf/input/device/MouseDevice.h>
#include <qsf/map/query/RayMapQuery.h>
#include <qsf_editor/renderer/RenderView.h>
#include <QtWidgets\qgraphicsview.h>
#include <qsf/map/query/ComponentMapQuery.h>
#include <qsf/math/EulerAngles.h>
#include <qsf/component/street/section/StreetSectionComponent.h>
#include <qsf/map/component/MapPropertiesComponent.h>
#include <QtWidgets\qprogressbar.h>
#include <qsf/component/nodes/PathMeshComponent.h>
#include <qsf/component/utility/BoostSignalComponent.h>
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
		const uint32 UnitViewer::PLUGINABLE_ID = qsf::StringHash("user::editor::UnitViewer");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		UnitViewer::UnitViewer(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
			View(viewManager, qWidgetParent),
			mUiUnitViewer(nullptr),
			mSelectedNodeDebug(0)
		{
			// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
			addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);
			mQImage = new QImage(512, 512, QImage::Format_RGB32);
			//QRgb value;

			//trial and error
			//
			
		}

		UnitViewer::~UnitViewer()
		{
			mBritifyJob.unregister();
			if (QSF_DEBUGDRAW.isRequestIdValid(mSelectedNodeDebug))
				QSF_DEBUGDRAW.cancelRequest(mSelectedNodeDebug);
			mTireJob.unregister();
			mMaterialViewerJob.unregister();
			// Destroy the UI view instance
			if (nullptr != mUiUnitViewer)
			{
				qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
				entitySelectionManager.Selected.disconnect(boost::bind(&UnitViewer::onSelectionChanged, this, _1));
				delete mUiUnitViewer;
			}
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		void UnitViewer::retranslateUi()
		{

			// Retranslate the content of the UI, this automatically clears the previous content
			mUiUnitViewer->retranslateUi(this);
		}

		void UnitViewer::changeVisibility(bool visible)
		{

			// Lazy evaluation: If the view is shown the first time, create its content
			if (visible && nullptr == mUiUnitViewer)
			{
				// Setup the view content
				QWidget* contentWidget = new QWidget(this);
				{

					// Load content to widget
					mUiUnitViewer = new Ui::UnitViewer();
					mUiUnitViewer->setupUi(contentWidget);
				}
				// Set content to view
				setWidget(contentWidget);
				// Connect Qt signals/slots
				connect(mUiUnitViewer->open_door, SIGNAL(clicked(bool)), this, SLOT(onPushOpenDoorButton(bool)));
				connect(mUiUnitViewer->close_door, SIGNAL(clicked(bool)), this, SLOT(onPushCloseDoorButton(bool)));
				connect(mUiUnitViewer->blinker, SIGNAL(clicked(bool)), this, SLOT(OnPushBlinker(bool)));
				connect(mUiUnitViewer->bluelight, SIGNAL(clicked(bool)), this, SLOT(OnPushBlueLight(bool)));
				connect(mUiUnitViewer->headlight, SIGNAL(clicked(bool)), this, SLOT(OnPushHeadLight(bool)));

				//WPT LIGHTS
				connect(mUiUnitViewer->brakeButton, SIGNAL(clicked(bool)), this, SLOT(onPushbrakeButton(bool)));
				connect(mUiUnitViewer->trafficwarnerbutton, SIGNAL(clicked(bool)), this, SLOT(onPushtrafficwarnerbutton(bool)));
				connect(mUiUnitViewer->reverseButton, SIGNAL(clicked(bool)), this, SLOT(onPushreverseButtonbutton(bool)));
				connect(mUiUnitViewer->interiorbutton, SIGNAL(clicked(bool)), this, SLOT(onPushtinteriorbutton(bool)));
				connect(mUiUnitViewer->enviromentbutton, SIGNAL(clicked(bool)), this, SLOT(onPushenviromentbutton(bool)));



				connect(mUiUnitViewer->rotatetires, SIGNAL(clicked(bool)), this, SLOT(OnPushRotateTires(bool)));
				connect(mUiUnitViewer->select_entity, SIGNAL(clicked(bool)), this, SLOT(onPushSelectEntity(bool)));
				connect(mUiUnitViewer->PushAddDebugCircles, SIGNAL(clicked(bool)), this, SLOT(onPushAddDebugCircles(bool)));
				connect(mUiUnitViewer->PushRemoveDebugCirecles, SIGNAL(clicked(bool)), this, SLOT(onPushRemoveDebugCircles(bool)));
				connect(mUiUnitViewer->PushUpdateNodes, SIGNAL(clicked(bool)), this, SLOT(onPushUpdateNodes(bool)));
				connect(mUiUnitViewer->pushAddAllNodes, SIGNAL(clicked(bool)), this, SLOT(onpushAddAllNodes(bool)));
				connect(mUiUnitViewer->pushButtonUV_coordinates, SIGNAL(clicked(bool)), this, SLOT(OnPushStartReadMaterial(bool)));
				connect(mUiUnitViewer->BritifyMap, SIGNAL(clicked(bool)), this, SLOT(onPushBritifyMap(bool)));
				connect(mUiUnitViewer->BritifyCrossing, SIGNAL(clicked(bool)), this, SLOT(onPushBritifyCrossing(bool)));
				//connect(mUiUnitViewer->setsimulting, SIGNAL(clicked(bool)), this, SLOT(OnPushSetSimulating(bool)));
				//connect(mUITrainTrackTool->pushButton_fill_tree_view, SIGNAL(clicked(bool)), this, SLOT(CreatePathEntities(bool)));
				/*connect(mUiUnitViewer->pushCheckUnit, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
				connect(mUiUnitViewer->loadexternallog, SIGNAL(clicked(bool)), this, SLOT(OnLoadLogFile(bool)));
				connect(mUiUnitViewer->searhprototype, SIGNAL(clicked(bool)), this, SLOT(OnPushSearchForPrototype(bool)));
				connect(mUiUnitViewer->export_2, SIGNAL(clicked(bool)), this, SLOT(OnSaveLogFile(bool)));*/

				//connect(mUiUnitViewer->comboBoxType, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));

				connect(mUiUnitViewer->AnalyseMeshButton, SIGNAL(clicked(bool)), this, SLOT(onPush_AnalyseMeshButton(bool)));
				qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
				auto con2 = &entitySelectionManager.Selected.connect(boost::bind(&UnitViewer::onSelectionChanged, this, _1), boost::signals2::at_back);
				if (con2 == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "entitySelectionManager :: Slot connection failed 2 ")
				}

			}
			else if (!visible && nullptr == mUiUnitViewer)
			{

			}
			mUiUnitViewer->BritifyMap->setEnabled(true);
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void UnitViewer::rebuildGui()
		{
		}

		void UnitViewer::onPushOpenDoorButton(const bool pressed)
		{
			for (auto a : GetSelectedEntity())
			{
				auto comp = a->getComponent<em5::VehicleComponent>();
				if (comp == nullptr)
					continue;
				for (auto b : comp->getVehicleDoors())
				{
					auto Door = QSF_MAINMAP.getEntityById(b);
					if (Door == nullptr)
						continue;
					em5::DoorComponent* DoorC = Door->getComponent<em5::DoorComponent>();
					if (DoorC == nullptr)
						continue;
					if (DoorC->getDoorState() != DoorC->DOOR_CLOSED)
						continue;
					DoorC->openDoor();

				}
			}
			for (auto Door : GetSelectedEntity())
			{
				auto comp = Door->getComponent<em5::DoorComponent>();
				if (comp == nullptr)
					continue;
				em5::DoorComponent* DoorC = Door->getComponent<em5::DoorComponent>();
				if (DoorC == nullptr)
					continue;
				if (DoorC->getDoorState() != DoorC->DOOR_CLOSED)
					continue;
				DoorC->openDoor();

			}
		}

		void UnitViewer::onPushCloseDoorButton(const bool pressed)
		{
			for (auto a : GetSelectedEntity())
			{
				auto comp = a->getComponent<em5::VehicleComponent>();
				if (comp == nullptr)
					continue;
				for (auto b : comp->getVehicleDoors())
				{
					auto Door = QSF_MAINMAP.getEntityById(b);
					if (Door == nullptr)
						continue;
					em5::DoorComponent* DoorC = Door->getComponent<em5::DoorComponent>();
					if (DoorC == nullptr)
						continue;
					if (DoorC->getDoorState() != DoorC->DOOR_OPEN)
						continue;
					DoorC->closeDoor();

				}
			}
			for (auto Door : GetSelectedEntity())
			{
				auto comp = Door->getComponent<em5::DoorComponent>();
				if (comp == nullptr)
					continue;
				em5::DoorComponent* DoorC = Door->getComponent<em5::DoorComponent>();
				if (DoorC == nullptr)
					continue;
				if (DoorC->getDoorState() != DoorC->DOOR_OPEN)
					continue;
				DoorC->closeDoor();

			}
		}

		void UnitViewer::onSelectionChanged(uint64 Id)
		{
			auto ent = QSF_MAINMAP.getEntityById(Id);
			if (ent == nullptr)
			{
				mUiUnitViewer->bluelight->setChecked(false);
				mUiUnitViewer->headlight->setChecked(false);
				mUiUnitViewer->blinker->setChecked(false);
				return;
			}
			auto Vec = ent->getComponent<em5::VehicleComponent>();
			if (Vec == nullptr)
			{
				mUiUnitViewer->bluelight->setChecked(false);
				mUiUnitViewer->headlight->setChecked(false);
				mUiUnitViewer->blinker->setChecked(false);
				return;
			}
			auto RoV = ent->getComponent<em5::RoadVehicleComponent>();
			if (RoV == nullptr)
			{
				mUiUnitViewer->bluelight->setChecked(false);
				mUiUnitViewer->headlight->setChecked(false);
				mUiUnitViewer->blinker->setChecked(false);
				mUiUnitViewer->brakeButton->setChecked(false);
				return;
			}
			ResetWheelsAfterDeselection();
			AffectedByTire.clear();
			bool BL = false;
			for (auto a : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_BLUE))
			{
				if (QSF_MAINMAP.getEntityById(a) == nullptr)
					continue;
				BL = QSF_MAINMAP.getEntityById(a)->getComponent<qsf::game::LightControllerComponent>()->isActive();
				break;
			}
			bool HL = false;
			for (auto a : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_HEAD))
			{
				if (QSF_MAINMAP.getEntityById(a) == nullptr)
					continue;
				HL = QSF_MAINMAP.getEntityById(a)->getComponent<qsf::game::LightControllerComponent>()->isActive();
				break;
			}
			bool Blinker = false;
			for (auto a : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_LEFT_BLINKER))
			{
				if (QSF_MAINMAP.getEntityById(a) == nullptr)
					continue;
				Blinker = QSF_MAINMAP.getEntityById(a)->getComponent<qsf::game::LightControllerComponent>()->isActive();
				break;
			}
			bool Brake = false;
			for (auto a : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_BRAKE))
			{
				if (QSF_MAINMAP.getEntityById(a) == nullptr)
					continue;
				Blinker = QSF_MAINMAP.getEntityById(a)->getComponent<qsf::game::LightControllerComponent>()->isActive();
				break;
			}
			mUiUnitViewer->bluelight->setChecked(BL);
			mUiUnitViewer->headlight->setChecked(HL);
			mUiUnitViewer->blinker->setChecked(Blinker);
			OnSelectionChange_SetAdditionalLightButtons(mUiUnitViewer->enviromentbutton, "umfeld");
			OnSelectionChange_SetAdditionalLightButtons(mUiUnitViewer->interiorbutton, "interior");
			OnSelectionChange_SetAdditionalLightButtons(mUiUnitViewer->reverseButton, "reverse");
			OnSelectionChange_SetAdditionalLightButtons(mUiUnitViewer->trafficwarnerbutton, "traffic");
		}

		void UnitViewer::OnPushBlinker(const bool pressed)
		{

			bool NewWantedState = mUiUnitViewer->blinker->isChecked();
			bool AnythingDone = false;
			for (auto a : GetSelectedEntity())
			{
				auto RoV = a->getComponent<em5::RoadVehicleComponent>();
				if (RoV != nullptr)
				{
					if (RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_LEFT_BLINKER).empty() && RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_RIGHT_BLINKER).empty())
					{
						QSF_LOG_PRINTS(INFO, "Blinker Light vector is empty")
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_LEFT_BLINKER))
						{
							if (QSF_MAINMAP.getEntityById(LB) == nullptr) continue;
							QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->setActive(NewWantedState);
						}
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_RIGHT_BLINKER))
						{
							if (QSF_MAINMAP.getEntityById(LB) == nullptr) continue;
							QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->setActive(NewWantedState);
						}
						AnythingDone = true;
					}
				}
			}
			if (AnythingDone)
				mUiUnitViewer->blinker->setChecked(NewWantedState);
		}

		void UnitViewer::OnPushBlueLight(const bool pressed)
		{

			bool NewWantedState = mUiUnitViewer->bluelight->isChecked();
			bool AnythingDone = false;
			for (auto a : GetSelectedEntity())
			{
				auto RoV = a->getComponent<em5::RoadVehicleComponent>();
				if (RoV != nullptr)
				{
					if (RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_BLUE).empty())
					{
						QSF_LOG_PRINTS(INFO, "Blue Light vector is empty")
							//everything emptY?
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_BLUE))
						{
							if (QSF_MAINMAP.getEntityById(LB) == nullptr) continue;
							QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->setActive(NewWantedState);
						}
						AnythingDone = true;
					}
				}
			}
			if (AnythingDone)
				mUiUnitViewer->bluelight->setChecked(NewWantedState);
		}

		void UnitViewer::OnPushHeadLight(const bool pressed)
		{
			bool NewWantedState = mUiUnitViewer->headlight->isChecked();
			bool AnythingDone = false;
			for (auto a : GetSelectedEntity())
			{
				auto RoV = a->getComponent<em5::RoadVehicleComponent>();
				if (RoV != nullptr)
				{
					if (RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_HEAD).empty())
					{
						//everything emptY?
						//QSF_LOG_PRINTS(INFO, "Head Light vector is empty")
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_HEAD))
						{
							if (QSF_MAINMAP.getEntityById(LB) == nullptr) continue;
							QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->setActive(NewWantedState);
						}
						AnythingDone = true;
					}
				}
			}
			if (AnythingDone)
				mUiUnitViewer->headlight->setChecked(NewWantedState);
		}

		void UnitViewer::OnPushSetSimulating(const bool pressed)
		{
			/*bool NewWantedState = mUiUnitViewer->setsimulting->isChecked();
			QSF_MAINMAP.setSimulatingMode(NewWantedState);
			mUiUnitViewer->setsimulting->setChecked(NewWantedState);*/
		}

		void UnitViewer::OnPushRotateTires(const bool pressed)
		{
			bool NewWantedState = mUiUnitViewer->rotatetires->isChecked();
			if (NewWantedState)
			{
				AffectedByTire.clear();
				for (auto a : GetSelectedEntity())
				{
					auto RC = a->getComponent<em5::RoadVehicleComponent>();
					if (RC == nullptr)
						continue;
					AffectedByTire.push_back(a);
					mTireJob.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&UnitViewer::TireJob, this, _1));
				}
			}
			else
			{
				ResetWheelsAfterDeselection();
				AffectedByTire.clear();
			}
			mUiUnitViewer->rotatetires->setChecked(NewWantedState);

		}

		void UnitViewer::onPushSelectEntity(const bool pressed)
		{
			auto content = mUiUnitViewer->entity_selection_line_edit->text().toStdString();
			uint64 unit;
			try
			{
				unit = boost::lexical_cast<uint64>(content);
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO, "UnitViewer ->" << e.what())
					return;
			}
			if (QSF_MAINMAP.getEntityById(unit) == nullptr)
				return;
			qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
			entitySelectionManager.clearSelection();

			entitySelectionManager.addIdToSelection(unit);
		}

		void UnitViewer::onPushAddDebugCircles(const bool pressed)
		{
			qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
			for (auto a : entitySelectionManager.getSelectedIdSet())
			{
				if (IsEntityAllreadySelected(a, DebugEntities))
					continue;
				auto ent = QSF_MAINMAP.getEntityById(a);
				if (ent == nullptr)
					continue;
				auto SCC = ent->getComponent<qsf::StreetCrossingComponent>();
				if (SCC == nullptr)
					continue;
				DebugEntities.push_back(a);
			}
			UpdateStreetDebugNodes();
		}

		void UnitViewer::onPushRemoveDebugCircles(const bool pressed)
		{
			DebugEntitiesFullNodeView.clear();
			DebugEntities.clear();
			UpdateStreetDebugNodes();
		}

		void UnitViewer::onPushUpdateNodes(const bool pressed)
		{
			UpdateStreetDebugNodes();
		}

		void UnitViewer::onpushAddAllNodes(const bool pressed)
		{
			qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
			for (auto a : entitySelectionManager.getSelectedIdSet())
			{
				if (IsEntityAllreadySelected(a, DebugEntitiesFullNodeView))
					continue;
				auto ent = QSF_MAINMAP.getEntityById(a);
				if (ent == nullptr)
					continue;
				auto SCC = ent->getComponent<qsf::StreetCrossingComponent>();
				if (SCC == nullptr)
					continue;
				DebugEntitiesFullNodeView.push_back(a);
			}
			UpdateStreetDebugNodes();
		}

		void UnitViewer::onPushBritifyMap(const bool pressed)
		{
			if(mBritifyJob.isValid())
			return;
			mUiUnitViewer->BritifyMap->setEnabled(false);
			mBritifyState = BritifyState::Start;
			mOldBritifyState = BritifyState::Finish;
			mBritifyJob.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&UnitViewer::BritifyJob, this, _1));
		}

		void user::editor::UnitViewer::BritifyJob(const qsf::JobArguments & jobArguments)
		{
			
			mJobStartTime = std::chrono::high_resolution_clock::now();
			if(mBritifyState != mOldBritifyState)
			{
				//QSF_LOG_PRINTS(INFO,"BritifyJob change state to "<< mBritifyState)
				mOldBritifyState = mBritifyState;
			}

			mUiUnitViewer->progressBar->setValue(0);
			OldProgress = 0;
			switch (mBritifyState)
			{
			case user::editor::UnitViewer::Start:
			{
				auto mapcomponents = qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<qsf::MapPropertiesComponent>();
				{
					for (auto a : mapcomponents)
					{
						mMidpoint = a->getMapBoundaryTopLeft() + a->getMapBoundaryBottomRight();
						mMidpoint = mMidpoint / 2.f;
						break;
					}
				}
				qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<qsf::TransformComponent>().copyToVector(mBritifyParents);
				CurrentChildIndex = 0;
				ChildTransforms.clear();
				mBritifyState = Collect_Childs;
				SetNewJobTiming();
				return;
			}
			case user::editor::UnitViewer::Collect_Childs:
			{
				for(uint64 t=CurrentChildIndex; t < mBritifyParents.size() && t <= CurrentChildIndex + 50; t++)
				{
					CollectAllChildTransforms(&mBritifyParents.at(t)->getEntity());
				}
				CurrentChildIndex = glm::clamp(CurrentChildIndex + 50, (uint64)0, (uint64)mBritifyParents.size());
				float ProgressValue = (float)CurrentChildIndex / (float)mBritifyParents.size();
				//max is 45%
				ProgressValue = ProgressValue * 45.f;
				if (glm::abs((int)ProgressValue - 1 - (int)OldProgress) >= 3)
				{
					mUiUnitViewer->progressBar->setValue(int(ProgressValue));
					OldProgress = (int)ProgressValue;
				}
				
				//QSF_LOG_PRINTS(INFO,"Scanned Childs "<< CurrentChildIndex << " / "<< mBritifyParents.size() << "Progress "<< ProgressValue)
				if (CurrentChildIndex >= mBritifyParents.size())
					mBritifyState = Collect_Childs_Finish;
				SetNewJobTiming();
				return;
			}
			case user::editor::UnitViewer::Collect_Childs_Finish:
			{
				CurrentChildIndex = 0;
				CurrentParentIndex = 0;
				mBritifyState = Apply_Transfroms_Parents;
				return;
			}
			case user::editor::UnitViewer::Apply_Transfroms_Parents:
			{
				qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
				for (uint64 t= CurrentParentIndex ; t < mBritifyParents.size() && t <= CurrentParentIndex+50;t++)
				{
					auto a = mBritifyParents.at(t);
					if (a->getEntity().getComponent<qsf::CameraComponent>() != nullptr) //do not change street sections or camera
					{
						continue;
					}
					
					//mirror z position
					glm::vec3 Oldpos = a->getPosition();
					float Dist = a->getPosition().z - mMidpoint.z;
					a->setPosition(glm::vec3(a->getPosition().x, a->getPosition().y, mMidpoint.z - Dist));
					compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(a->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::StringHash("Position"), a->getPosition()));
					if (a->getEntity().getComponent<qsf::StreetSectionComponent>() != nullptr)
					{
						HandleStreetSection(&a->getEntity());
						continue;
					}
					//try out for everyone

					//if (a->getEntity().getComponent<qsf::StreetCrossingComponent>() != nullptr) //if street crossing we need to mirror y and rotate y by 180° (so we create a left side system)
					//{
						glm::vec3 Scale = a->getScale();
						Scale.y = Scale.y*-1.f;
						a->setScale(Scale);
						compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(a->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::StringHash("Scale"), a->getScale()));
						//finally we need to invert the turning
						auto b = a->getRotation();
						auto OldRot = b;
						b.y = -b.y;
						b.z = -b.z;
						a->setRotation(b*qsf::EulerAngles::eulerToQuaternion(0.f, glm::radians(180.f), 0.f));
						compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(a->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::StringHash("Rotation"), a->getRotation()));
					//}
					/*else //turn x by 180 degrees
					{
						a->setRotation(a->getRotation()*qsf::EulerAngles::eulerToQuaternion(glm::vec3(glm::radians(180.f), 0.f, 0.f)));
						compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(a->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::StringHash("Rotation"), a->getRotation()));

					}*/
					//a->setAllPropertyOverrideFlags(true);

				}

			QSF_EDITOR_OPERATION.push(compoundOperation2);
			CurrentParentIndex = glm::clamp(CurrentParentIndex + 50, (uint64)0, (uint64)mBritifyParents.size());

			float ProgressValue = (float)CurrentParentIndex / (float)mBritifyParents.size();
			ProgressValue = ProgressValue * 10.f + 45.f; //45 to 55
			if (glm::abs((int)ProgressValue - 1 - (int)OldProgress) >= 3)
			{
				mUiUnitViewer->progressBar->setValue(int(ProgressValue));
				OldProgress = (int)ProgressValue;
			}

			//QSF_LOG_PRINTS(INFO, "Moved Childs " << CurrentParentIndex << " / " << mBritifyParents.size())
			if (CurrentParentIndex >= mBritifyParents.size())
			{
				mBritifyState = Update_StreetSections;
				/* update crossings*/
				CurrentChildIndex = 0;
			}
				SetNewJobTiming();
				return;
			}
			case Update_StreetSections:
			{
			 //no progress bar
				for (auto a : mBritifyParents)
				{
					if (a->getEntity().getComponent<qsf::StreetSectionComponent>() != nullptr)
					{
						auto TC = a->getEntity().getComponent<qsf::TransformComponent>();
						a->getEntity().getComponent<qsf::StreetSectionComponent>()->dirtyMesh();
						//update street sections later
					}
					if (a->getEntity().getComponent<qsf::PathMeshComponent>() != nullptr)
					{
						a->getEntity().getComponent<qsf::PathMeshComponent>()->dirtyMesh();
						//update street sections later
						continue;
					}
				}
				mBritifyState = Apply_Transforms_Childs;
				return;
			}
			case user::editor::UnitViewer::Apply_Transforms_Childs:
			{
				for (uint64 t = CurrentChildIndex; t < ChildTransforms.size() && t <= CurrentChildIndex + 50; t++)
				{
					ApplyChildTransforms((int)t);
				}
				CurrentChildIndex = glm::clamp(CurrentChildIndex + 50, (uint64)0, (uint64)ChildTransforms.size());
				//QSF_LOG_PRINTS(INFO, "Moved Childs " << CurrentChildIndex << " / " << mBritifyParents.size())
					if (CurrentChildIndex >= ChildTransforms.size())
						mBritifyState = Finish;
				float ProgressValue = (float)CurrentChildIndex / (float)ChildTransforms.size();
				ProgressValue = ProgressValue * 45.f+55.f; //50 to 99
				if (glm::abs((int)ProgressValue - 1 - (int)OldProgress) >= 3)
				{
					mUiUnitViewer->progressBar->setValue(int(ProgressValue));
					OldProgress = (int)ProgressValue;
				}
				//QSF_LOG_PRINTS(INFO,"Applied Child Transforms "<< CurrentChildIndex << " / "<< mBritifyParents.size() << "Progress "<< ProgressValue)
					SetNewJobTiming();
					return;
			}

			case user::editor::UnitViewer::Finish:
			{
				//QSF_LOG_PRINTS(INFO, "Britify done")
				//Clear vectors
				boost::container::flat_set<uint64> Crossings;
				for (auto a : mBritifyParents)
				{
					if(a->getEntity().getComponent<qsf::StreetCrossingComponent>() != nullptr)
					Crossings.insert(a->getEntityId());
				}
				mBritifyJob.unregister();
				ChildTransforms.clear();
				mBritifyParents.clear();
				mUiUnitViewer->progressBar->setValue(int(100));
				mUiUnitViewer->BritifyMap->setEnabled(true);

				//select all crossings
				//user hve to emit transform signal by hand
				QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>().setSelectionByIdSet(Crossings);
				return;
			}
			default:
				break;
			}
		}

		void UnitViewer::HandleStreetSection(qsf::Entity * ent)
		{
			//we need to apply rotation
			//
			auto MasterPos = em5::EntityHelper(ent).getPosition();
			//ent->getComponent<qsf::TransformComponent>()->setRotation(glm::quat(1,0,0,0));
			auto Rotation = ent->getComponent<qsf::TransformComponent>()->getRotation();
			auto SSC = ent->getComponent<qsf::StreetSectionComponent>();
			if(SSC->getNumberOfNodes() <= 4) //only critital if more then 4 nodes
			return;
			MasterPos.x = 0;
			MasterPos.y = 0;
			std::vector<qsf::Node> NodeVec;
			NodeVec.push_back(SSC->getNodes().at(0));
			NodeVec.push_back(SSC->getNodes().at(1));
			if(false)
			{ 
			for (uint32 t = 2; t < SSC->getNumberOfNodes() - 2; t++) //avoid last 2 and first 2 nodes
			{
				qsf::Node MyNewNode = SSC->getNodes().at(t);
				//glm::vec3 ModifiedNodePos = Rotation*MyNewNode.getPosition();
				//glm::vec3 MyPos = ModifiedNodePos + MasterPos;
				glm::vec3 ModifiedNodePos = MyNewNode.getPosition();
				ModifiedNodePos.z = ModifiedNodePos.z*-1.f;
				MyNewNode.setPosition(ModifiedNodePos);
				NodeVec.push_back(MyNewNode);
			}
			}
			else
			{
				//different approach first convert rotated positions into world coordinates
				//then rotate entity to 1,0,0,0
				//
				std::vector<glm::vec3> SafePos; 
				for (uint32 t = 2; t < SSC->getNumberOfNodes() - 2; t++) //avoid last 2 and first 2 nodes
				{
					qsf::Node MyNewNode = SSC->getNodes().at(t);
					glm::vec3 ModifiedNodePos = MyNewNode.getPosition()*Rotation;
					SafePos.push_back(ModifiedNodePos);
				}
				//rotate back
				ent->getComponent<qsf::TransformComponent>()->setRotation(glm::quat(1,0,0,0));
				ent->getComponent<qsf::TransformComponent>()->setAllPropertyOverrideFlags(true);
				for (uint32 t = 2; t < SSC->getNumberOfNodes() - 2; t++) //avoid last 2 and first 2 nodes
				{
					qsf::Node MyNewNode = SSC->getNodes().at(t);
					auto Pos = SafePos.at(t-2);
					Pos.z = Pos.z* -1.f;
					MyNewNode.setPosition(Pos);
					NodeVec.push_back(MyNewNode);
				}

			}
			NodeVec.push_back(SSC->getNodes().at(SSC->getNumberOfNodes() - 2));
			NodeVec.push_back(SSC->getNodes().at(SSC->getNumberOfNodes() - 1));
			SSC->setNodes(NodeVec);
			SSC->setAllPropertyOverrideFlags(true);
		}

		void UnitViewer::SetNewJobTiming()
		{
			auto stop2 = std::chrono::high_resolution_clock::now();
			auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(stop2 - mJobStartTime);
			mBritifyJob.changeTimeBetweenCalls(qsf::Time::fromMilliseconds(duration2.count() + 1.f));

			//QSF_LOG_PRINTS(INFO,"New Timing "<<duration2.count() + 3.f)
		}

		void UnitViewer::CollectAllChildTransforms(qsf::Entity* Parent)
		{
		//the first part is about collecting all childrens
		//everything is written to entList
			std::vector<qsf::Entity*> entList;
			if (Parent == nullptr)
				return;
			auto LC = Parent->getComponent<qsf::LinkComponent>();
			if(LC == nullptr)
			return;
			//QSF_LOG_PRINTS(INFO,"Scanned Child size "<<LC->getChildLinks().size())
			for (auto a : LC->getChildLinks())
			{
				if (a != nullptr && &a->getEntity() != nullptr)
					entList.push_back(&a->getEntity());
			}
			std::vector<qsf::Entity*> ChildList;

			ChildList.insert(ChildList.begin(), entList.begin(), entList.end());
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
			//we could push back ourself
			//entList.push_back(Entity);
			//save our transform now
			for (auto a : entList)
			{
				if(a->getComponent<qsf::LinkComponent>() == nullptr)
				continue;
				ChildTransforms.push_back(std::pair<qsf::Transform,qsf::Entity*>(a->getComponent<qsf::LinkComponent>()->getLocalTransform(),a));
				//this was a test object
				/*if (a->getId() == 12375565814870347598)
				{
					QSF_LOG_PRINTS(INFO,"Initial pos"<< a->getComponent<qsf::LinkComponent>()->getLocalPosition() << " size "<< ChildTransforms.size())
				}*/
			}
		}

		void UnitViewer::ApplyChildTransforms(uint64 index)
		{
			auto Item = ChildTransforms.at(index);
			/*if (Item.second->getId() == 12375565814870347598)
			{
				QSF_LOG_PRINTS(INFO, "Before set " <<Item.second->getComponent<qsf::LinkComponent>()->getLocalPosition())
			}*/
			auto LC = Item.second->getComponent<qsf::LinkComponent>();
			LC->setLocalTransform(Item.first);
			LC->setAllPropertyOverrideFlags(true);
			/*if (Item.second->getId() == 12375565814870347598)
			{
				QSF_LOG_PRINTS(INFO, "After set " << Item.second->getComponent<qsf::LinkComponent>()->getLocalPosition())
			}*/
		}



		void UnitViewer::onPushBritifyCrossing(const bool pressed)
		{
			
			qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
			for (auto ab : GetSelectedEntity())
			{
				auto a = ab->getComponent<qsf::TransformComponent>();
				if (a->getEntity().getComponent<qsf::StreetCrossingComponent>() != nullptr) //if street crossing we need to mirror y and rotate y by 180° (so we create a left side system)
				{
					glm::vec3 Scale = a->getScale();
					Scale.y = Scale.y*-1.f;
					a->setScale(Scale);
					compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(a->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::StringHash("Scale"), a->getScale()));
					//finally we need to invert the turning
					auto b = a->getRotation();
					b.y = -b.y;
					b.z = -b.z;
					a->setRotation(b*qsf::EulerAngles::eulerToQuaternion(0.f, glm::radians(180.f), 0.f));
					compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(a->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::StringHash("Rotation"), a->getRotation()));
				}
			}
			QSF_EDITOR_OPERATION.push(compoundOperation2);
		}

		void UnitViewer::onPushbrakeButton(const bool pressed)
		{

			bool NewWantedState = mUiUnitViewer->brakeButton->isChecked();
			bool AnythingDone = false;
			for (auto a : GetSelectedEntity())
			{
				auto RoV = a->getComponent<em5::RoadVehicleComponent>();
				if (RoV != nullptr)
				{
					if (RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_BRAKE).empty())
					{
						//everything emptY?
						//QSF_LOG_PRINTS(INFO, "Head Light vector is empty")
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_BRAKE))
						{
							if (QSF_MAINMAP.getEntityById(LB) == nullptr) continue;
							QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->setActive(NewWantedState);
						}
						AnythingDone = true;
					}
				}
			}
			if (AnythingDone)
				mUiUnitViewer->brakeButton->setChecked(NewWantedState);

		}

		void UnitViewer::onPushtrafficwarnerbutton(const bool pressed)
		{
			bool NewWantedState = mUiUnitViewer->trafficwarnerbutton->isChecked();
			bool AnythingDone = false;
			for (auto a : GetSelectedEntity())
			{
				auto RoV = a->getComponent<em5::RoadVehicleComponent>();
				if (RoV != nullptr)
				{
					if (RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED).empty())
					{
						//everything emptY?
						QSF_LOG_PRINTS(INFO, "traffic (warner) light vector is empty")
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED))
						{
							auto ent = QSF_MAINMAP.getEntityById(LB);
							if (ent == nullptr || ent->getComponent<qsf::MetadataComponent>() == nullptr || ent->getComponent<qsf::MetadataComponent>()->getDescription() != "traffic")
								continue;
							QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->setActive(NewWantedState);
						}
						AnythingDone = true;
					}
				}
			}
			if (AnythingDone)
				mUiUnitViewer->trafficwarnerbutton->setChecked(NewWantedState);
		}

		void UnitViewer::onPushreverseButtonbutton(const bool pressed)
		{
			bool NewWantedState = mUiUnitViewer->reverseButton->isChecked();
			bool AnythingDone = false;
			for (auto a : GetSelectedEntity())
			{
				auto RoV = a->getComponent<em5::RoadVehicleComponent>();
				if (RoV != nullptr)
				{
					if (RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED).empty())
					{
						//everything emptY?
						QSF_LOG_PRINTS(INFO, "reverse light vector is empty")
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED))
						{
							auto ent = QSF_MAINMAP.getEntityById(LB);
							if (ent == nullptr || ent->getComponent<qsf::MetadataComponent>() == nullptr || ent->getComponent<qsf::MetadataComponent>()->getDescription() != "reverse")
								continue;
							QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->setActive(NewWantedState);
						}
						AnythingDone = true;
					}
				}
			}
			if (AnythingDone)
				mUiUnitViewer->reverseButton->setChecked(NewWantedState);
		}

		void UnitViewer::onPushtinteriorbutton(const bool pressed)
		{
			bool NewWantedState = mUiUnitViewer->interiorbutton->isChecked();
			bool AnythingDone = false;
			for (auto a : GetSelectedEntity())
			{
				auto RoV = a->getComponent<em5::RoadVehicleComponent>();
				if (RoV != nullptr)
				{
					if (RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED).empty())
					{
						//everything emptY?
						QSF_LOG_PRINTS(INFO, "interior (umfeld) light vector is empty")
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED))
						{
							auto ent = QSF_MAINMAP.getEntityById(LB);
							if (ent == nullptr || ent->getComponent<qsf::MetadataComponent>() == nullptr || ent->getComponent<qsf::MetadataComponent>()->getDescription() != "interior")
								continue;
							QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->setActive(NewWantedState);
						}
						AnythingDone = true;
					}
				}
			}
			if (AnythingDone)
				mUiUnitViewer->interiorbutton->setChecked(NewWantedState);
		}

		void UnitViewer::onPushenviromentbutton(const bool pressed)
		{
			bool NewWantedState = mUiUnitViewer->enviromentbutton->isChecked();
			bool AnythingDone = false;
			for (auto a : GetSelectedEntity())
			{
				auto RoV = a->getComponent<em5::RoadVehicleComponent>();
				if (RoV != nullptr)
				{
					if (RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED).empty())
					{
						//everything emptY?
						QSF_LOG_PRINTS(INFO, "enviroment light vector is empty")
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED))
						{
							auto ent = QSF_MAINMAP.getEntityById(LB);
							if (ent == nullptr || ent->getComponent<qsf::MetadataComponent>() == nullptr || ent->getComponent<qsf::MetadataComponent>()->getDescription() != "umfeld")
								continue;
							QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->setActive(NewWantedState);
						}
						AnythingDone = true;
					}
				}
			}
			if (AnythingDone)
				mUiUnitViewer->enviromentbutton->setChecked(NewWantedState);
		}

		void UnitViewer::onPush_AnalyseMeshButton(const bool pressed)
		{
			QSF_LOG_PRINTS(INFO, "[UnitViewer]Analys Mesh Button pressed")
				std::string TextbrowserText = "";
			std::string ArrayText = "";
			bool found = false;
			for (auto a : GetSelectedEntity())
			{
				found = true;
				auto MC = a->getComponent<qsf::MeshComponent>();
				if (MC == nullptr)
				{
					TextbrowserText = "[UnitViewer]Selected Entity has no Mesh Component (base/tintable/skinnable)";
					break;
				}
				QSF_LOG_PRINTS(INFO, "[UnitViewer]we have a mesh component");
				//we can analyse mesh
				if (MC->getMesh().getAsset() == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "[UnitViewer]error no mesh was set");
					found = false;
					break;
				}
				auto GID = MC->getMesh().getGlobalAssetId();
				auto AbsoluteFilename = qsf::AssetProxy(MC->getMesh().getGlobalAssetId()).getAbsoluteCachedAssetDataFilename();
				auto ADC = qsf::AssetDependencyCollector(GID);
				std::vector<uint64> EntityList;
				ADC.collectUniqueRecursiveAssetDependencies(EntityList);
				if (MC->getOgreEntity() == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "Ogre entity is a nullptr")
						return;
				}
				auto nMaxSubMesh = MC->getOgreEntity()->getNumSubEntities();
				for (int nSubMesh = 0; nSubMesh < nMaxSubMesh; nSubMesh++)
				{
					if (MC->getOgreEntity()->getSubEntity(nSubMesh) == nullptr || MC->getOgreEntity()->getSubEntity(nSubMesh)->getSubMesh() == nullptr)

					{
						QSF_LOG_PRINTS(INFO, "Submesh is not defined " << nSubMesh)
							continue;
					}
					//QSF_LOG_PRINTS(INFO,MC->getOgreEntity()->getSubEntity(nSubMesh)->getSubMesh()->getMaterialName().c_str());
					auto Mat = qsf::AssetProxy(boost::lexical_cast<uint64>(MC->getOgreEntity()->getSubEntity(nSubMesh)->getSubMesh()->getMaterialName().c_str()));
					if (Mat.getAsset() == nullptr)
					{
						QSF_LOG_PRINTS(INFO, "there was a undefined material for Submesh: " << nSubMesh)
							continue;
					}
					TextbrowserText += "[" + boost::lexical_cast<std::string>(nSubMesh) + "] " + Mat.getLocalAssetName() + " (" + boost::lexical_cast<std::string>(Mat.getGlobalAssetId()) + ")\n";
					ArrayText += "[" + boost::lexical_cast<std::string>(Mat.getGlobalAssetId()) + "]";

				}

				QSF_LOG_PRINTS(INFO, "[UnitViewer]we are done");
				break;
			}
			if (!found)
				TextbrowserText = "[UnitViewer]No Entity selected";
			mUiUnitViewer->SkinnableMeshArrayLineEdit->setText(ArrayText.c_str());
			TextbrowserText += ReadSkeleton();
			mUiUnitViewer->textBrowser->setText(TextbrowserText.c_str());
		}

		std::string UnitViewer::ReadSkeleton()
		{
			qsf::Entity* MyEnt = nullptr;
			for (auto a : GetSelectedEntity())
			{
				MyEnt = a;
				break;
			}
			if (MyEnt == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "No Entity Selected")
					return "";
			}
			auto MC = MyEnt->getComponent<qsf::MeshComponent>();
			if (MC == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Entity has no Mesh Component")
					return "";
			}
			if (MC->getOgreEntity() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "found no Ogre Entity")
					return "";
			}
			auto Skeleton = MC->getOgreEntity()->getSkeleton();
			if (Skeleton == nullptr)
			{
				//QSF_LOG_PRINTS(INFO, "found no Skeleton")
				return "";
			}
			int numBones = Skeleton->getNumBones();
			std::string Text = "\n\n Skeleton Data\n\n";
			for (unsigned short int iBone = 0; iBone < numBones; ++iBone)
			{

				auto pBone = Skeleton->getBone(iBone);
				if (!pBone)
				{
					assert(false);
					continue;
				}

				//Ogre::Entity *ent;
				//Ogre::TagPoint *tp;

				// Absolutely HAVE to create bone representations first. Otherwise we would get the wrong child count
				// because an attached object counts as a child
				// Would be nice to have a function that only gets the children that are bones...
				unsigned short numChildren = pBone->numChildren();
				if (numChildren == 0)
				{
					// There are no children, but we should still represent the bone
					// Creates a bone of length 1 for leaf bones (bones without children)
					//ent = mSceneMan->createEntity("SkeletonDebug/BoneMesh");
					//tp = mEntity->attachObjectToBone(pBone->getName(), (Ogre::MovableObject*)ent);
					//mBoneEntities.push_back(ent);
					//QSF_LOG_PRINTS(INFO, "[" << iBone << "] " << pBone->getName().c_str())
					Text += "[" + boost::lexical_cast<std::string>(iBone) + "] " + pBone->getName().c_str() + "\n";
				}
				else
				{
					for (int i = 0; i < numChildren; ++i)
					{
						Text += "[" + boost::lexical_cast<std::string>(iBone) + " | " + boost::lexical_cast<std::string>(i) + "] " + pBone->getName().c_str() + "\n";
						//QSF_LOG_PRINTS(INFO, "[" << iBone << " | " << i << " ] " << pBone->getChild(i)->getName().c_str())
							//Vector3 v = pBone->getChild(i)->getPosition();
							// If the length is zero, no point in creating the bone representation
							//float length = v.length();
							//if (length < 0.00001f)
								//continue;

							//ent = mSceneMan->createEntity("SkeletonDebug/BoneMesh");
							//tp = mEntity->attachObjectToBone(pBone->getName(), (Ogre::MovableObject*)ent);
							//mBoneEntities.push_back(ent);

							//tp->setScale(length, length, length);
					}
				}
			}
			return Text;
		}

		void UnitViewer::OnPushStartReadMaterial(const bool pressed)
		{
			if (!mMaterialViewerJob.isValid())
			{
				mMaterialViewerJob.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&UnitViewer::MaterialViewerJob, this, _1));
				mMaterialViewerJob.changeTimeBetweenCalls(qsf::Time::fromSeconds(0.1f));
				mUiUnitViewer->pushButtonUV_coordinates->setChecked(true);
			}
			else
			{
				mMaterialViewerJob.unregister();
				mUiUnitViewer->pushButtonUV_coordinates->setChecked(false);
			}
		}


		bool UnitViewer::IsEntityAllreadySelected(uint64 Target, std::vector<uint64> CompareList)
		{
			for (auto a : CompareList)
			{
				if (a == Target)
					return true;
			}
			return false;
		}

		void UnitViewer::UpdateStreetDebugNodes()
		{
			if (QSF_DEBUGDRAW.isRequestIdValid(mSelectedNodeDebug))
				QSF_DEBUGDRAW.cancelRequest(mSelectedNodeDebug);
			SelectedNodeDebug.mCircles.clear();

			for (auto a : DebugEntities)
			{
				auto entity = QSF_MAINMAP.getEntityById(a);
				if (entity == nullptr)
				{
					continue;
				}
				auto SCC = entity->getComponent<qsf::StreetCrossingComponent>();
				if (SCC == nullptr)
					continue;
				if (entity->getComponent<qsf::TransformComponent>() == nullptr)
					continue;
				auto entity_pos = em5::EntityHelper(entity).getPosition();
				qsf::Transform* ent_transform = &entity->getComponent<qsf::TransformComponent>()->getTransform();
				for (auto a : SCC->getStreetGateways())
				{
					if (a.getGatewayNodes().size() == 0)
						continue;
					glm::vec3 gatepos = glm::vec3(0, 0, 0);
					for (auto b : a.getGatewayNodes())
					{
						//auto nodepos = SCC->getNodes().at(b).getPosition();
						auto nodepos = ApplyMasterTransformToNode(ent_transform, SCC->getNodes().at(b).getPosition());
						SelectedNodeDebug.mCircles.push_back(qsf::CircleDebugDrawRequest(nodepos + entity_pos, qsf::CoordinateSystem::getUp(), 1.5f, qsf::Color4::RED, false));
						SelectedNodeDebug.mCircles.push_back(qsf::CircleDebugDrawRequest(nodepos + entity_pos, qsf::CoordinateSystem::getUp(), 0.2f, qsf::Color4::RED, true));
						gatepos += nodepos;

					}
					gatepos.x = gatepos.x / a.getGatewayNodes().size();
					gatepos.y = gatepos.y / a.getGatewayNodes().size();
					gatepos.z = gatepos.z / a.getGatewayNodes().size();
					gatepos += entity_pos;
					SelectedNodeDebug.mCircles.push_back(qsf::CircleDebugDrawRequest(gatepos, qsf::CoordinateSystem::getUp(), 1.5f, qsf::Color4::GREEN, false));
					SelectedNodeDebug.mCircles.push_back(qsf::CircleDebugDrawRequest(gatepos, qsf::CoordinateSystem::getUp(), 0.2f, qsf::Color4::RED, true));
				}

			}

			for (auto a : DebugEntitiesFullNodeView)
			{
				auto entity = QSF_MAINMAP.getEntityById(a);
				if (entity == nullptr)
				{
					continue;
				}
				auto SCC = entity->getComponent<qsf::StreetCrossingComponent>();
				if (SCC == nullptr)
					continue;
				if (entity->getComponent<qsf::TransformComponent>() == nullptr)
					continue;
				auto entity_pos = em5::EntityHelper(entity).getPosition();
				qsf::Transform* ent_transform = &entity->getComponent<qsf::TransformComponent>()->getTransform();
				for (auto a : SCC->getNodes())
				{
					//auto nodepos = SCC->getNodes().at(b).getPosition();
					auto nodepos = ApplyMasterTransformToNode(ent_transform, a.getPosition());
					SelectedNodeDebug.mCircles.push_back(qsf::CircleDebugDrawRequest(nodepos + entity_pos, qsf::CoordinateSystem::getUp(), 1.4f, qsf::Color4::BLUE, false));
					SelectedNodeDebug.mCircles.push_back(qsf::CircleDebugDrawRequest(nodepos + entity_pos, qsf::CoordinateSystem::getUp(), 0.1f, qsf::Color4::BLUE, true));
				}


			}
			mSelectedNodeDebug = QSF_DEBUGDRAW.requestDraw(SelectedNodeDebug);
		}

		void UnitViewer::OnSelectionChange_SetAdditionalLightButtons(QPushButton * Buttonname, std::string Lightdescription)
		{
			for (auto a : GetSelectedEntity())
			{
				auto RoV = a->getComponent<em5::RoadVehicleComponent>();
				if (RoV != nullptr)
				{
					if (RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED).empty())
					{
						//everything emptY?
						//QSF_LOG_PRINTS(INFO, "Head Light vector is empty")
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_UNDEFINED))
						{
							auto ent = QSF_MAINMAP.getEntityById(LB);
							if (ent == nullptr || ent->getComponent<qsf::MetadataComponent>() == nullptr || ent->getComponent<qsf::MetadataComponent>()->getDescription() != Lightdescription)
								continue;
							Buttonname->setChecked(QSF_MAINMAP.getEntityById(LB)->getComponent<qsf::game::LightControllerComponent>()->isActive());
							return;
						}
					}
				}
			}
			Buttonname->setChecked(false);
		}



		void UnitViewer::MaterialViewerJob(const qsf::JobArguments & jobArguments)
		{
			auto renderWindow = &QSF_EDITOR_APPLICATION.getMainWindow()->getRenderView().getRenderWindow();
			if (renderWindow == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no render window")
					return;
			}
			auto Height = renderWindow->getHeight();
			auto Width = renderWindow->getWidth();
			glm::vec2 mousePosition = QSF_INPUT.getMouse().getPosition();
			//MaterialName
			const glm::vec2 normalizedPosition = renderWindow->getNormalizedWindowSpaceCoords(mousePosition.x, mousePosition.y);
			qsf::RayMapQueryResponse rayMapQueryResponse(48);
			//qsf::RayMapQuery(QSF_MAINMAP).getFirstHitByRenderWindowNormalizedPosition(*renderWindow, normalizedPosition.x, normalizedPosition.y, rayMapQueryResponse);
			qsf::RayMapQuery(QSF_MAINMAP).getFirstHitByRenderWindow(*renderWindow, mousePosition.x, mousePosition.y, rayMapQueryResponse);
			if (rayMapQueryResponse.component == nullptr)
				return;
			uint64 Id = rayMapQueryResponse.component->getEntityId();
			//auto ent = &QSF_MAINMAP.getEntityById(Id)->getOrCreateComponent<qsf::LinkComponent>()->getTopmostAncestorLink(qsf::LinkComponent::SELECT_PARENT).getEntity();
			//return (ent->getId() == Target->getId());
			//if(rayMapQueryResponse.textureCoordinate.x)
			if (qsf::AssetProxy(rayMapQueryResponse.globalMaterialAssetId).getAsset() == nullptr)
				return;
			auto name = qsf::AssetProxy(rayMapQueryResponse.globalMaterialAssetId).getLocalAssetName();
			mUiUnitViewer->MaterialName->setText(name.c_str());

			//now load dds
			if(rayMapQueryResponse.textureCoordinate.x < 0.001)
				rayMapQueryResponse.textureCoordinate.x = 0;
			if (rayMapQueryResponse.textureCoordinate.y < 0.001)
				rayMapQueryResponse.textureCoordinate.y = 0;
			std::string i = "x = " + boost::lexical_cast<std::string>(rayMapQueryResponse.textureCoordinate.x) + " y = " + boost::lexical_cast<std::string>(rayMapQueryResponse.textureCoordinate.y);
			
			//QSF_LOG_PRINTS(INFO, rayMapQueryResponse_textcoordinate.textureCoordinate.y);
			//QSF_LOG_PRINTS(INFO, rayMapQueryResponse_textcoordinate.textureCoordinate.x);
			mUiUnitViewer->coordinates->setText(i.c_str());
			uint64 TextureName = qsf::getUninitialized<uint64>();
			bool NewMaterial = false;
			if (rayMapQueryResponse.globalMaterialAssetId == mOldAssetId)
			{
				mCrossState = CrossState::Active;
				TextureName = mOldAssetId;
			}
			else
			{
				NewMaterial = true;
				mOldAssetId = rayMapQueryResponse.globalMaterialAssetId;
				mCrossState = CrossState::Inactive;
				MyCrossPixels.clear();
				//pls avoid asset dependency collector
				//...
				auto a = qsf::AssetDependencyCollector(rayMapQueryResponse.globalMaterialAssetId);
				std::vector<uint64> CompleteList;
				a.collectAllDerivedAssets(CompleteList);
				for (auto b : CompleteList)
				{

					if (qsf::AssetProxy(b).getAsset() == nullptr)
						continue;
					if (qsf::AssetProxy(b).getAsset()->getTypeName() != "texture")
						continue;
					if (qsf::AssetProxy(b).getLocalAssetName().find("crgb_aa") == std::string::npos)
						continue;
					TextureName = b;
					break;
				}
			}
				if (TextureName == qsf::getUninitialized<uint64>())
				{
					QSF_LOG_PRINTS(INFO,"TextureName not init")
					return;
				}
				if (NewMaterial)
				{
					try
				{
					QSF_LOG_PRINTS(INFO, TextureName)
						auto Filepath = qsf::AssetProxy(TextureName).getAbsoluteCachedAssetDataFilename();


					Magick::Image image(Filepath);
					int mrows = (int)image.columns();
					int mcolumns = (int)image.rows();
					if (mrows <= 0 || mcolumns <= 0)
					{
						QSF_LOG_PRINTS(INFO, "broken image (size detection failed)" << Filepath)
							QSF_LOG_PRINTS(INFO, "width " << mrows << " height " << mcolumns << " channels " << image.channels())
							return;
					}
					if (image.channels() != 4 && image.channels() != 3)
					{
						QSF_LOG_PRINTS(INFO, "cant detect alpha channel of " << Filepath)
							return;
					}

					//int ScaleFactor = 1;
					/*if(w > 512)
					{
						ScaleFactor = w/512;
					}
					if(h > 512)
					{
						int mScale = h /512;
						if(ScaleFactor < mScale)
							ScaleFactor = mScale;
					}*/
						image.scale(Magick::Geometry(512, 512));
					QSF_LOG_PRINTS(INFO, "scale  image" << image.columns() << " " << image.rows())
						float Alpha = 0;
					image.syncPixels();
					mcolumns = (int)image.columns();
					mrows = (int)image.rows();
					MagickCore::Quantum *pixels = image.getPixels(0, 0, mcolumns, mrows);
					ImageRows = mrows;
					Imagecolumns = mcolumns;
					//QSF_LOG_PRINTS(INFO, "scaled image")
						
						//now create a normal map img
						//uint8* bufferColor = new uint8[w * h * 8];
						//Ogre::Image ColorMap;
						//ColorMap.loadDynamicImage(bufferColor, w, h, Ogre::PixelFormat::PF_FLOAT16_RGB);
						//auto channels = image.channels();
						//uint8* bufferAlpha = new uint8[w * h * 8]; //red
						//Ogre::Image AlphaMap;
						//AlphaMap.loadDynamicImage(bufferAlpha, w, h, Ogre::PixelFormat::PF_FLOAT32_GR);

						auto channels = image.channels();
						//QSF_LOG_PRINTS(INFO, "channels " << channels)
					//scan our dds
					//oben nach unten
					//QSF_LOG_PRINTS(INFO, "read img")
					for (size_t row = 0; row < mrows; row++)
					{
						//links nach rechts
						for (size_t column = 0; column < mcolumns; column++)
						{
							uint64 offset = (row* mcolumns + column) * channels; //4 is because we use 4 channels
							//QSF_LOG_PRINTS(INFO, "Offset " << offset << " " << row << " "<< w<< " " << column)
							float Red = (float)(*(pixels + offset) / 256);
							float Green = (float)(*(pixels + offset + 1) / 256);
							float Blue = (float)(*(pixels + offset + 2) / 256);
							if (channels == 4)
							{
								Alpha = (float)(*(pixels + offset + 3) / 256);
								//auto OgreValSpec = Ogre::ColourValue(Alpha, Alpha, Alpha);
								//AlphaMap.setColourAt(OgreValSpec, column, row, 0);
							}
							//z is not transfered so use
							//auto OgreValNormal = Ogre::ColourValue(Red, Green, Blue);
							//ColorMap.setColourAt(OgreValNormal, column, row, 0);

							mQImage->setPixel((int)column, (int)row, qRgba(Red, Green, Blue, Alpha));

						}
						//QSF_LOG_PRINTS(INFO, "set pixel " << row << " ")
					}
					QSF_LOG_PRINTS(INFO, "done reading")
					//make img blank (i.e. size of 256 * 512)
					for (size_t row = 0; row < 512; row++)
					{
						//links nach rechts
						for (size_t column = mcolumns; column < 512; column++)
						{
							mQImage->setPixel((int)column, (int)row, qRgba(0, 0, 0, 255));

						}
						//QSF_LOG_PRINTS(INFO, "set pixel " << row << " ")
					}
					for (size_t row = mrows; row < 512; row++)
					{
						//links nach rechts
						for (size_t column = 0; column < 512; column++)
						{
							mQImage->setPixel((int)column, (int)row, qRgba(0, 0, 0, 255));

						}
						//QSF_LOG_PRINTS(INFO, "set pixel " << row << " ")
					}
					QSF_LOG_PRINTS(INFO, "Wrote Image")
						//mUiUnitViewer->graphicsView->setBackgroundBrush(*mQImage);
							//mUiUnitViewer->graphicsView->setCacheMode(QGraphicsView::CacheBackground);
							//mUiUnitViewer->graphicsView->show();
						mUiUnitViewer->label_4->setPixmap(QPixmap::fromImage(*mQImage));
					QSF_LOG_PRINTS(INFO, "Applied Image")
						//graphicsView->
							//delete[] bufferColor;
						//delete[] bufferAlpha;
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO, e.what())
						return;
				}
			}

			if (mCrossState == CrossState::Active)
			{
				//delete old Cross Values
				for (auto a : MyCrossPixels)
				{
					mQImage->setPixelColor((int)a.X, (int)a.Y, qRgba(a.Red, a.Green, a.Blue, a.Alpha));
				}
				MyCrossPixels.clear();

			}
				float X = rayMapQueryResponse.textureCoordinate.x;
				if(X >= 0)
				{
					X = X - glm::floor(X);
				}
				else
				{
					X=  glm::ceil(X)-X;
				}
				
				float Y = rayMapQueryResponse.textureCoordinate.y;
				if (Y >= 0)
				{
					Y = Y - glm::floor(Y);
				}
				else
				{
					Y =  glm::ceil(Y)-Y;
				}
				//QSF_LOG_PRINTS(INFO, "do sth at " << X << " " << Y)
				X = X*Imagecolumns;
				Y = Y*ImageRows;
				//QSF_LOG_PRINTS(INFO,"do sth at "<< (int)X << " "<< (int) Y)
				for (int CrossX = glm::max(0, (int)X - 10); CrossX < glm::min(512, (int)X + 10); CrossX++)
				{
					if(CrossX == (int)X)
					continue;
					CrossPixel CP;
					auto Color = mQImage->pixelColor(CrossX,(int)Y);
					CP.Red= Color.red();
					CP.Green = Color.green();
					CP.Blue = Color.blue();
					CP.Alpha = Color.alpha();
					CP.X = CrossX;
					CP.Y = (int)Y;
					MyCrossPixels.push_back(CP);
					mQImage->setPixelColor((int)CrossX, (int)Y, qRgba(100, 255, 100, 0));
				}
				for (int CrossY = glm::max(0, (int)Y - 10); CrossY < glm::min(512, (int)Y + 10); CrossY++)
				{
					if (CrossY == (int)Y)
						continue;
					CrossPixel CP;
					auto Color = mQImage->pixelColor((int)X,CrossY);
					CP.Red = Color.red();
					CP.Green = Color.green();
					CP.Blue = Color.blue();
					CP.Alpha = Color.alpha();
					CP.Y = CrossY;
					CP.X = (int)X;
					MyCrossPixels.push_back(CP);
					mQImage->setPixelColor((int)X, (int)CrossY, qRgba(100, 255, 100, 0));
				}
				
				mUiUnitViewer->label_4->setPixmap(QPixmap::fromImage(*mQImage));
				mUiUnitViewer->label_4->repaint();
				//mUiUnitViewer->scrollArea->setWidget(mUiUnitViewer->label_4);
				/*if(AE == nullptr)
				{ 
				AE = new QScrollArea;
				AE->setBackgroundRole(QPalette::Dark);
				AE->setWidget(mUiUnitViewer->label_4);
				}*/

		}

		void UnitViewer::TireJob(const qsf::JobArguments & jobArguments)
		{
			for (auto a : AffectedByTire)
			{
				if (a == nullptr)
					continue;
				auto RC = a->getComponent<em5::RoadVehicleComponent>();
				if (RC == nullptr)
					continue;
				for (size_t t = 0; t < RC->VehicleWheelsArray.size(); t++)
				{
					qsf::Entity* tire = QSF_MAINMAP.getEntityById(RC->VehicleWheelsArray.get(t));
					if (tire == nullptr)
						continue;
					auto WC = tire->getComponent<em5::WheelComponent>();
					if (WC == nullptr)
						continue;
					const float moveDistance = 3.f * jobArguments.getTimePassed().getSeconds();
					float Val = -1.f*mUiUnitViewer->horizontalSlider->value();
					if (WC->getWheelType() == WC->WHEELTYPE_FRONT_LEFT || WC->getWheelType() == WC->WHEELTYPE_FRONT_RIGHT)
					{
						WC->updateWheel(moveDistance, Val);
					}
					else //no support for chains
					{
						WC->updateWheel(moveDistance, 0);
					}
				}
			}
		}

		void UnitViewer::ResetWheelsAfterDeselection()
		{
			mTireJob.unregister();
			for (auto a : AffectedByTire)
			{
				if (a == nullptr)
					continue;
				auto RC = a->getComponent<em5::RoadVehicleComponent>();
				if (RC == nullptr)
					continue;
				for (size_t t = 0; t < RC->VehicleWheelsArray.size(); t++)
				{
					qsf::Entity* tire = QSF_MAINMAP.getEntityById(RC->VehicleWheelsArray.get(t));
					if (tire == nullptr)
						continue;
					auto WC = tire->getComponent<em5::WheelComponent>();
					if (WC == nullptr)
						continue;
					if (WC->getEntity().getComponent<qsf::LinkComponent>() == nullptr)
						continue;
					WC->getEntity().getComponent<qsf::LinkComponent>()->setLocalRotation(WC->getOriginalLocalRotation());
				}
			}
			mUiUnitViewer->rotatetires->setChecked(false);
			mUiUnitViewer->horizontalSlider->setValue(0);
		}

		glm::vec3 UnitViewer::ApplyMasterTransformToNode(qsf::Transform* Transform, glm::vec3 NodePos)
		{

			/*
			Vector3 P1, P2; //your points
	Quaternion rot; //the rotation

	var v = P1 - P2; //the relative vector from P2 to P1.
	v = rot * v; //rotatate
	v = P2 + v; //bring back to world space
				*/
			NodePos.x = NodePos.x*Transform->getScale().x;
			NodePos.y = NodePos.y*Transform->getScale().y;
			NodePos.z = NodePos.z*Transform->getScale().z;
			NodePos = Transform->getRotation()*NodePos;
			return NodePos;
		}


		std::vector<qsf::Entity*> UnitViewer::GetSelectedEntity()
		{
			std::vector<qsf::Entity*> Entities;
			for (auto a : QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>().getSelectedIdSet())
			{
				auto ent = QSF_MAINMAP.getEntityById(a);
				if (ent == nullptr)
					continue;
				Entities.push_back(ent);

			}
			return Entities;
		}






		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]
		void UnitViewer::showEvent(QShowEvent* qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);

			// Perform a GUI rebuild to ensure the GUI content is up-to-date
			rebuildGui();
			//boost::signals2::signal<void(const LogMessage&)> NewMessage;

			// Connect Qt signals/slots
			//connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &UnitViewer::onUndoOperationExecuted);
			//connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &UnitViewer::onRedoOperationExecuted);
		}

		void UnitViewer::hideEvent(QHideEvent* qHideEvent)
		{
			if (QSF_DEBUGDRAW.isRequestIdValid(mSelectedNodeDebug))
				QSF_DEBUGDRAW.cancelRequest(mSelectedNodeDebug);
			mTireJob.unregister();
			// Call the base implementation
			View::hideEvent(qHideEvent);
			// Disconnect Qt signals/slots
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &UnitViewer::onUndoOperationExecuted);
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &UnitViewer::onRedoOperationExecuted);
		}


		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		void UnitViewer::onPushSelectButton(const bool pressed)
		{


		}








		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
