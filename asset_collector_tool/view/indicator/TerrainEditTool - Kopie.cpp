// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/indicator/TerrainEditTool.h"
#include "ui_TerrainEditTool.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

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
		const uint32 TerrainEditTool::PLUGINABLE_ID = qsf::StringHash("qsf::editor::TerrainEditTool");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainEditTool::TerrainEditTool(qsf::editor::EditModeManager* editModeManager) :
			EditMode(editModeManager)
		{
			QSF_LOG_PRINTS(INFO,"we did sth")
		}


		TerrainEditTool::~TerrainEditTool()
		{
		}

		bool TerrainEditTool::onStartup(EditMode * previousEditMode)
		{
			QSF_LOG_PRINTS(INFO,"yolo we started it!")
			return true;
		}

		void TerrainEditTool::onShutdown(EditMode * nextEditMode)
		{
			QSF_LOG_PRINTS(INFO, "yolo we killed it!")
		}

		bool TerrainEditTool::evaluateBrushPosition(const QPoint & mousePosition, glm::vec3 & position)
		{
		float closestDistance = -1.0f;
		mTerrainComponent = nullptr;

		// Get the camera component the render window is using
		auto renderWindow = &getRenderView().getRenderWindow();
		auto cameraComponent = renderWindow->getCameraComponent();
		if (nullptr != cameraComponent)
		{
			// Get the normalized mouse position
			const glm::vec2 normalizedPosition = renderWindow->getNormalizedWindowSpaceCoords(mousePosition.x(), mousePosition.y());

			// Get a ray at the given viewport position
			const qsf::Ray ray = cameraComponent->getRayAtViewportPosition(normalizedPosition.x, normalizedPosition.y);
			Ogre::Ray ogreRay = qsf::Convert::getOgreRay(ray);

			for (qsf::TerrainComponent* terrainComponent : qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<qsf::TerrainComponent>())
			{
				if (terrainComponent->getTerrainHitBoundingBoxByRay(ogreRay))
				{
					// Get terrain intersection
					glm::vec3 hitPosition;
					if (terrainComponent->getTerrainHitByRay(ogreRay, &hitPosition))
					{
						// Get distance to intersection
						const float distance = glm::length(hitPosition - qsf::Convert::getGlmVec3(ogreRay.getOrigin()));
						if (distance < closestDistance || closestDistance < 0.0f)
						{
							closestDistance = distance;
							position = hitPosition;
							mTerrainComponent = terrainComponent;
						}
					}
				}
			}

			if (closestDistance < 0.0f)
			{
				const qsf::Plane plane = qsf::Plane(position, qsf::Math::GLM_VEC3_UNIT_Y);
				qsf::Math::intersectRayWithPlane(ray, plane, &position, nullptr);
				closestDistance = 0.0f;
			}
		}

		if (closestDistance < 0.0f)
		{
			mTerrainComponent = nullptr;
		}
		//skip this!
		if (nullptr != mTerrainComponent)
		{
			/*mTerrainEditHelper = QSF_EDITOR_TERRAINEDIT.findTerrainEditHelper(mTerrainComponent->getEntityId());
			if (nullptr == mTerrainEditHelper || !mTerrainEditHelper->isGood() || !mTerrainEditHelper->isReady())
			{
				mTerrainComponent = nullptr;
			}*/
		}
		else
		{
			// Do not invalidate the terrain edit helper instance in here, we might be in the middle of feeding an operation
			// mTerrainEditHelper = nullptr;
		}

		return (nullptr != mTerrainComponent);
	}



		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		/*void TerrainEditTool::retranslateUi()
		{
			// Retranslate the content of the UI, this automatically clears the previous content
			mUiTerrainEditTool->retranslateUi(this);

		/*	QComboBox* qComboBoxViewMode = mUiTerrainEditTool->comboBoxType;
			qComboBoxViewMode->clear();
			qComboBoxViewMode->addItem(tr("ID_TerrainEditTool_ALL"));
			qComboBoxViewMode->addItem(tr("ID_TerrainEditTool_WHITE"));
			qComboBoxViewMode->addItem(tr("ID_TerrainEditTool_RED"));
			qComboBoxViewMode->addItem(tr("ID_TerrainEditTool_YELLOW"));
			qComboBoxViewMode->addItem(tr("ID_TerrainEditTool_GREEN"));
			qComboBoxViewMode->addItem(tr("ID_TerrainEditTool_BLUE"));
			qComboBoxViewMode->setCurrentIndex(0);*/
			/*mUiTerrainEditTool->pushButtonSelect->setText("Place Units");
			this->setWindowTitle("Unit Placer");
			
		}

		void TerrainEditTool::changeVisibility(bool visible)
		{
			// Lazy evaluation: If the view is shown the first time, create its content
			if (visible && nullptr == mUiTerrainEditTool)
			{
				// Setup the view content
				QWidget* contentWidget = new QWidget(this);
				{
					// Load content to widget
					mUiTerrainEditTool = new Ui::TerrainEditTool();
					mUiTerrainEditTool->setupUi(contentWidget);
				}

				// Set content to view
				setWidget(contentWidget);

				// Connect Qt signals/slots

				connect(mUiTerrainEditTool->pushButtonSelect, SIGNAL(clicked(bool)), this, SLOT(onPushSelectButton(bool)));
				connect(mUiTerrainEditTool->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(onMinimumSliderChanged(int)));
				connect(mUiTerrainEditTool->horizontalSlider_2, SIGNAL(valueChanged(int)), this, SLOT(onMaximumSliderChanged(int)));
				
				connect(mUiTerrainEditTool->lineEdit, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
				connect(mUiTerrainEditTool->lineEdit_2, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));
				connect(mUiTerrainEditTool->lineEdit_3, SIGNAL(textChanged(QString)), this, SLOT(onEditPrefab(QString)));

				connect(mUiTerrainEditTool->horizontalSlider_3, SIGNAL(valueChanged(int)), this, SLOT(onPrefab_1SliderChanged(int)));
				connect(mUiTerrainEditTool->horizontalSlider_4, SIGNAL(valueChanged(int)), this, SLOT(onPrefab_2SliderChanged(int)));
				connect(mUiTerrainEditTool->horizontalSlider_5, SIGNAL(valueChanged(int)), this, SLOT(onPrefab_3SliderChanged(int)));


					MarkNotGuiltyPrefabs();
			}
		}*/


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void TerrainEditTool::rebuildGui()
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
			mUiTerrainEditTool->spinBoxCounter->setValue(count);
			
		}

		void TerrainEditTool::componentCreated()
		{
			return;
			// Increase the counter in spinbox
			QSpinBox* spinBox = mUiTerrainEditTool->spinBoxCounter;
			int oldValue = spinBox->value();
			spinBox->setValue(++oldValue);
		}

		void TerrainEditTool::componentDestroyed()
		{
			return;
			// Decrease the counter in spinbox
			QSpinBox* spinBox = mUiTerrainEditTool->spinBoxCounter;
			int oldValue = spinBox->value();
			spinBox->setValue(--oldValue);
		}

		void TerrainEditTool::updateComponentData(uint64 entityId)
		{
			// TODO(user) fill with live
		}


		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]
		/*void TerrainEditTool::showEvent(QShowEvent* qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);

			// Perform a GUI rebuild to ensure the GUI content is up-to-date
			rebuildGui();

			// Connect Qt signals/slots
			connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &TerrainEditTool::onUndoOperationExecuted);
			connect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &TerrainEditTool::onRedoOperationExecuted);
		}

		void TerrainEditTool::hideEvent(QHideEvent* qHideEvent)
		{
			// Call the base implementation
			View::hideEvent(qHideEvent);

			// Disconnect Qt signals/slots
			disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &TerrainEditTool::onUndoOperationExecuted);
			disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &TerrainEditTool::onRedoOperationExecuted);

			PaintJobProxy.unregister();
			mDebugDrawProxy.unregister();
		}*/

		void TerrainEditTool::PaintJob(const qsf::JobArguments & jobArguments)
		{
			if (QSF_INPUT.getKeyboard().Escape.isPressed())
			{
				IsActive = false;
				mUiTerrainEditTool->pushButtonSelect->setText("Place Units");
				auto a = QSF_EDITOR_SELECTION_SYSTEM.get<qsf::editor::EntitySelectionManager>();
				a->setSelectionByIdSet(CreatedUnits);
				CreatedUnits.clear();
				PaintJobProxy.unregister();
				mDebugDrawProxy.unregister();
				return;
			}
			mDebugDrawProxy.registerAt(QSF_MAINMAP.getDebugDrawManager());
			glm::vec3 position = getPositionUnderMouse();
			float Radius = 1.f* mUiTerrainEditTool->spinBoxCounter->value()*0.2f;
			mDebugDrawProxy.addRequest(qsf::CircleDebugDrawRequest(position, qsf::CoordinateSystem::getUp(), Radius, qsf::Color4::GREEN));

			MoveDelta += glm::distance(OldPos, position);
			OldPos = position;

			if (QSF_INPUT.getMouse().Left.isPressed())
			{
				if (MoveDelta > mUiTerrainEditTool->doubleSpinBox->value())
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
		void TerrainEditTool::onPushSelectButton(const bool pressed)
		{
			IsActive = !IsActive;
			if (IsActive)
			{
				mUiTerrainEditTool->pushButtonSelect->setText("Stop Placing Units");
				CreatedUnits.clear();
				PaintJobProxy.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&TerrainEditTool::PaintJob, this, _1));
			}
			else
			{
				mUiTerrainEditTool->pushButtonSelect->setText("Place Units");
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

		void TerrainEditTool::onCurrentIndexChanged(int index)
		{
			//QString displayText(mUiTerrainEditTool->comboBoxType->itemText(index).toUtf8().constData());
			//mUiTerrainEditTool->lineEditSelected->setText(displayText);
		}

		void TerrainEditTool::onUndoOperationExecuted(const qsf::editor::base::Operation& operation)
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

		void TerrainEditTool::onRedoOperationExecuted(const qsf::editor::base::Operation& operation)
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

		glm::vec3 TerrainEditTool::getPositionUnderMouse()
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

		void TerrainEditTool::CreateUnit(glm::vec3 position)
		{
			float Radius = 1.f* mUiTerrainEditTool->spinBoxCounter->value()*0.2f;

			//which prefabs are active=
			bool Prefab1 = mUiTerrainEditTool->lineEdit_6->isEnabled();
			bool Prefab2 = mUiTerrainEditTool->lineEdit_7->isEnabled();
			bool Prefab3 = mUiTerrainEditTool->lineEdit_8->isEnabled();
			int num = 0;
			//what are the weights
			if (Prefab1)
				num = mUiTerrainEditTool->horizontalSlider_3->value();
			if (Prefab2)
				num += mUiTerrainEditTool->horizontalSlider_4->value();
			if (Prefab3)
				num += mUiTerrainEditTool->horizontalSlider_5->value();
		
			qsf::StringHash  shash = 0;
			//get a unit based on the weight!
			num = qsf::Random::getRandomInt(0, num);
			//QSF_LOG_PRINTS(INFO,num)
			if (Prefab1)
			{
				num -= mUiTerrainEditTool->horizontalSlider_3->value();
			}
			if (num <= 0)
			{
				shash = qsf::StringHash(mUiTerrainEditTool->lineEdit->text().toStdString());
			}
			else
			{
				if (Prefab2)
				{
					num -= mUiTerrainEditTool->horizontalSlider_4->value();
				}
				if (num <= 0)
				{
					shash = qsf::StringHash(mUiTerrainEditTool->lineEdit_2->text().toStdString());
				}
				else
				{
					shash = qsf::StringHash(mUiTerrainEditTool->lineEdit_3->text().toStdString());
				}
			}

			float groundHeight = 0.f;
			qsf::GroundMapQuery(QSF_MAINMAP, em5::GroundMaps::FILTER_DEFAULT).getHeightAt(position, groundHeight);
			position.y = groundHeight;
			float ValueR = qsf::Random::getRandomFloat(0.f, mUiTerrainEditTool->spinBoxCounter->value()*0.2f);
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
			float ScaleX = qsf::Random::getRandomFloat(mUiTerrainEditTool->lineEdit_4->text().toDouble(), mUiTerrainEditTool->lineEdit_5->text().toDouble());
			float ScaleY = qsf::Random::getRandomFloat(mUiTerrainEditTool->lineEdit_4->text().toDouble(), mUiTerrainEditTool->lineEdit_5->text().toDouble());
			float ScaleZ = qsf::Random::getRandomFloat(mUiTerrainEditTool->lineEdit_4->text().toDouble(), mUiTerrainEditTool->lineEdit_5->text().toDouble());
			entity->getOrCreateComponent<qsf::TransformComponent>()->setPosition(position + glm::vec3(OffsetX, 0, OffsetY));
			if(mUiTerrainEditTool->checkBox->isChecked())
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

		void TerrainEditTool::onMinimumSliderChanged(const int value)
		{
			std::stringstream ss;
			ss << 1.0*mUiTerrainEditTool->horizontalSlider->value() / 10.0;
			mUiTerrainEditTool->lineEdit_4->setText(ss.str().c_str());
		}

		void TerrainEditTool::onMaximumSliderChanged(const int value)
		{
			std::stringstream ss;
			ss << 1.0*mUiTerrainEditTool->horizontalSlider_2->value() / 10.0;
			mUiTerrainEditTool->lineEdit_5->setText(ss.str().c_str());
		}

		void TerrainEditTool::onEditPrefab(const QString & text)
		{
			//QSF_LOG_PRINTS(INFO, "Signal recieved");
			MarkNotGuiltyPrefabs();
		}

		void TerrainEditTool::onPrefab_1SliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUiTerrainEditTool->horizontalSlider_3->value();
			mUiTerrainEditTool->lineEdit_6->setText(ss.str().c_str());
		}

		void TerrainEditTool::onPrefab_2SliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUiTerrainEditTool->horizontalSlider_4->value();
			mUiTerrainEditTool->lineEdit_7->setText(ss.str().c_str());
		}

		void TerrainEditTool::onPrefab_3SliderChanged(const int value)
		{
			std::stringstream ss;
			ss << mUiTerrainEditTool->horizontalSlider_5->value();
			mUiTerrainEditTool->lineEdit_8->setText(ss.str().c_str());
		}

		void TerrainEditTool::buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select)
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

		uint64 TerrainEditTool::BuildEntity(glm::vec3 position, qsf::StringHash shash)
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

		void TerrainEditTool::MarkNotGuiltyPrefabs()
		{
			if (qsf::AssetProxy(mUiTerrainEditTool->lineEdit->text().toStdString()).getAsset() != nullptr)
			{
				mUiTerrainEditTool->lineEdit->setAutoFillBackground(false);
				mUiTerrainEditTool->horizontalSlider_3->setEnabled(true);
				mUiTerrainEditTool->lineEdit_6->setEnabled(true);
			}
			else
			{
				mUiTerrainEditTool->lineEdit->setAutoFillBackground(true);
				mUiTerrainEditTool->horizontalSlider_3->setEnabled(false);
				mUiTerrainEditTool->lineEdit_6->setEnabled(false);
			}

			if (qsf::AssetProxy(mUiTerrainEditTool->lineEdit_2->text().toStdString()).getAsset() != nullptr)
			{
				mUiTerrainEditTool->lineEdit_2->setAutoFillBackground(false);
				mUiTerrainEditTool->horizontalSlider_4->setEnabled(true);
				mUiTerrainEditTool->lineEdit_7->setEnabled(true);
			}
			else
			{
				mUiTerrainEditTool->lineEdit_2->setAutoFillBackground(true);
				mUiTerrainEditTool->horizontalSlider_4->setEnabled(false);
				mUiTerrainEditTool->lineEdit_7->setEnabled(false);
			}

			if (qsf::AssetProxy(mUiTerrainEditTool->lineEdit_3->text().toStdString()).getAsset() != nullptr)
			{
				mUiTerrainEditTool->lineEdit_3->setAutoFillBackground(false);
				mUiTerrainEditTool->horizontalSlider_5->setEnabled(true);
				mUiTerrainEditTool->lineEdit_8->setEnabled(true);
			}
			else
			{
				mUiTerrainEditTool->lineEdit_3->setAutoFillBackground(true);
				mUiTerrainEditTool->horizontalSlider_5->setEnabled(false);
				mUiTerrainEditTool->lineEdit_8->setEnabled(false);
			}
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // user
