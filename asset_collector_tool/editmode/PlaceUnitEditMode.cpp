// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/editmode/PlaceUnitEditMode.h"
#include <asset_collector_tool\qsf_editor\tools\TerrainEditToolbox.h>
//#include "ui_PlaceUnitEditMode.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

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
#include <../../plugin_api/external/qt/include/QtGui/qevent.h>

#include <ogre\Terrain\OgreTerrainGroup.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <qsf/input/device/KeyboardDevice.h>
#include <qsf/debug/DebugDrawManager.h>

#include <qsf/file/FileSystem.h>
#include <experimental/filesystem> 
#include <boost\filesystem.hpp>
#include <qsf/plugin/QsfAssetTypes.h>
#include <qsf/asset/project/AssetPackage.h>
#include <qsf_editor/asset/import/AssetImportManager.h>
#include <QtWidgets\qinputdialog.h>
#include <windows.h>
#include <qsf/component/placement/WalkableComponent.h>
#include <qsf/component/street/StreetComponent.h>
#include <qsf/input/event/mouse/MouseButtonEvent.h>
#include <qsf_editor/view/asset/PrefabBrowserView.h>
#include <em5\map\EntityHelper.h>
#include <asset_collector_tool/view/EditorTerrainManager.h>
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
		const uint32 PlaceUnitEditMode::PLUGINABLE_ID = qsf::StringHash("user::editor::PlaceUnitEditMode");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		PlaceUnitEditMode::PlaceUnitEditMode(qsf::editor::EditModeManager* editModeManager) :
			EditMode(editModeManager),
			mPrototypeId(qsf::getUninitialized<uint64>()),
			mEntityId(qsf::getUninitialized<uint64>())
		{
			
		}

		bool PlaceUnitEditMode::eventFilter(QObject* obj, QEvent* event)
		{
			//if (event->type() != 129) //Mouse Move
				//QSF_LOG_PRINTS(INFO, event->type())
				if (event->type() == 51) {
					auto key = static_cast<QKeyEvent*>(event);
					if ((key->key() == Qt::Key_Escape) || (key->key() == Qt::Key_Return)) {
						QSF_EDITOR_EDITMODE_MANAGER.selectEditModeById("qsf::editor::ObjectEditMode");
						//Enter or return was pressed
					}
					else {

						return QObject::eventFilter(obj, event);
					}
					return true;
				}
				else if (event->type() == QEvent::MouseButtonPress) {
					{
						auto key = static_cast<QMouseEvent*>(event);
						if (key->button() == Qt::RightButton)
							QSF_EDITOR_EDITMODE_MANAGER.selectEditModeById("qsf::editor::ObjectEditMode");
					}
					return QObject::eventFilter(obj, event);
				}
				else if (event->type() == QEvent::Leave)
				{
					mState = State::Outside; //no check here if on map
				}
				else if (event->type() == QEvent::Enter)
				{
					mState = State::Inside;
					QSF_EDITOR_APPLICATION.getMainWindow()->getRenderView().setFocus();
				}
			return false;
		}


		PlaceUnitEditMode::~PlaceUnitEditMode()
		{
			
		}

		uint64 PlaceUnitEditMode::GetPrototypeId()
		{
			return mPrototypeId;
		}

		void PlaceUnitEditMode::SetPrototypeId(uint64 PrototypeId)
		{
			 mPrototypeId = PrototypeId;
			// QSF_LOG_PRINTS(INFO, "start new mode with prototype id "<< PrototypeId)
		}


		bool PlaceUnitEditMode::evaluateBrushPosition(const QPoint & mousePosition, glm::vec3 & position)
		{
			return true;
		}






		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]


		void PlaceUnitEditMode::PaintJob(const qsf::JobArguments & jobArguments)
		{
			uint64 MyEntity = GetOrCreateEntity();
			if (mState == State::Outside)
			{
				//hide entity
				//if(MyEntity != qsf::getUninitialized<uint64>())
				//em5::EntityHelper(QSF_MAINMAP.getEntityById(MyEntity)).fadeOut(qsf::Time::fromSeconds(0.f));
				return;
			}
			if (mState == State::Inside)
			{
				//show
				//if (MyEntity != qsf::getUninitialized<uint64>())
				//em5::EntityHelper(QSF_MAINMAP.getEntityById(MyEntity)).fadeIn(qsf::Time::fromSeconds(0.f));
			}
			if(MyEntity == qsf::getUninitialized<uint64>()) //not yet created?
				return;
			auto NewPos = glm::vec3(0,0,0);
			NewPos = getPositionUnderMouse();
			/*if (QSF_INPUT.getMouse().Right.isPressed())
			{*/
				//NewPos = getPositionUnderMouse(LastMousePos);
			/*}
			else
			{
				NewPos = getPositionUnderMouse();
				LastMousePos = QSF_INPUT.getMouse().getPosition();
			}*/
			//0,0,0 is no update possible
			if (glm::vec3(0, 0, 0) == NewPos)
			{
				mState = State::Not_on_Map;
			}
			else
			{
				mState = State::Visible;
				QSF_MAINMAP.getEntityById(MyEntity)->getOrCreateComponent<qsf::TransformComponent>()->setPosition(NewPos);
			}
			if (QSF_INPUT.getMouse().Left.isPressed() && !mLastButtonState)
			{
			if(State::Visible == mState)
			{
				//Place Unit on Map
				auto Unit = user::GeneralFunctions::BuildEntity(NewPos, qsf::AssetProxy(mPrototypeId).getLocalAssetId());
				//if(QSF_MAINMAP.getEntityById(Unit) != nullptr)
				qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
				compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(Unit, qsf::TransformComponent::COMPONENT_ID, qsf::TransformComponent::POSITION, NewPos));
				QSF_EDITOR_OPERATION.push(compoundOperation2);
			}
			}
			mLastButtonState = QSF_INPUT.getMouse().Left.isPressed(); //but jus once per press

		}

		
	





		glm::vec3 PlaceUnitEditMode::getPositionUnderMouse()
		{
		//Does work
			getRenderView().getRenderWindow().getCameraComponent();
			auto IgnoreList = boost::container::flat_set<uint64>{mIgnoreYourself};
			int counter = 0;
			while (true)
			{
			counter++;
			glm::vec2 mousePosition = QSF_INPUT.getMouse().getPosition();
			qsf::RayMapQueryResponse response = qsf::RayMapQueryResponse(qsf::RayMapQueryResponse::POSITION_RESPONSE);
			qsf::RayMapQuery(QSF_MAINMAP).getFirstHitByRenderWindow(getRenderView().getRenderWindow(), mousePosition.x, mousePosition.y, response,&IgnoreList);
			if(kc_terrain::EditorTerrainManager::GetInstance() != nullptr)
			{
				SetText(response.component);
			}
			else
			{
				return glm::vec3(0, 0, 0);
			}
			if(response.component == nullptr)
			{
				auto TerrainMaster = qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<kc_terrain::TerrainComponent>();
				if(TerrainMaster != nullptr)
				{
					auto vec = glm::vec3();
					if(TerrainMaster->getTerrainHitByRenderWindow(getRenderView().getRenderWindow(), mousePosition.x, mousePosition.y,&vec))
					{
						kc_terrain::EditorTerrainManager::GetInstance()->setLabelName("Hit kc Terrain");
						return vec;
					}
				}
				return glm::vec3(0, 0, 0);
			}
#ifdef NoQSFTerrain
			if (response.component->getEntity().getComponent<kc_terrain::TerrainComponent>() != nullptr) //our own terrain component
			{
				return glm::vec3(response.position);
			}
#endif
			//all others
			if(response.component->getEntity().getComponent<qsf::TerrainComponent>() != nullptr || response.component->getEntity().getComponent<qsf::WalkableComponent>() != nullptr || response.component->getEntity().getComponent<qsf::StreetComponent>())
			{
				//QSF_DEBUGDRAW.requestDraw(qsf::CircleDebugDrawRequest(response.position,glm::vec3(0.f,1.f,0.f),1.f),qsf::DebugDrawLifetimeData(qsf::Time::fromMilliseconds(100.f)));
				//QSF_LOG_PRINTS(INFO," point "<< response.position  << "counter " << counter);
				return glm::vec3(response.position);
			}
				//we hit an entity which is no terrain and not walkable ->ignore and search again
			IgnoreList.insert(response.component->getEntityId());
			}
			return glm::vec3(0, 0, 0);
		}

		glm::vec3 PlaceUnitEditMode::getPositionUnderMouse(glm::vec2 Pos)
		{
			getRenderView().getRenderWindow().getCameraComponent();
			auto IgnoreList = boost::container::flat_set<uint64>{ mIgnoreYourself };
			int counter = 0;
			while (true)
			{
				counter++;
				glm::vec2 mousePosition = Pos;
				qsf::RayMapQueryResponse response = qsf::RayMapQueryResponse(qsf::RayMapQueryResponse::POSITION_RESPONSE);
				qsf::RayMapQuery(QSF_MAINMAP).getFirstHitByRenderWindow(getRenderView().getRenderWindow(), mousePosition.x, mousePosition.y, response, &IgnoreList);

				if (response.component == nullptr)
					return glm::vec3(0, 0, 0);
				if (response.component->getEntity().getComponent<qsf::TerrainComponent>() != nullptr || response.component->getEntity().getComponent<qsf::WalkableComponent>() != nullptr || response.component->getEntity().getComponent<qsf::StreetComponent>())
				{
					//QSF_DEBUGDRAW.requestDraw(qsf::CircleDebugDrawRequest(response.position,glm::vec3(0.f,1.f,0.f),1.f),qsf::DebugDrawLifetimeData(qsf::Time::fromMilliseconds(100.f)));
					//QSF_LOG_PRINTS(INFO," point "<< response.position  << "counter " << counter);
					return glm::vec3(response.position);
				}
				//we hit an entity which is no terrain and not walkable ->ignore and search again
				IgnoreList.insert(response.component->getEntityId());
			}
			return glm::vec3(0, 0, 0);
		}





		

		


		inline void PlaceUnitEditMode::mouseMoveEvent(QMouseEvent & qMouseEvent)
		{
			//QSF_LOG_PRINTS(INFO,"moved mouse")
		}

		/*inline void PlaceUnitEditMode::keyPressEvent(QKeyEvent & qEvent)
		{
			QSF_LOG_PRINTS(INFO,qEvent.key())
			if (qEvent.key() == Qt::Key::Key_Escape)
			{
				QSF_EDITOR_EDITMODE_MANAGER.selectEditModeById("qsf::editor::ObjectEditMode");
			}
		}*/

		




		bool PlaceUnitEditMode::onStartup(EditMode * previousEditMode)
		{

			
			if (!PaintJobProxy.isValid())
				PaintJobProxy.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&PlaceUnitEditMode::PaintJob, this, _1));
			//PaintJobProxy.changeTimeBetweenCalls(qsf::Time::fromMilliseconds(50.f));
				//PaintJobProxy.changeTimeBetweenCalls(qsf::Time::fromMilliseconds(100.f));
				this->getRenderView().parentWidget()->installEventFilter(this);
			return true;
		}

		void PlaceUnitEditMode::onShutdown(EditMode * nextEditMode)
		{
			//QSF_LOG_PRINTS(INFO, "Shutdown")
				PaintJobProxy.unregister();
			this->getRenderView().parentWidget()->removeEventFilter(this);

			QSF_MAINMAP.destroyObjectById(mEntityId);
			//better call Operation
			//collect all childs
			/*std::vector<qsf::Entity*> entList, ChildList;
			ChildList.push_back(QSF_MAINMAP.getEntityById(mEntityId));
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
			qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
			QSF_EDITOR_OPERATION.push(compoundOperation2);
			for(auto a : entList)
			{
				compoundOperation2->pushBackOperation(new qsf::editor::base::DestroyEntityOperation(a->getId()));
			}
			QSF_EDITOR_OPERATION.push(compoundOperation2);*/

			
		}

		uint64 PlaceUnitEditMode::GetOrCreateEntity()
		{
			if(QSF_MAINMAP.getEntityById(mEntityId) != nullptr)
			return mEntityId;
			auto ent = QSF_MAINMAP.createObjectByLocalPrefabAssetId(qsf::AssetProxy(mPrototypeId).getLocalAssetId());
			mEntityId = ent->getId();
			 //user::GeneralFunctions::BuildEntity(glm::vec3(0,0,0), qsf::AssetProxy(mPrototypeId).getLocalAssetId());
			//Ignore when using Raycast , notice we only have direct childs here. If we have a vehicle with huge amount of equipment it could slow down?
			mIgnoreYourself.clear();
			mIgnoreYourself.insert(mEntityId);
			if(QSF_MAINMAP.getEntityById(mEntityId)->getComponent<qsf::LinkComponent>() != nullptr)
			{
			auto LC = QSF_MAINMAP.getEntityById(mEntityId)->getComponent<qsf::LinkComponent>();
			for(auto a : LC->getChildLinks())
				mIgnoreYourself.insert(a->getEntityId());
			}
			return mEntityId;
		}

		void PlaceUnitEditMode::SetText(qsf::Component* whatDidWeHit)
		{
			std::string Hitted;
			if (whatDidWeHit == nullptr)
			{
				Hitted = "Nothing";
			}
			else if (whatDidWeHit->getEntity().getComponent<kc_terrain::TerrainComponent>() != nullptr)
			{
				Hitted = "KC_Terrain";
				if (whatDidWeHit->getEntity().getComponent<qsf::TerrainComponent>() == nullptr)
				{
					Hitted = "KC_Terrain but not QSF";
				}
			}
			else if (whatDidWeHit->getEntity().getComponent<qsf::WalkableComponent>() != nullptr)
			{
				Hitted = "Walkable "+ whatDidWeHit->getEntity().getComponent<qsf::MetadataComponent>()->getName();
			}
			else if (whatDidWeHit->getEntity().getComponent<qsf::StreetComponent>() != nullptr)
			{
				Hitted = "StreetComponent " + whatDidWeHit->getEntity().getComponent<qsf::MetadataComponent>()->getName();
			}
			else if (whatDidWeHit->getEntity().getComponent<qsf::TerrainComponent>() != nullptr)
			{
				Hitted = "qsf::Terrain";
			}
			kc_terrain::EditorTerrainManager::GetInstance()->setLabelName(Hitted);
		}

		bool PlaceUnitEditMode::CheckNewTerrain()
		{
			return false;
		}



		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	}

	void GeneralFunctions::buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select)
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

	uint64 GeneralFunctions::BuildEntity(glm::vec3 position, qsf::StringHash shash)
	{
		QSF_LOG_PRINTS(INFO,"Build Ent 1")
		//Create Unit <-> this is quite long
		qsf::Prototype* prototype = QSF_MAINPROTOTYPE.getPrefabByLocalAssetId(shash);
		if (prototype == nullptr)
		{
			return qsf::getUninitialized<uint64>();
		}
		QSF_LOG_PRINTS(INFO, "Build Ent 2")
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
		QSF_LOG_PRINTS(INFO, "Build Ent 3")
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
		QSF_LOG_PRINTS(INFO, "Build Ent 4")
		return entityId;
		//Unit is created now apply transform
	}
	// editor
} // user
