// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/EditorTerrainManager.h"
#include "ui_EditorTerrainManager.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)


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
#include <qsf/prototype/Prototype.h>
#include <qsf\map\Entity.h>
#include <qsf\component\base\MetadataComponent.h>
#include <qsf\file\FileStream.h>
#include <qsf\file\FileHelper.h>

#include <qsf/asset/AssetSystem.h>
#include <qsf/asset/type/AssetType.h>
#include < qsf/asset/type/AssetTypeManager.h>
#include <boost\foreach.hpp>
#include <algorithm>
#include <QtWidgets\qmenu.h>
#include <QtGui\qclipboard.h>
#include <QtWidgets\qfiledialog.h>
#include <QtCore\qstandardpaths.h>
#include <fstream>

#include <ogre\Ogre.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <assert.h>
#include <time.h>

#include <filesystem>
#include <iostream>     // std::cout, std::endl
#include <iomanip>      // std::setfill, std::setw

#include <asset_collector_tool\extern\include\Magick++.h>
#include <string>
#include <iostream>
#include <list>
#include <algorithm>
#include <qsf/plugin/PluginSystem.h>

#include <qsf/renderer/RendererSystem.h>
#include <qsf_editor/application/Application.h>
#include <qsf_editor/application/manager/CameraManager.h>
#include <qsf_compositing/plugin/Plugin.h>
#include <qsf/window/Window.h>
#include <qsf/window/WindowSystem.h>
#include <qsf/renderer/component/CameraComponent.h>
#include <qsf/renderer/window/RenderWindow.h>
#include <em5\component\vehicle\RoadVehicleComponent.h>
#include <qsf/math/EulerAngles.h>
#include <qsf/component/base/TransformComponent.h>
#include <qsf/map/query/ComponentMapQuery.h>
#include <OGRE/OgreRay.h>
#include <qsf/math/Convert.h>
#include <qsf/map/query/RayMapQuery.h>
#include <qsf_editor/renderer/RenderView.h>
#include <em5/map/EntityHelper.h>
#include <qsf_editor/base/GuiHelper.h>
#include <QtGui\qdesktopservices.h>
#include "qsf/renderer/material/MaterialSystem.h"
#include <qsf/renderer/material/material/MaterialVariationManager.h>
#include <qsf_editor/editmode/map/PrefabInstancingEditMode.h>
#include <qsf/input/InputSystem.h>
#include <qsf/input/device/MouseDevice.h>
#include <qsf/map/query/ComponentMapQuery.h>
#include <qsf/map/Entity.h>
#include <qsf/renderer/helper/RendererHelper.h>
#include <qsf/renderer/utility/CameraControlComponent.h>
#include <qsf/math/Plane.h>
#include <qsf/math/Math.h>
#include <em5/plugin/Jobs.h>
#include <asset_collector_tool\editmode\PlaceUnitEditMode.h>
#include <qsf/input/device/KeyboardDevice.h>

using namespace std;

using namespace Magick;


