// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/indicator/UnitPlacerView.h"
#include "ui_UnitPlacerView.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

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
		const uint32 UnitPlacerView::PLUGINABLE_ID = qsf::StringHash("qsf::editor::UnitPlacerView");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		UnitPlacerView::UnitPlacerView(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
			View(viewManager, qWidgetParent),
			mUiUnitPlacerView(nullptr),
			IsActive(false),
			MoveDelta(0.f),
			CreatedUnits(),
			WasPressed(false)
		{
			// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
			addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);
		}

		UnitPlacerView::~UnitPlacerView()
		{
			// Destroy the UI view instance
			if (nullptr != mUiUnitPlacerView)
			{
				delete mUiUnitPlacerView;
			}
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		void UnitPlacerView::retranslateUi()
		{
			// Retranslate the content of the UI, this automatically clears the previous content
			mUiUnitPlacerView->retranslateUi(this);

		/*	QComboBox* qComboBoxViewMode = mUiUnitPlacerView->comboBoxType;
			qComboBoxViewMode->clear();
			qComboBoxViewMode->addItem(tr("ID_UnitPlacerView_ALL"));
			qComboBoxViewMode->addItem(tr("ID_UnitPlacerView_WHITE"));
			qComboBoxViewMode->addItem(tr("ID_UnitPlacerView_RED"));
			qComboBoxViewMode->addItem(tr("ID_UnitPlacerView_YELLOW"));
			qComboBoxViewMode->addItem(tr("ID_UnitPlacerView_GREEN"));
			qComboBoxViewMode->addItem(tr("ID_UnitPlacerView_BLUE"));
			qComboBoxViewMode->setCurrentIndex(0);*/
			mUiUnitPlacerView->pushButtonSelect->setText("Place Units");
			this->setWindowTitle("Unit Placer");
			
		}

		void UnitPlacerView::changeVisibility(bool visible)
		{
			// Lazy evaluation: If the view is shown the first time, create its content
			if (visible && nullptr == mUiUnitPlacerView)
			{
				// Setup the view content
				QWidget* contentWidget = new QWidget(this);
				{
					// Load content to widget
					mUiUnitPlacerView = new Ui::UnitPlacerView();
					mUiUnitPlacerView->setupUi(contentWidget);
				}

				// Set content to view
				setWidget(contentWidget);

				// Connect Qt signals/slots

				connect(mUiUnitPlacerView->pushButtonSelect, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
				connect(mUiUnitPlacerView->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(onMinimumSliderChanged(int)));
				connect(mUiUnitPlacerView->horizontalSlider_2, SIGNAL(valueChanged(int)), this, SLOT(onMaximumSliderChanged(int)));
				
				connect(mUiUnitPlacerView->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
				connect(mUiUnitPlacerView->lineEdit_2, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
				connect(mUiUnitPlacerView->lineEdit_3, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));

				connect(mUiUnitPlacerView->horizontalSlider_3, SIGNAL(valueChanged(int)), this, SLOT(onPrefab_1SliderChanged(int)));
				connect(mUiUnitPlacerView->horizontalSlider_4, SIGNAL(valueChanged(int)), this, SLOT(onPrefab_2SliderChanged(int)));
				connect(mUiUnitPlacerView->horizontalSlider_5, SIGNAL(valueChanged(int)), this, SLOT(onPrefab_3SliderChanged(int)));


					MarkNotGuiltyPrefabs();
			}
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void UnitPlacerView::rebuildGui()
		{
			return;
			// Count all entities from the map
			int count = 0;
			for (const qsf::Entity* entity : QSF_MAINMAP.getEntities())
			{
				if (nullptr != entity->getComponentById(qsf::StringHash("qsf::MetadataComponent")))
				{
					++count;
				}
			}
			// Update the number in spinbox
			mUiUnitPlacerView->spinBoxCounter->setValue(count);
			
		}

		void UnitPlacerView::componentCreated()
		{
			return;
			// Increase the counter in spinbox
			QSpinBox* spinBox = mUiUnitPlacerView->spinBoxCounter;
			int oldValue = spinBox->value();
			spinBox->setValue(++oldValue);
		}

		void UnitPlacerView::componentDestroyed()
		{
			return;
			// Decrease the counter in spinbox
			QSpinBox* spinBox = mUiUnitPlacerView->spinBoxCounter;
			int oldValue = spinBox->value();
			spinBox->setValue(--oldValue);
		}

		void UnitPlacerView::updateComponentData(uint64 entityId)
		{
			// TODO(user) fill with live
		}


		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]
		void UnitPlacerView::showEvent(QShowEvent* qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);

			// Perform a GUI rebuild to ensure the GUI content is up-to-date
			rebuildGui();

			// Connect Qt signals/slots
			connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &UnitPlacerView::onUndoOperationExecuted);
			connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &UnitPlacerView::onRedoOperationExecuted);
		}

		void UnitPlacerView::hideEvent(QHideEvent* qHideEvent)
		{
			// Call the base implementation
			View::hideEvent(qHideEvent);

			// Disconnect Qt signals/slots
			disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &UnitPlacerView::onUndoOperationExecuted);
			disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &UnitPlacerView::onRedoOperationExecuted);

			PaintJobProxy.unregister();
			mDebugDrawProxy.unregister();
		}

		void UnitPlacerView::PaintJob(const qsf::JobArguments & jobArguments)
		{
			if (QSF_INPUT.getKeyboard().Escape.isPressed())
			{
				IsActive = false;
				mUiUnitPlacerView->pushButtonSelect->setText("Place Units");
				auto a = QSF_EDITOR_SELECTION_SYSTEM.get<qsf::editor::EntitySelectionManager>();
				a->setSelectionByIdSet(CreatedUnits);
				CreatedUnits.clear();
				PaintJobProxy.unregister();
				mDebugDrawProxy.unregister();
				return;
			}
			mDebugDrawProxy.registerAt(QSF_MAINMAP.getDebugDrawManager());
			glm::vec3 position = getPositionUnderMouse();
			float Radius = 1.f* mUiUnitPlacerView->spinBoxCounter->value()*0.2f;
			mDebugDrawProxy.addRequest(qsf::CircleDebugDrawRequest(position, qsf::CoordinateSystem::getUp(), Radius, qsf::Color4::GREEN));

			MoveDelta += glm::distance(OldPos, position);
			OldPos = position;

			if (QSF_INPUT.getMouse().Left.isPressed())
			{
				if (MoveDelta > mUiUnitPlacerView->doubleSpinBox->value())
				{
					CreateUnit(position);

				}
			}
			else
			{
				MoveDelta = 0.f;
			}
		}


		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		void UnitPlacerView::onPushSelectButton(const bool pressed)
		{
			IsActive = !IsActive;
			if (IsActive)
			{
				mUiUnitPlacerView->pushButtonSelect->setText("Stop Placing Units");
				CreatedUnits.clear();
				PaintJobProxy.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&UnitPlacerView::PaintJob, this, _1));
			}
			else
			{
				mUiUnitPlacerView->pushButtonSelect->setText("Place Units");
				auto a = QSF_EDITOR_SELECTION_SYSTEM.get<qsf::editor::EntitySelectionManager>();
					a->setSelectionByIdSet(CreatedUnits);
				CreatedUnits.clear();
				PaintJobProxy.unregister();
				mDebugDrawProxy.unregister();
			}
			return;
			//default
			//for (size_t t=0;QSF_MAINMAP.getEntities().size();t++)
			// Cycle through all entities of the map
			for (const qsf::Entity* entity : QSF_MAINMAP.getEntities())
			{
				// Look for indicator component
				if (entity->getComponent<qsf::MetadataComponent>()!= nullptr)
				{
					// Get the unique entity id
					uint64 entityId = entity->getId();

					// Check if the current entity is selected
					qsf::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::EntitySelectionManager>();
					if (!entitySelectionManager.isIdSelected(entityId))
					{
						// Select and fly camera to entity
						entitySelectionManager.setSelectionById(entityId);
						QSF_EDITOR_APPLICATION.getCameraManager().flyCameraToEntity(*entity);
						break;
					}
					/*if (!entitySelectionManager.isIdSelected(entityId))
					{
						
					}*/
				}
			}
		}

		void UnitPlacerView::onCurrentIndexChanged(int index)
		{
			//QString displayText(mUiUnitPlacerView->comboBoxType->itemText(index).toUtf8().constData());
			//mUiUnitPlacerView->lineEditSelected->setText(displayText);
		}

		void UnitPlacerView::onUndoOperationExecuted(const qsf::editor::base::Operation& operation)
		{
			const uint32 operationId = operation.getId();
			if (qsf::editor::base::CreateComponentOperation::OPERATION_ID == operationId)
			{
				// Check whether or not the component is an Indicator component
				const qsf::editor::base::CreateComponentOperation* createComponentOperation = static_cast<qsf::editor::base::CreateComponentOperation*>(const_cast<qsf::editor::base::Operation*>(&operation));
				if (createComponentOperation->getComponentId() == qsf::MetadataComponent::COMPONENT_ID)
				{
					// Inform the view that a component has been destroyed
					componentDestroyed();
				}
			}
			else if (qsf::editor::base::CreateEntityOperation::OPERATION_ID == operationId)
			{
				rebuildGui();
			}
			else if (qsf::editor::base::DestroyEntityOperation::OPERATION_ID == operationId || qsf::editor::base::DestroyComponentOperation::OPERATION_ID == operationId)
			{
				// Inform the view that a component has been created
				componentCreated();
			}
			else if (qsf::editor::base::SetComponentPropertyOperation::OPERATION_ID == operationId)
			{
				// Inform the view that the indicator component has been changed
				updateComponentData(static_cast<const qsf::editor::base::EntityOperation&>(operation).getEntityId());
			}
			else if (qsf::editor::RebuildGuiOperation::OPERATION_ID == operationId)
			{
				rebuildGui();
			}
		}

		void UnitPlacerView::onRedoOperationExecuted(const qsf::editor::base::Operation& operation)
		{
			const uint32 operationId = operation.getId();
			if (qsf::editor::base::CreateComponentOperation::OPERATION_ID == operationId)
			{
				// Check whether or not the component is an indicator component
				const qsf::editor::base::CreateComponentOperation* createComponentOperation = static_cast<qsf::editor::base::CreateComponentOperation*>(const_cast<qsf::editor::base::Operation*>(&operation));
				if (createComponentOperation->getComponentId() == qsf::MetadataComponent::COMPONENT_ID)
				{
					// Inform the view browser that a component has been created
					componentCreated();
				}
			}
			else if (qsf::editor::base::CreateEntityOperation::OPERATION_ID == operationId)
			{
				// Check whether or not the component is an indicator component
				const qsf::editor::base::CreateEntityOperation* createEntityOperation = static_cast<qsf::editor::base::CreateEntityOperation*>(const_cast<qsf::editor::base::Operation*>(&operation));
				qsf::Entity* entity = QSF_MAINMAP.getEntityById(createEntityOperation->getEntityId());
				if (nullptr != entity && nullptr != entity->getComponent<qsf::MetadataComponent>())
				{
					// Inform the view browser that a component has been created
					componentCreated();
				}
			}
			else if (qsf::editor::base::DestroyEntityOperation::OPERATION_ID == operationId || qsf::editor::base::DestroyComponentOperation::OPERATION_ID == operationId)
			{
				const uint32 componentId = static_cast<const qsf::editor::base::ComponentOperation&>(operation).getComponentId();
				if (componentId == qsf::MetadataComponent::COMPONENT_ID)
				{
					// Inform the view that a component has been destroyed
					componentDestroyed();
				}
				
			}
			else if (qsf::editor::base::SetComponentPropertyOperation::OPERATION_ID == operationId)
			{
				// Inform the view that the indicator component has been changed
				updateComponentData(static_cast<const qsf::editor::base::EntityOperation&>(operation).getEntityId());
			}
			else if (qsf::editor::RebuildGuiOperation::OPERATION_ID == operationId)
			{
				rebuildGui();
			}
		}

		glm::vec3 UnitPlacerView::getPositionUnderMouse()
		{
			glm::vec2 mousePosition = QSF_INPUT.getMouse().getPosition();
			qsf::RayMapQueryResponse response = qsf::RayMapQueryResponse(qsf::RayMapQueryResponse::POSITION_RESPONSE);
			//QSF_WINDOW.getWindows().at(0)->getNativeWindowHandle()
			qsf::ComponentCollection::ComponentList<qsf::CameraControlComponent> QueryFireComponents = qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<qsf::CameraControlComponent>();
			for (auto a : QueryFireComponents)
			{

				auto CC = a->getEntity().getComponent<qsf::CameraComponent>();
				if (CC == nullptr)
					continue;
				if (CC->getRenderWindows().size() == 0)
					continue;
				//this creates a ray which does not have a standart PUBLIC constructor?
				auto pos = CC->getRayAtViewportPosition(mousePosition.x, mousePosition.y);
				qsf::RendererHelper::getViewportRayAtWindowPosition(pos, *CC->getRenderWindows().at(0), mousePosition.x, mousePosition.y);
				float t = a->getTargetPosition().y - pos.getOrigin().y;
				t = t / pos.getDirection().y;
				//QSF_LOG_PRINTS(INFO, t << " " << pos.getPoint(t).x << "  " << pos.getPoint(t).y << "  " << pos.getPoint(t).z)
					return glm::vec3(pos.getPoint(t));

			}
			return glm::vec3(0, 0, 0);
		}

		void UnitPlacerView::CreateUnit(glm::vec3 position)
		{
			float Radius = 1.f* mUiUnitPlacerView->spinBoxCounter->value()*0.2f;

			//which prefabs are active=
			bool Prefab1 = mUiUnitPlacerView->lineEdit_6->isEnabled();
			bool Prefab2 = mUiUnitPlacerView->lineEdit_7->isEnabled();
			bool Prefab3 = mUiUnitPlacerView->lineEdit_8->isEnabled();
			int num = 0;
			//what are the weights
			if (Prefab1)
				num = mUiUnitPlacerView->horizontalSlider_3->value();
			if (Prefab2)
				num += mUiUnitPlacerView->horizontalSlider_4->value();
			if (Prefab3)
				num += mUiUnitPlacerView->horizontalSlider_5->value();
		
			qsf::StringHash  shash = 0;
			//get a unit based on the weight!
			num = qsf::Random::getRandomInt(0, num);
			//QSF_LOG_PRINTS(INFO,num)
			if (Prefab1)
			{
				num -= mUiUnitPlacerView->horizontalSlider_3->value();
			}
			if (num <= 0)
			{
				shash = qsf::StringHash(mUiUnitPlacerView->lineEdit->text().toStdString());
			}
			else
			{
				if (Prefab2)
				{
					num -= mUiUnitPlacerView->horizontalSlider_4->value();
				}
				if (num <= 0)
				{
					shash = qsf::StringHash(mUiUnitPlacerView->lineEdit_2->text().toStdString());
				}
				else
				{
					shash = qsf::StringHash(mUiUnitPlacerView->lineEdit_3->text().toStdString());
				}
			}

			float groundHeight = 0.f;
			qsf::GroundMapQuery(QSF_MAINMAP, em5::GroundMaps::FILTER_DEFAULT).getHeightAt(position, groundHeight);
			position.y = groundHeight;
			float ValueR = qsf::Random::getRandomFloat(0.f, mUiUnitPlacerView->spinBoxCounter->value()*0.2f);
			float Angle = qsf::Random::getRandomFloat(0.f, 4 * glm::pi<float>());

			float OffsetX = Radius*glm::sin(Angle);
			float OffsetY = Radius*glm::cos(Angle);

			float RotX = qsf::Random::getRandomFloat(0.f, 4 * glm::pi<float>());

			//this is quite complicated as we need the layer, the prototype of the prefab  and much more ;)
			uint64 entityId = BuildEntity(position,shash);
			if (entityId == qsf::getUninitialized<uint64>())
			{
				return;
			}
			qsf::Entity* entity = QSF_MAINMAP.getEntityById(entityId);
			if (entity == nullptr)
				return;
			float ScaleX = qsf::Random::getRandomFloat(mUiUnitPlacerView->lineEdit_4->text().toDouble(), mUiUnitPlacerView->lineEdit_5->text().toDouble());
			float ScaleY = qsf::Random::getRandomFloat(mUiUnitPlacerView->lineEdit_4->text().toDouble(), mUiUnitPlacerView->lineEdit_5->text().toDouble());
			float ScaleZ = qsf::Random::getRandomFloat(mUiUnitPlacerView->lineEdit_4->text().toDouble(), mUiUnitPlacerView->lineEdit_5->text().toDouble());
			entity->getOrCreateComponent<qsf::TransformComponent>()->setPosition(position + glm::vec3(OffsetX, 0, OffsetY));
			if(mUiUnitPlacerView->checkBox->isChecked())
			entity->getOrCreateComponent<qsf::TransformComponent>()->setRotation(entity->getOrCreateComponent<qsf::TransformComponent>()->getRotation()*qsf::EulerAngles::eulerToQuaternion(glm::vec3(RotX, 0, 0)));
			entity->getOrCreateComponent<qsf::TransformComponent>()->setScale(glm::vec3(ScaleX, ScaleY, ScaleZ));


			qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
			compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(entity->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::TransformComponent::ROTATION, entity->getOrCreateComponent<qsf::TransformComponent>()->getRotation()));
			compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(entity->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::TransformComponent::SCALE, entity->getOrCreateComponent<qsf::TransformComponent>()->getScale()));
			compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(entity->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::TransformComponent::POSITION, entity->getOrCreateComponent<qsf::TransformComponent>()->getPosition()));
			QSF_EDITOR_OPERATION.push(compoundOperation2);

			//finally reset the tool
			MoveDelta = 0.f;
			//add it to the selection
			CreatedUnits.insert(entity->getId());
			
		}

		void UnitPlacerView::onMinimumSliderChanged(const int value)
		{
			std::stringstream ss;
			ss << 1.0*mUiUnitPlacerView->horizontalSlider->value() / 10.0;
			mUiUnitPlacerView->lineEdit_4->setText(ss.str().c_str());
		}

		void UnitPlacerView::onMaximumSliderChanged(const int value)
		{
			std::stringstream ss;
			ss << 1.0*mUiUnitPlacerView->horizontalSlider_2->value() / 10.0;
			mUiUnitPlacerView->lineEdit_5->setText(ss.str().c_str());
		}

		void UnitPlacerView::onEditPrefab(const QString & text)
		{
			//QSF_LOG_PRINTS(INFO, "Signal recieved");
			MarkNotGuiltyPrefabs();
		}

		void UnitPlacerView::onPrefab_1SliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUiUnitPlacerView->horizontalSlider_3->value();
			mUiUnitPlacerView->lineEdit_6->setText(ss.str().c_str());
		}

		void UnitPlacerView::onPrefab_2SliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUiUnitPlacerView->horizontalSlider_4->value();
			mUiUnitPlacerView->lineEdit_7->setText(ss.str().c_str());
		}

		void UnitPlacerView::onPrefab_3SliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUiUnitPlacerView->horizontalSlider_5->value();
			mUiUnitPlacerView->lineEdit_8->setText(ss.str().c_str());
		}

		void UnitPlacerView::buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select)
		{
			// Cycle through link components in forward order, so that parents are created before their children
			for (size_t index = 0; index < temporaryPrototypes.size(); ++index)
			{
				qsf::Prototype& prototype = *temporaryPrototypes[index];

				// Backup the temporary prototype
				qsf::editor::base::BackupPrototypeOperationData* backupPrototypeOperationData = new qsf::editor::base::BackupPrototypeOperationData(prototype);
				compoundOperation.addOperationData(backupPrototypeOperationData);

				// Create entity
				qsf::editor::base::CreateEntityOperation* createEntityOperation = new qsf::editor::base::CreateEntityOperation(prototype.getId(), layerId, backupPrototypeOperationData);
				compoundOperation.pushBackOperation(createEntityOperation);

				// Is this the primary prototype?
				if (index == 0)
				{
					// Set text for compound operation
					compoundOperation.setText(createEntityOperation->getText());
				}
			}

			// In case there's a select operation, we have to commit it after all operations to ensure everything required is already there
			if (select)
			{
				// TODO(co) We should call "qsf::editor::EntityOperationHelper::buildSelectOperation(primaryEntityId)" to properly select everything in case we just instanced a group,
				//          at the moment this is not possible because we would need a concrete instance at this point which we don't have until the operation has been executed
				const uint64 primaryPrototypeId = temporaryPrototypes[0]->getId();
				//compoundOperation.pushBackOperation(QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>().createSelectEntityOperation(primaryPrototypeId));
			}
		}

		uint64 UnitPlacerView::BuildEntity(glm::vec3 position, qsf::StringHash shash)
		{
			//Create Unit <-> this is quite long
			qsf::Prototype* prototype = QSF_MAINPROTOTYPE.getPrefabByLocalAssetId(shash);
			if (prototype == nullptr)
			{
				return qsf::getUninitialized<uint64>();
			}

			const uint32 selectedLayerId = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::LayerSelectionManager>().getSelectedId();
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
				const qsf::Asset* asset = &qsf::Asset(qsf::AssetProxy(shash).getGlobalAssetId());
				const std::string* name = (nullptr != asset) ? &asset->getName() : nullptr;
				usedPrefabContent->processForEntityInstancing(originalPrototypes, globalPrefabAssetId, name, &position);
			}
			if (usedPrefabContent->getPrototypes().at(0)->getComponent<qsf::MeshComponent>() != nullptr)
				usedPrefabContent->getPrototypes().at(0)->destroyComponent<qsf::DebugBoxComponent>();
			// Build compound operation that creates a copy of the cloned prototypes in the map
			qsf::editor::base::CompoundOperation* compoundOperation = new qsf::editor::base::CompoundOperation();
			buildInstantiateTemporaryPrototypesOperation(*compoundOperation, usedPrefabContent->getPrototypes(), selectedLayerId, false);
			uint64 entityId = usedPrefabContent->getMainPrototype()->getId();
			QSF_EDITOR_OPERATION.push(compoundOperation);
			return entityId;
			//Unit is created now apply transform
		}

		void UnitPlacerView::MarkNotGuiltyPrefabs()
		{
			if (qsf::AssetProxy(mUiUnitPlacerView->lineEdit->text().toStdString()).getAsset() != nullptr)
			{
				mUiUnitPlacerView->lineEdit->setAutoFillBackground(false);
				mUiUnitPlacerView->horizontalSlider_3->setEnabled(true);
				mUiUnitPlacerView->lineEdit_6->setEnabled(true);
			}
			else
			{
				mUiUnitPlacerView->lineEdit->setAutoFillBackground(true);
				mUiUnitPlacerView->horizontalSlider_3->setEnabled(false);
				mUiUnitPlacerView->lineEdit_6->setEnabled(false);
			}

			if (qsf::AssetProxy(mUiUnitPlacerView->lineEdit_2->text().toStdString()).getAsset() != nullptr)
			{
				mUiUnitPlacerView->lineEdit_2->setAutoFillBackground(false);
				mUiUnitPlacerView->horizontalSlider_4->setEnabled(true);
				mUiUnitPlacerView->lineEdit_7->setEnabled(true);
			}
			else
			{
				mUiUnitPlacerView->lineEdit_2->setAutoFillBackground(true);
				mUiUnitPlacerView->horizontalSlider_4->setEnabled(false);
				mUiUnitPlacerView->lineEdit_7->setEnabled(false);
			}

			if (qsf::AssetProxy(mUiUnitPlacerView->lineEdit_3->text().toStdString()).getAsset() != nullptr)
			{
				mUiUnitPlacerView->lineEdit_3->setAutoFillBackground(false);
				mUiUnitPlacerView->horizontalSlider_5->setEnabled(true);
				mUiUnitPlacerView->lineEdit_8->setEnabled(true);
			}
			else
			{
				mUiUnitPlacerView->lineEdit_3->setAutoFillBackground(true);
				mUiUnitPlacerView->horizontalSlider_5->setEnabled(false);
				mUiUnitPlacerView->lineEdit_8->setEnabled(false);
			}
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // user
