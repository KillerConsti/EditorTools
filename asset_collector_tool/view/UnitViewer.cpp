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

			
			//trial and error
			//
		}

		UnitViewer::~UnitViewer()
		{
			if (QSF_DEBUGDRAW.isRequestIdValid(mSelectedNodeDebug))
				QSF_DEBUGDRAW.cancelRequest(mSelectedNodeDebug);
			mTireJob.unregister();
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
				connect(mUiUnitViewer->pushAddAllNodes, SIGNAL(clicked(bool)), this,  SLOT(onpushAddAllNodes(bool)));
				//connect(mUiUnitViewer->setsimulting, SIGNAL(clicked(bool)), this, SLOT(OnPushSetSimulating(bool)));
				//connect(mUITrainTrackTool->pushButton_fill_tree_view, SIGNAL(clicked(bool)), this, SLOT(CreatePathEntities(bool)));
				/*connect(mUiUnitViewer->pushCheckUnit, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
				connect(mUiUnitViewer->loadexternallog, SIGNAL(clicked(bool)), this, SLOT(OnLoadLogFile(bool)));
				connect(mUiUnitViewer->searhprototype, SIGNAL(clicked(bool)), this, SLOT(OnPushSearchForPrototype(bool)));
				connect(mUiUnitViewer->export_2, SIGNAL(clicked(bool)), this, SLOT(OnSaveLogFile(bool)));*/

				//connect(mUiUnitViewer->comboBoxType, SIGNAL(currentIndexChanged(int)), this, SLOT(onCurrentIndexChanged(int)));
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
			OnSelectionChange_SetAdditionalLightButtons(mUiUnitViewer->enviromentbutton,"umfeld");
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
						QSF_LOG_PRINTS(INFO, "Head Light vector is empty")
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
				if (IsEntityAllreadySelected(a,DebugEntities))
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
						QSF_LOG_PRINTS(INFO, "Head Light vector is empty")
					}
					else
					{
						for (auto LB : RoV->getVehicleLightIdsByType(qsf::game::LightControllerComponent::LIGHTPOSITION_BRAKE))
						{
							if(QSF_MAINMAP.getEntityById(LB) == nullptr) continue;
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
						QSF_LOG_PRINTS(INFO, "Head Light vector is empty")
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