#undef ERROR
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace kc_terrain
{

	using boost::lexical_cast;
	using boost::bad_lexical_cast;
	//[-------------------------------------------------------]
	//[ Public definitions                                    ]
	//[-------------------------------------------------------]
	const uint32 EditorTerrainManager::PLUGINABLE_ID = qsf::StringHash("user::editor::EditorTerrainManager");


	//[-------------------------------------------------------]
	//[ Public methods                                        ]
	//[-------------------------------------------------------]
	EditorTerrainManager::EditorTerrainManager(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
		qsf::editor::View(viewManager, qWidgetParent),
		//View(viewManager, qWidgetParent),
		mUiEditorTerrainManager(nullptr),
		mPlaceUnitEditMode(nullptr)
	{
		// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
		addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);
		mWaitUntilIsInEditMode.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&EditorTerrainManager::WaitUntilIsInEditMode, this, _1));
	}

	EditorTerrainManager::~EditorTerrainManager()
	{
		mWaitUntilIsInEditMode.unregister();
		// Destroy the UI view instance
		if (nullptr != mUiEditorTerrainManager)
		{
			qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
			entitySelectionManager.Selected.disconnect(boost::bind(&EditorTerrainManager::onSelectionChanged, this, _1));
			delete mUiEditorTerrainManager;
		}
	}


	//[-------------------------------------------------------]
	//[ Protected virtual qsf::editor::View methods           ]
	//[-------------------------------------------------------]
	void EditorTerrainManager::retranslateUi()
	{
		// Retranslate the content of the UI, this automatically clears the previous content
		mUiEditorTerrainManager->retranslateUi(this);
	}

	void EditorTerrainManager::changeVisibility(bool visible)
	{
		// Lazy evaluation: If the view is shown the first time, create its content
		if (visible && nullptr == mUiEditorTerrainManager)
		{
			// Setup the view content
			QWidget* contentWidget = new QWidget(this);
			{

				// Load content to widget
				mUiEditorTerrainManager = new Ui::EditorTerrainManager();
				mUiEditorTerrainManager->setupUi(contentWidget);
			}

			// Set content to view
			setWidget(contentWidget);
			// Connect Qt signals/slots
			connect(mUiEditorTerrainManager->stick_to_gnd, SIGNAL(clicked(bool)), this, SLOT(onPressStickToGnd(bool)));
			/*connect(mUiEditorTerrainManager->decompilebutton, SIGNAL(clicked(bool)), this, SLOT(onPushDecompileButton(bool)));
			connect(mUiEditorTerrainManager->SetSaveLocationButton, SIGNAL(clicked(bool)), this, SLOT(onSetSaveDirectory(bool)));
			connect(mUiEditorTerrainManager->SetSelectionGreenscreen, SIGNAL(clicked(bool)), this, SLOT(onSetSelectionGreenscreen(bool)));

			connect(mUiEditorTerrainManager->cam1, SIGNAL(clicked(bool)), this, SLOT(onCam1(bool)));
			connect(mUiEditorTerrainManager->cam2, SIGNAL(clicked(bool)), this, SLOT(onCam2(bool)));
			connect(mUiEditorTerrainManager->origcam, SIGNAL(clicked(bool)), this, SLOT(onOrigCam(bool)));
			connect(mUiEditorTerrainManager->removeEntity, SIGNAL(clicked(bool)), this, SLOT(onremoveEntity(bool)));
			connect(mUiEditorTerrainManager->OpenSaveLocation, SIGNAL(clicked(bool)), this, SLOT(onOpenSaveLocation(bool)));*/
			qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
			auto con2 = &entitySelectionManager.Selected.connect(boost::bind(&EditorTerrainManager::onSelectionChanged, this, _1), boost::signals2::at_back);
			if (con2 == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "entitySelectionManager :: Slot connection failed 2 ")
			}
		}
		else if (!visible && nullptr == mUiEditorTerrainManager)
		{
		}

	}

	void EditorTerrainManager::showEvent(QShowEvent * qShowEvent)
	{
		// Call the base implementation
		View::showEvent(qShowEvent);

		// Perform a GUI rebuild to ensure the GUI content is up-to-date
		rebuildGui();
	}

	void EditorTerrainManager::hideEvent(QHideEvent * qHideEvent)
	{
		// Call the base implementation
		View::hideEvent(qHideEvent);
	}

	void EditorTerrainManager::onPressStickToGnd(const bool pressed)
	{
		mUiEditorTerrainManager->stick_to_gnd->setChecked(pressed);
		QSF_LOG_PRINTS(INFO, "is active " << pressed)
			ActivateStickToGnd(!pressed);
		if (pressed)
			mUiEditorTerrainManager->stick_to_gnd->setStyleSheet("border:5px solid #ffff00;");
		else
			mUiEditorTerrainManager->stick_to_gnd->setStyleSheet("border:5px solid #646464;");

	}

	//[-------------------------------------------------------]
	//[ Private methods                                       ]
	//[-------------------------------------------------------]
	void EditorTerrainManager::rebuildGui()
	{
	}

	void EditorTerrainManager::ActivateStickToGnd(bool active)
	{
	}

	//we could also just store a list of old entites
	void EditorTerrainManager::onSelectionChanged(uint64 Id)
	{
		if (Id == qsf::getUninitialized<uint64>())
			return;
		auto sel = QSF_EDITOR_EDITMODE_MANAGER.getSelectedEditMode();
		if (sel == nullptr)
			return;
		if (!mUiEditorTerrainManager->stick_to_gnd->isChecked())
			return;
		auto PIB = static_cast<qsf::editor::PrefabInstancingEditMode*>(sel);
		if (PIB == nullptr)
			return;
		QSF_LOG_PRINTS(INFO, "we selected a new unit (at least we guess so)")
			if (PushObjectOnTopmostTerrain(Id))
				QSF_LOG_PRINTS(INFO, "new pos")
	}

	bool EditorTerrainManager::PushObjectOnTopmostTerrain(uint64 id)
	{
		float closestDistance = -1.0f;
		qsf::TerrainComponent* mTerrainComponent = nullptr;

		// Get the camera component the render window is using
		auto renderWindow = &QSF_EDITOR_EDITMODE_MANAGER.getSelectedEditMode()->getRenderView().getRenderWindow();
		glm::vec2 mousePosition = QSF_INPUT.getMouse().getPosition();
		glm::vec3 position;
		//auto renderWindow = &getRenderView().getRenderWindow();
		auto cameraComponent = renderWindow->getCameraComponent();
		if (nullptr != cameraComponent)
		{
			// Get the normalized mouse position
			const glm::vec2 normalizedPosition = renderWindow->getNormalizedWindowSpaceCoords(mousePosition.x, mousePosition.y);

			// Get a ray at the given viewport position
			const qsf::Ray ray = cameraComponent->getRayAtViewportPosition(normalizedPosition.x, normalizedPosition.y);
			Ogre::Ray ogreRay = qsf::Convert::getOgreRay(ray);
			for (qsf::TerrainComponent* terrainComponent : qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<qsf::TerrainComponent>())
			{
				QSF_LOG_PRINTS(INFO,"2is kc terrain")
					kc_terrain::TerrainComponent* kc_terraincomp = terrainComponent->getEntity().getComponent<kc_terrain::TerrainComponent>();
				if (kc_terraincomp == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "no it is no kc terrain")
					if (terrainComponent->getTerrainHitBoundingBoxByRay(ogreRay))
					{
						// Get terrain intersection
						glm::vec3 hitPosition;

						if (terrainComponent->getTerrainHitByRay(ogreRay, &hitPosition))
						{
							// Get distance to intersection
							const float distance = glm::length(hitPosition - qsf::Convert::getGlmVec3(ogreRay.getOrigin()));
							QSF_LOG_PRINTS(INFO, terrainComponent->getEntity().getComponent<qsf::MetadataComponent>()->getName() << " " << distance)
								if (distance < closestDistance || closestDistance < 0.0f)
								{
									closestDistance = distance;
									position = hitPosition;
									mTerrainComponent = terrainComponent;

								}
						}
				}
				else
				{
					QSF_LOG_PRINTS(INFO, "no hit " << terrainComponent->getEntity().getComponent<qsf::MetadataComponent>()->getName())
				}
			}
				else
				{
					QSF_LOG_PRINTS(INFO, "yes it is kc terrain")
					if (kc_terraincomp->getTerrainHitBoundingBoxByRay(ogreRay))
					{
						// Get terrain intersection
						glm::vec3 hitPosition;

						if (kc_terraincomp->getTerrainHitByRay(ogreRay, &hitPosition))
						{
							// Get distance to intersection
							const float distance = glm::length(hitPosition - qsf::Convert::getGlmVec3(ogreRay.getOrigin()));
							QSF_LOG_PRINTS(INFO, kc_terraincomp->getEntity().getComponent<qsf::MetadataComponent>()->getName() << " " << distance)
								if (distance < closestDistance || closestDistance < 0.0f)
								{
									closestDistance = distance;
									position = hitPosition;
									mTerrainComponent = terrainComponent;

								}
						}
					}
					else
					{
						QSF_LOG_PRINTS(INFO, "no hit " << terrainComponent->getEntity().getComponent<qsf::MetadataComponent>()->getName())
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
		if (nullptr == mTerrainComponent)
			return false;
		qsf::Entity* ent = QSF_MAINMAP.getEntityById(id);
		if (ent == nullptr)
			return false;
		if (ent->getComponent<qsf::TransformComponent>() == nullptr)
			return false;
		ent->getComponent<qsf::TransformComponent>()->setPosition(position);
		QSF_LOG_PRINTS(INFO, "Finalposition " << position)
			//return glm::vec3(pos.getPoint(t));
			return true;
	}

	void EditorTerrainManager::WaitUntilIsInEditMode(const qsf::JobArguments & jobArguments)
	{
		auto sel = QSF_EDITOR_EDITMODE_MANAGER.getSelectedEditMode();
		if (sel == nullptr)
		{
			ReplaceEditMode(false);
			//QSF_LOG_PRINTS(INFO,1)

			return;
		}
		//QSF_LOG_PRINTS(INFO,sel->campClassName())
		/*auto PIB = static_cast<qsf::editor::PrefabInstancingEditMode*>(sel);
		if (PIB == nullptr)
		{
			ReplaceEditMode(false);
			QSF_LOG_PRINTS(INFO, 2)
			return;
		}*/
		if("user::editor::PlaceUnitEditMode" == std::string(sel->campClassName())) //allready in right mode
		{
			return;
		}
		/*
			//QSF_LOG_PRINTS(INFO,QSF_INPUT.getKeyboard().Escape.isPressed())
			if(QSF_INPUT.getKeyboard().Escape.isPressed())
			{
				QSF_LOG_PRINTS(INFO,"go back")
				QSF_EDITOR_EDITMODE_MANAGER.selectEditModeById("qsf::editor::ObjectEditMode");
			}
			return;
		}
		else
		{
			//QSF_LOG_PRINTS(INFO, sel->campClassName())
		}*/
		if (std::string(sel->campClassName()) != "qsf::editor::PrefabInstancingEditMode")
		{
			//QSF_LOG_PRINTS(INFO,"qsf::editor::PrefabInstancingEditMode |" << sel->campClassName() <<"|")
			ReplaceEditMode(false);
			return;
		}
	
			ReplaceEditMode(true);

			auto PIB = static_cast<qsf::editor::PrefabInstancingEditMode*>(sel);
			if(PIB != nullptr && PIB->getOriginalPrototype() != nullptr && PIB->getOriginalPrototype()->getComponent<qsf::MetadataComponent>() != nullptr && &PIB->getOriginalPrototype()->getComponent<qsf::MetadataComponent>()->getBasePrefab() != nullptr)
				//QSF_LOG_PRINTS(INFO,PIB->getOriginalPrototype()->getComponent<qsf::MetadataComponent>()->getName());
			//QSF_LOG_PRINTS(INFO, PIB->getOriginalPrototype()->getId());
			mUiEditorTerrainManager->editmodeindicator->setText(QString(std::string(PIB->getOriginalPrototype()->getComponent<qsf::MetadataComponent>()->getName() + " " +boost::lexical_cast<std::string>(PIB->getOriginalPrototype()->getId())).c_str()));

	}

	void EditorTerrainManager::ReplaceEditMode(bool IsGood)
	{
		QPalette pal = mUiEditorTerrainManager->editmodeindicator->palette();
		mUiEditorTerrainManager->editmodeindicator->setAutoFillBackground(true);
		if(IsGood)
		{	
			auto sel = QSF_EDITOR_EDITMODE_MANAGER.getSelectedEditMode();
			auto PIB = static_cast<qsf::editor::PrefabInstancingEditMode*>(sel);
			uint64 id = static_cast<qsf::PrototypeManager*>(&PIB->getOriginalPrototype()->getPrototypeManager())->getPrefabGlobalAssetIdByPrototypeId(PIB->getOriginalPrototype()->getId());
			mPlaceUnitEditMode = new user::editor::PlaceUnitEditMode(&QSF_EDITOR_EDITMODE_MANAGER);
			QSF_EDITOR_EDITMODE_MANAGER.selectEditModeByPointer(mPlaceUnitEditMode,nullptr);
			mPlaceUnitEditMode->SetPrototypeId(id);
			pal.setColor(mUiEditorTerrainManager->editmodeindicator->backgroundRole(), Qt::green);
		}
		else
		{
			pal.setColor(mUiEditorTerrainManager->editmodeindicator->backgroundRole(), Qt::red);
		}
		mUiEditorTerrainManager->editmodeindicator->setPalette(pal);
	}

} // user
