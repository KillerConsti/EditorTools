// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool\tools\TrainTrackTool.h>
#include <asset_collector_tool\view\indicator\TerrainEditTool.h>
#include "ui_TrainTrackTool.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

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

#include <qsf_editor/editmode/EditMode.h>
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

#include <qsf/debug/request/CircleDebugDrawRequest.h>
#include <qsf/debug/DebugDrawManager.h>
#include <qsf/math/CoordinateSystem.h>
#include <qsf_ai/worldModel/WorldModelManager.h>
#include "em5/ai/NavigationMaps.h"
#include <qsf_ai/worldModel/trafficLanes/ClosestTrafficLanesHelper.h>
#include <em5/ai/MoverType.h>
#include <qsf_ai/worldModel/trafficLanes/TrafficLaneWorld.h>
#include <qsf/debug/request/SegmentDebugDrawRequest.h>
#include <qsf/component/nodes/PathComponent.h>
#include <em5\map\EntityHelper.h>
#include <qsf/debug/request/CompoundDebugDrawRequest.h>
#include <qsf/debug/request/CircleDebugDrawRequest.h>
#include <qsf/debug/request/TextDebugDrawRequest.h>
#include <em5/plugin/Plugin.h>
#include <QtCore\qstringlistmodel.h>
#include <QtCore\qstringlist.h>
#include <QtWidgets\QListWidgetItem>
#include <qsf_editor_base/operation/layer/CreateLayerOperation.h>
#include <qsf_editor/operation/layer/SelectLayerOperation.h>
#include <qsf_editor_base/operation/layer/SetLayerPropertyOperation.h>
#include <qsf_editor_base/operation/entity/SelectEntityOperation.h>
#include <qsf/component/base/MetadataComponent.h>
#include <qsf_editor_base/operation/entity/SelectEntityOperation.h>
#include <qsf_editor_base/operation/entity/AddEntitiesToSelectionOperation.h>
#include <qsf_editor_base/operation/entity/RemoveEntitiesFromSelectionOperation.h>
#include <em5\map\MapHelper.h>
#include <qsf\debug\request\LaneDebugDrawRequest.h>
#include <QtWidgets\qmenu.h>
#include <QtWidgets\qinputdialog.h>
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
		const uint32 TrainTrackTool::PLUGINABLE_ID = qsf::StringHash("qsf::editor::TrainTrackTool");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]


		TrainTrackTool::TrainTrackTool(qsf::editor::ToolManager * toolManager) :
			qsf::editor::Tool(toolManager),
			mUITrainTrackTool(new Ui::TrainTrackTool()),
			mUpdateTreeDebug(qsf::getUninitialized<uint32>())
		{
			ItemList = new QStringList();
		}

		TrainTrackTool::~TrainTrackTool()
		{
		}



		bool TrainTrackTool::onStartup(qsf::editor::ToolboxView & toolboxView)
		{
			if (mUITrainTrackTool != nullptr)

				mUITrainTrackTool->setupUi(toolboxView.widget());
			if (mUITrainTrackTool == nullptr) //shouldnt happen
				return false;
			connect(mUITrainTrackTool->pushButton_fill_tree_view, SIGNAL(clicked(bool)), this, SLOT(CreatePathEntities(bool)));
			if (!connect(mUITrainTrackTool->generateNewTrack, SIGNAL(clicked(bool)), this, SLOT(OnPushGenerateNewTrackButton(bool))))
				QSF_LOG_PRINTS(INFO, "Slot connection generate track failed")
				connect(mUITrainTrackTool->append_after, SIGNAL(clicked(bool)), this, SLOT(OnPushAppendButton(bool)));
			//list widget
			mUITrainTrackTool->listWidget->addItem("no accessible");
			mUITrainTrackTool->listWidget->addItem("streetmap found");
			ReadMap(true);
			if (!connect(mUITrainTrackTool->listWidget->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(mCurrentSelectionChanged(const QModelIndex&, const QModelIndex&))))

				QSF_LOG_PRINTS(INFO, "Slot connection change selection failed")

				if (!connect(mUITrainTrackTool->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(mDoubleClick(QListWidgetItem*))))
					QSF_LOG_PRINTS(INFO, "Slot connection double click failed")
					//QTreeWidget
					SetUpTree();
			if (!connect(mUITrainTrackTool->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(TreeWidgetcurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*))))
				QSF_LOG_PRINTS(INFO, "Slot connection treewidget item change failed")

				mUITrainTrackTool->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

			if (!connect(mUITrainTrackTool->treeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &))))
				QSF_LOG_PRINTS(INFO, "Slot connection treewidget custom context Menu failed")
				//Tree widget signals void currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
				//(QTreeWidgetItem *item, int column)

				//void itemDoubleClicked(QTreeWidgetItem *item, int column);
				if (!connect(mUITrainTrackTool->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(mTreeViewDoubleClick(QTreeWidgetItem*, int))))
					QSF_LOG_PRINTS(INFO, "treeWidget :: Slot connection double click failed")

					//boost::signals2::signal<void(IdType)> qsf::SelectionManagerTemplate< IdType >::Selected
					qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
				auto con2 = &entitySelectionManager.Selected.connect(boost::bind(&TrainTrackTool::onSelectionChanged, this, _1), boost::signals2::at_back);
				if (con2 == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "entitySelectionManager :: Slot connection failed 2 ")
				}

				//if(!connect(boost::bind(&QSF_EDITOR_APPLICATION.postMapRebuild, this,_1 SLOT(TrainTrackTool::onMapChanged())))
					//QSF_LOG_PRINTS(INFO,"failed")
				/*auto con2 = &entitySelectionManager.Selected.connect(boost::bind(&TrainTrackTool::onSelectionChanged, this, _1), boost::signals2::at_back);
				if (con2 == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "entitySelectionManager :: Slot connection failed 2 ")
				}*/
					return true;

		}

		void TrainTrackTool::mTreeViewDoubleClick(QTreeWidgetItem * item, const int column) //fly to entity 
		{
			if (item == nullptr || item->parent() == nullptr)
			{
				return;
			}
			qsf::Entity* ent = GetTreeViewEntity();
			if (ent == nullptr)
				return;
			auto data = item->data(0, 0).toString().toStdString();
			//QSF_LOG_PRINTS(INFO,"RemoveNodeFromContextMenu :: data "<< data)
			//now find node
			data.erase(0, 4);
			int index;
			try
			{
				index = boost::lexical_cast<int>(data);
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO, " TrainTrackTool::mTreeViewDoubleClick :" << e.what())
					return;
			}
			auto PC = ent->getComponent<qsf::PathComponent>();
			if (PC == nullptr)
				return;
			auto Nodes = PC->getNodes();
			if (index > Nodes.size() - 1)
				return;
			auto Pos = PC->getNodes().at(index).getPosition() + em5::EntityHelper(ent).getPosition();
			//we should compare to groundlevel first
			//Pos.y = QSF_EDITOR_APPLICATION.getCameraManager().getCameraComponent()->getEntity().getComponent<qsf::TransformComponent>()->getPosition().y;
			
			QSF_EDITOR_APPLICATION.getCameraManager().flyCameraToPosition(Pos);
		}

		void TrainTrackTool::mCurrentSelectionChanged(const QModelIndex & current, const QModelIndex & previous)
		{
			//remove old compound object

			//generate new compound object
			//QSF_LOG_PRINTS(INFO, current.data(0).toString().toStdString());

			if (QSF_DEBUGDRAW.isRequestIdValid(mDetailViewSingleTrack))
				QSF_DEBUGDRAW.cancelRequest(mDetailViewSingleTrack);
			DetailViewSingleTrack.mLanes.clear();

			uint64 entID = GetTreeEntity(current.data(0).toString().toStdString());
			auto entity = QSF_MAINMAP.getEntityById(entID);
			if (entity == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "it is a nullptr")
					return;
			}
			if (entity->getComponent<qsf::PathComponent>() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no path component found")
					return;
			}
			QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>().setSelectionById(entID);
			auto ENtPOS = em5::EntityHelper(entity).getPosition();
			std::vector<glm::vec3> pos;
			for (size_t t = 0; t < entity->getComponent<qsf::PathComponent>()->getNumberOfNodes(); t++)
				pos.push_back(ENtPOS + entity->getComponent<qsf::PathComponent>()->getNodes().at(t).getPosition());
			DetailViewSingleTrack.mLanes.push_back(qsf::LaneDebugDrawRequest(pos, 1.5f, 0.5f, 1.f, 4, true));
			mDetailViewSingleTrack = QSF_DEBUGDRAW.requestDraw(DetailViewSingleTrack);
		}

		void TrainTrackTool::UpdateTreeDebugVisual()
		{
			if (QSF_DEBUGDRAW.isRequestIdValid(mUpdateTreeDebug))
				QSF_DEBUGDRAW.cancelRequest(mUpdateTreeDebug);
			UpdateTreeDebug.mLanes.clear();
			qsf::Entity* TreeEntity = GetTreeViewEntity();
			if (TreeEntity == nullptr)
				return;
			if (TreeEntity->getComponent<qsf::PathComponent>() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no path component found")
					return;
			}
			auto ENtPOS = em5::EntityHelper(TreeEntity).getPosition();
			std::vector<glm::vec3> pos;
			for (size_t t = 0; t < TreeEntity->getComponent<qsf::PathComponent>()->getNumberOfNodes(); t++)
				pos.push_back(ENtPOS + TreeEntity->getComponent<qsf::PathComponent>()->getNodes().at(t).getPosition());
			UpdateTreeDebug.mLanes.push_back(qsf::LaneDebugDrawRequest(pos, 2.5f, 0.75f, 1.f, 2, true));
			mUpdateTreeDebug = QSF_DEBUGDRAW.requestDraw(UpdateTreeDebug);
			UpdateTreeSubNodes(TreeEntity);
		}

		void TrainTrackTool::UpdateTreeSubNodes(qsf::Entity*  CurrentEntity)
		{
			auto CurrentItem = mUITrainTrackTool->treeWidget->currentItem();
			if (CurrentItem == nullptr)
				return;
			if (CurrentItem->parent() != nullptr) //go one step back
				CurrentItem = CurrentItem->parent();

			qsf::Entity* ent = GetTreeViewEntity();
			if (ent == nullptr)
				return;
			auto PC = ent->getComponent<qsf::PathComponent>();
			if (PC == nullptr)
				return;
			//Check old amount of Nodes vs new amount
			int cc = CurrentItem->childCount();
			if (cc == PC->getNumberOfNodes())
				return; //no update needed

				//better remember node by position
			mUITrainTrackTool->treeWidget->setCurrentItem(CurrentItem);
			for (size_t t = CurrentItem->childCount(); t > 0; t--)
			{
				CurrentItem->removeChild(CurrentItem->takeChild((int)t - 1));
			}
			auto Pos = em5::EntityHelper(ent).getPosition();
			for (size_t t = 0; t < PC->getNumberOfNodes(); t++)
			{
				auto NodePos = Pos + PC->getNodes().at(t).getPosition();
				std::string Name = "Node" + boost::lexical_cast<std::string>(t);
				std::string PosS = "Position" + boost::lexical_cast<std::string>(NodePos.x) + " , " + boost::lexical_cast<std::string>(NodePos.y) + " , " + boost::lexical_cast<std::string>(NodePos.z);
				addTreeChild(CurrentItem, Name.c_str(), PosS.c_str());
			}

		}

		qsf::Entity * TrainTrackTool::GetTreeViewEntity()
		{
			auto CurrentItem = mUITrainTrackTool->treeWidget->currentItem();
			if (CurrentItem == nullptr)
				return nullptr;
			if (CurrentItem->parent() != nullptr) //go one step back
				CurrentItem = CurrentItem->parent();

			//QSF_LOG_PRINTS(INFO, CurrentItem->text(0).toStdString());
			auto Layermanager = &QSF_MAINMAP.getLayerManager();
			qsf::Layer* Layer = Layermanager->getLayerByName(qsf::StringHash("KC_COMBINED_TRAINTRACKS"));
			if (Layer == nullptr)
				return nullptr;
			qsf::Entity* MatchingEntity = nullptr;
			for (auto a : Layer->getEntityIds())
			{
				auto ent = QSF_MAINMAP.getEntityById(a);
				if (ent == nullptr || ent->getComponent<qsf::MetadataComponent>() == nullptr)
					continue;
				if (ent->getComponent<qsf::MetadataComponent>()->getName() == CurrentItem->text(0).toStdString())
					return ent;
			}
			return nullptr;
		}

		QTreeWidgetItem * TrainTrackTool::GetCurrentTreeWidgetItem()
		{
			return mUITrainTrackTool->treeWidget->currentItem();
		}

		bool TrainTrackTool::IsCurrentTreeWidgetItemANode()
		{
			auto Item = mUITrainTrackTool->treeWidget->currentItem();
			if (Item == nullptr)
				return false;
			if (Item->parent() == nullptr)
				return false;
			return true;
		}

		void TrainTrackTool::PolishNodes(std::vector<qsf::Node>& AllMyNodes)
		{
			bool resize_done = false;
			int Counter = 0;
			while (!resize_done)
			{
				resize_done = true;
				for (size_t t = 1; t < AllMyNodes.size() - 1; t++)
				{
					qsf::Node CurrentNode = AllMyNodes.at(t);
					qsf::Node NextNode = AllMyNodes.at(t + 1);
					qsf::Node PreviousNode = AllMyNodes.at(t - 1);
					glm::vec3 Direction = glm::normalize(NextNode.getPosition() - PreviousNode.getPosition());
					glm::vec3 NewDirection = glm::normalize(NextNode.getPosition() - CurrentNode.getPosition());
					//QSF_LOG_PRINTS(INFO, "t: " << t << "distance " << Direction - NewDirection)
					if (glm::distance(Direction, NewDirection) < 0.001)
					{
						AllMyNodes.erase(AllMyNodes.begin() + t);
						resize_done = false;
						Counter++;
						break;
					}

				}
			}
			for (size_t t = 0; t < AllMyNodes.size(); t++)
			{
				if (t == 0 || t == AllMyNodes.size() - 1)
				{
					//beginning and end node
					//there isnt a good way we want to set a radius and i guess we dont even need to
				}
				else
				{
					float frontdistance = glm::distance(AllMyNodes.at(t).getPosition(), AllMyNodes.at(t - 1).getPosition());
					float backdistance = glm::distance(AllMyNodes.at(t).getPosition(), AllMyNodes.at(t + 1).getPosition());
					if (frontdistance > backdistance)
						frontdistance = backdistance;
					AllMyNodes.at(t).setRadius(frontdistance / 2);
				}
			}
		}

		void TrainTrackTool::RemoveNodeFromContextMenu()
		{
			auto data = GetCurrentTreeWidgetItem()->data(0, 0).toString().toStdString();
			//QSF_LOG_PRINTS(INFO,"RemoveNodeFromContextMenu :: data "<< data)
			//now find node
			data.erase(0, 4);
			int index;
			try
			{
				index = boost::lexical_cast<int>(data);
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO, " TrainTrackTool::RemoveNodeFromContextMenu :" << e.what())
					return;
			}
			//QSF_LOG_PRINTS(INFO, "RemoveNodeFromContextMenu :: data " << data)
			auto ent = GetTreeViewEntity();
			if (ent == nullptr)
				return;
			auto PC = ent->getComponent<qsf::PathComponent>();
			if (PC == nullptr)
				return;
			auto Nodes = PC->getNodes();
			if (index > Nodes.size() - 1)
				return;
			Nodes.erase(Nodes.begin() + index);
			PolishNodes(Nodes);
			PC->setNodes(Nodes);
			PC->setAllPropertyOverrideFlags(true);
			UpdateTreeDebugVisual();

		}

		void TrainTrackTool::SetNewNameFromContextMenu()
		{
			qsf::Entity* ent = GetTreeViewEntity();
			QTreeWidgetItem* Item = GetCurrentTreeWidgetItem();
			//Item->setText(2)

			bool ok;
			// Ask for birth date as a string.
			QString text = QInputDialog::getText(0, "Input dialog",
				"New Name:", QLineEdit::Normal,
				"", &ok);
			if (ok && !text.isEmpty()) {
				Item->setText(0, text);

				qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
				compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(ent->getId(), qsf::MetadataComponent::COMPONENT_ID, qsf::MetadataComponent::NAME, text.toStdString().c_str()));
				QSF_EDITOR_OPERATION.push(compoundOperation2);
			}
		}

		void TrainTrackTool::SetNewDescriptionFromContextMenu()
		{
			qsf::Entity* ent = GetTreeViewEntity();
			QTreeWidgetItem* Item = GetCurrentTreeWidgetItem();
			//Item->setText(2)

			bool ok;
			// Ask for birth date as a string.
			QString text = QInputDialog::getText(0, "Input dialog",
				"New Description:", QLineEdit::Normal,
				"", &ok);
			if (ok && !text.isEmpty()) {
				Item->setText(1, text);

				qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
				compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(ent->getId(), qsf::MetadataComponent::COMPONENT_ID, qsf::MetadataComponent::DESCRIPTION, text.toStdString().c_str()));
				QSF_EDITOR_OPERATION.push(compoundOperation2);
			}
		}

		void TrainTrackTool::InvertDirection()
		{
			qsf::Entity* ent = GetTreeViewEntity();
			if (ent == nullptr)
				return;
			auto PC = ent->getComponent<qsf::PathComponent>();
			if (PC == nullptr)
				return;
			auto Nodes = PC->getNodes();
			std::vector<qsf::Node> InvertedNodes;
			for (size_t t = Nodes.size(); t > 0; t--)
			{
				InvertedNodes.push_back(Nodes.at(t - 1));
			}
			PolishNodes(InvertedNodes);
			PC->setNodes(InvertedNodes);
			PC->setAllPropertyOverrideFlags(true);
			UpdateTreeDebugVisual();
		}

		void TrainTrackTool::CloneTrack()
		{
			qsf::Entity* OrginalEntity = GetTreeViewEntity();
			QTreeWidgetItem* OrginalItem = GetCurrentTreeWidgetItem();
			if (OrginalEntity == nullptr || OrginalItem == nullptr)
				return;
			auto Layermanager = &QSF_MAINMAP.getLayerManager();
			qsf::Layer* Layer = Layermanager->getLayerByName(qsf::StringHash("KC_COMBINED_TRAINTRACKS"));
			if (Layer == nullptr)
				return;
			glm::vec3 position = em5::EntityHelper(OrginalEntity).getPosition();
			uint64 NewEntity = BuildEntity(position, 0, "KC_COMBINED_TRAINTRACKS");
			qsf::Entity* entity = QSF_MAINMAP.getEntityById(NewEntity);
			std::string EntityName = OrginalEntity->getComponent<qsf::MetadataComponent>()->getName() + boost::lexical_cast<std::string>(Layer->getEntityIds().size());
			qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
			compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(NewEntity, qsf::TransformComponent::COMPONENT_ID, qsf::TransformComponent::POSITION, entity->getOrCreateComponent<qsf::TransformComponent>()->getPosition()));
			compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(NewEntity, qsf::MetadataComponent::COMPONENT_ID, qsf::MetadataComponent::NAME, EntityName.c_str()));
			compoundOperation2->pushBackOperation(new qsf::editor::base::CreateComponentOperation(NewEntity, qsf::StringHash("qsf::PathComponent")));
			QSF_EDITOR_OPERATION.push(compoundOperation2);
			SetUpTree();
			//mUITrainTrackTool->treeWidget->setcurr
			for (int i = 0; i < mUITrainTrackTool->treeWidget->topLevelItemCount(); ++i)
			{
				QTreeWidgetItem *item = mUITrainTrackTool->treeWidget->topLevelItem(i);
				if (item->text(0).toStdString() == EntityName)
					mUITrainTrackTool->treeWidget->setCurrentItem(item);
				// Do something with item ...
			}
			auto DEPC = OrginalEntity->getComponent<qsf::PathComponent>();
			if (DEPC == nullptr)
				return; //shouldnt happen
			entity->getComponent<qsf::PathComponent>()->setNodes(DEPC->getNodes());
			entity->getComponent<qsf::PathComponent>()->setAllPropertyOverrideFlags(true);

		}

		void TrainTrackTool::onSelectionChanged(uint64 Id)
		{
			QSF_LOG_PRINTS(INFO,"sth was selected "<< Id)
			qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
			if(entitySelectionManager.getSelectionSize() != 1 )
			return;
			auto ent = QSF_MAINMAP.getEntityById(Id);
			if(ent == nullptr)
			return;
			auto layer = em5::EntityHelper(ent).getLayer();
			if(layer == nullptr)
			return;
			if(layer->getName() != "KC_SINGLE_TRAINTRACKLAYER")
			return;
			if(ent->getComponent<qsf::MetadataComponent>() == nullptr)
			return;
			std::string EntityName = ent->getComponent<qsf::MetadataComponent>()->getName();
			auto a = mUITrainTrackTool->listWidget->findItems(EntityName.c_str(),Qt::MatchFlag::MatchExactly);
			if(a.size() == 0)
			return;
			mUITrainTrackTool->listWidget->setCurrentItem(a.at(0));
			mUITrainTrackTool->listWidget->setFocus();

			//Update by hand

			if (QSF_DEBUGDRAW.isRequestIdValid(mDetailViewSingleTrack))
				QSF_DEBUGDRAW.cancelRequest(mDetailViewSingleTrack);
			DetailViewSingleTrack.mLanes.clear();


			if (ent->getComponent<qsf::PathComponent>() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no path component found")
					return;
			}
			auto ENtPOS = em5::EntityHelper(ent).getPosition();
			std::vector<glm::vec3> pos;
			for (size_t t = 0; t < ent->getComponent<qsf::PathComponent>()->getNumberOfNodes(); t++)
				pos.push_back(ENtPOS + ent->getComponent<qsf::PathComponent>()->getNodes().at(t).getPosition());
			DetailViewSingleTrack.mLanes.push_back(qsf::LaneDebugDrawRequest(pos, 1.5f, 0.5f, 1.f, 4, true));
			mDetailViewSingleTrack = QSF_DEBUGDRAW.requestDraw(DetailViewSingleTrack);
		}

		void TrainTrackTool::onMapChanged()
		{
			QSF_LOG_PRINTS(INFO, "sth was loaded")
		}




		void TrainTrackTool::mDoubleClick(QListWidgetItem* yop)
		{
			uint64 entID = GetTreeEntity(yop->data(0).toString().toStdString());
			auto entity = QSF_MAINMAP.getEntityById(entID);
			if (entity == nullptr)
				return;
			QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>().setSelectionById(entID);
			QSF_EDITOR_APPLICATION.getCameraManager().flyCameraToEntityById(entID);
		}
		void TrainTrackTool::CreatePathEntities(const bool pressed)
		{
			auto Layermanager = &QSF_MAINMAP.getLayerManager();
			qsf::Layer* Layer = Layermanager->getLayerByName(qsf::StringHash("KC_SINGLE_TRAINTRACKLAYER"));
			if (Layer == nullptr) //create Layer
			{
				qsf::editor::base::CreateLayerOperation* CLO = new qsf::editor::base::CreateLayerOperation(Layermanager->getRootLayer().getId(), qsf::StringHash("KC_SINGLE_TRAINTRACKLAYER"));
				CLO->setText("KC_SINGLE_TRAINTRACKLAYER");
				QSF_EDITOR_OPERATION.push(CLO);
				qsf::editor::base::SetLayerPropertyOperation* SLP = new qsf::editor::base::SetLayerPropertyOperation(qsf::StringHash("KC_SINGLE_TRAINTRACKLAYER"), qsf::StringHash("Name"), camp::Value("KC_SINGLE_TRAINTRACKLAYER"));
				QSF_EDITOR_OPERATION.push(SLP);
			}
			Layer = Layermanager->getLayerByName(qsf::StringHash("KC_SINGLE_TRAINTRACKLAYER"));
			if (Layer->getEntityIds().empty()) //it is empty -> create entitites
			{
				if (qsf::ai::WorldModelManager::getInstance().tryAcquireReadAccess(0) == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "Train Track Manager Tool cant aquire read access for street network. Check if you allready calculated navigation map (0)")
						return;
				}
				const std::unique_ptr<qsf::ai::ManagedNavigationMapReadAccess> readAccess = qsf::ai::WorldModelManager::getInstance().acquireReadAccess(0);
				if (readAccess.get() == nullptr)
					return;
				const qsf::ai::WorldModel& requestedNavMap = readAccess.get()->get();
				const qsf::ai::TrafficLaneWorld* trafficLaneWorld = dynamic_cast<const qsf::ai::TrafficLaneWorld*>(&requestedNavMap);
				//qsf::ai::TrafficLaneWorld* trafficLaneWorld2 = const_cast<qsf::ai::TrafficLaneWorld*>(trafficLaneWorld);
				if (nullptr == trafficLaneWorld)
					return;
				// Cycle through all lanes
				const qsf::ai::LaneCollection& laneCollection = trafficLaneWorld->getLanes();
				for (auto laneId : LaneIds)
				{
					auto lane = laneCollection.tryGetLane(laneId);
					if (lane == nullptr)
						continue;
					//we want to create entity in the middle
					int MiddleIndex = lane->getNumNodes() / 2;
					auto Position = lane->getNodes().at(MiddleIndex).mPosition;

					//now lets create entities :)
					uint64 NewEntity = BuildEntity(Position, laneId, "KC_SINGLE_TRAINTRACKLAYER");
					qsf::Entity* entity = QSF_MAINMAP.getEntityById(NewEntity);
					std::string EntityName = "SingleTrack_" + boost::lexical_cast<std::string>(laneId);
					IDVisualsDebug.mTexts.push_back(qsf::TextDebugDrawRequest(EntityName, Position + glm::vec3(2.f, 0.f, 2.f), qsf::Color4::GREEN));
					qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
					compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(NewEntity, qsf::TransformComponent::COMPONENT_ID, qsf::TransformComponent::POSITION, entity->getOrCreateComponent<qsf::TransformComponent>()->getPosition()));
					compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(NewEntity, qsf::MetadataComponent::COMPONENT_ID, qsf::MetadataComponent::NAME, EntityName.c_str()));
					QSF_EDITOR_OPERATION.push(compoundOperation2);
					ApplyNodes(lane, Position, entity);
				}

			}
			else //Read Layer
			{
				for (auto a : Layer->getEntityIds())
				{
					auto ent = QSF_MAINMAP.getEntityById(a);
					if (ent == nullptr || ent->getComponent<qsf::MetadataComponent>() == nullptr)
						continue;
					CreateSingleTrackWithFullName(ent->getComponent<qsf::MetadataComponent>()->getName());

					IDVisualsDebug.mTexts.push_back(qsf::TextDebugDrawRequest(ent->getComponent<qsf::MetadataComponent>()->getName(), em5::EntityHelper(ent).getPosition() + glm::vec3(2.f, 0.f, 2.f), qsf::Color4::GREEN));
				}
			}
			mIDVisualsDebug = QSF_DEBUGDRAW.requestDraw(IDVisualsDebug);
			//no api export :(
			//qsf::editor::SelectLayerOperation* SLO = new qsf::editor::SelectLayerOperation(qsf::StringHash("KC_SINGLE_TRAINTRACKLAYER"));
			//QSF_EDITOR_OPERATION.push(SLO);
		}

		void TrainTrackTool::TreeWidgetcurrentItemChanged(QTreeWidgetItem * current, QTreeWidgetItem * previous)
		{
			UpdateTreeDebugVisual();

			SelectedNodeDebug.mCircles.clear();
			if (QSF_DEBUGDRAW.isRequestIdValid(mSelectedNodeDebug))
				QSF_DEBUGDRAW.cancelRequest(mSelectedNodeDebug);
			//erase last selection in any case
			if (current == nullptr || current->parent() == nullptr)
			{
				return;
			}
			qsf::Entity* ent = GetTreeViewEntity();
			if (ent == nullptr)
				return;
			auto data = current->data(0, 0).toString().toStdString();
			//QSF_LOG_PRINTS(INFO,"RemoveNodeFromContextMenu :: data "<< data)
			//now find node
			data.erase(0, 4);
			int index;
			try
			{
				index = boost::lexical_cast<int>(data);
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO, " TrainTrackTool::RemoveNodeFromContextMenu :" << e.what())
					return;
			}
			auto PC = ent->getComponent<qsf::PathComponent>();
			if (PC == nullptr)
				return;
			auto Nodes = PC->getNodes();
			if (index > Nodes.size() - 1)
				return;
			auto Pos = PC->getNodes().at(index).getPosition() + em5::EntityHelper(ent).getPosition();
			SelectedNodeDebug.mCircles.push_back(qsf::CircleDebugDrawRequest(Pos, qsf::CoordinateSystem::getUp(), 2.5f, qsf::Color4::GREEN, true));
			mSelectedNodeDebug = QSF_DEBUGDRAW.requestDraw(SelectedNodeDebug);
		}

		void TrainTrackTool::OnPushGenerateNewTrackButton(const bool pressed)
		{
			auto Layermanager = &QSF_MAINMAP.getLayerManager();
			qsf::Layer* Layer = Layermanager->getLayerByName(qsf::StringHash("KC_COMBINED_TRAINTRACKS"));
			if (Layer == nullptr)
				return;
			glm::vec3 position = glm::vec3(0, 50, 0);
			uint64 NewEntity = BuildEntity(position, 0, "KC_COMBINED_TRAINTRACKS");
			qsf::Entity* entity = QSF_MAINMAP.getEntityById(NewEntity);
			std::string EntityName = "CombinedTrack" + boost::lexical_cast<std::string>(Layer->getEntityIds().size() + 1);
			qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
			compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(NewEntity, qsf::TransformComponent::COMPONENT_ID, qsf::TransformComponent::POSITION, entity->getOrCreateComponent<qsf::TransformComponent>()->getPosition()));
			compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(NewEntity, qsf::MetadataComponent::COMPONENT_ID, qsf::MetadataComponent::NAME, EntityName.c_str()));
			QSF_EDITOR_OPERATION.push(compoundOperation2);
			SetUpTree();
			//mUITrainTrackTool->treeWidget->setcurr
			for (int i = 0; i < mUITrainTrackTool->treeWidget->topLevelItemCount(); ++i)
			{
				QTreeWidgetItem *item = mUITrainTrackTool->treeWidget->topLevelItem(i);
				if (item->text(0).toStdString() == EntityName)
					mUITrainTrackTool->treeWidget->setCurrentItem(item);
				// Do something with item ...
			}

		}

		void TrainTrackTool::OnPushAppendButton(const bool pressed)
		{
			//find entity
			qsf::Entity* MatchingEntity = GetTreeViewEntity();
			if (MatchingEntity == nullptr)
				return;
			if (mUITrainTrackTool->listWidget->currentItem() == nullptr)
				return;
			uint64 Donator = GetTreeEntity(mUITrainTrackTool->listWidget->currentItem()->data(0).toString().toStdString());
			auto DonatorEnt = QSF_MAINMAP.getEntityById(Donator);
			if (DonatorEnt == nullptr)
				return;
			AddFinallyNodesToCombinedTrack(MatchingEntity, DonatorEnt);
		}

		void TrainTrackTool::ShowContextMenu(const QPoint & pos)
		{
			if (GetCurrentTreeWidgetItem() == nullptr)
				return;
			if (!IsCurrentTreeWidgetItemANode())
			{
				QMenu contextMenu("Modify Path Entity ", mUITrainTrackTool->treeWidget);
				QAction action1("Change Description", mUITrainTrackTool->treeWidget);
				QAction action2("Change Name", mUITrainTrackTool->treeWidget);
				QAction action3("Invert Direction", mUITrainTrackTool->treeWidget);
				QAction action4("Clone Track", mUITrainTrackTool->treeWidget);
				contextMenu.addAction(&action1);
				contextMenu.addAction(&action2);
				contextMenu.addAction(&action3);
				contextMenu.addAction(&action4);
				connect(&contextMenu, SIGNAL(triggered(QAction*)), SLOT(ExecutContextMenu(QAction*)));
				contextMenu.exec(mUITrainTrackTool->treeWidget->mapToGlobal(pos));
			}
			else
			{
				QMenu contextMenu("Modify Nodes ", mUITrainTrackTool->treeWidget);

				QAction action1("Remove Node", mUITrainTrackTool->treeWidget);
				contextMenu.addAction(&action1);
				connect(&contextMenu, SIGNAL(triggered(QAction*)), SLOT(ExecutContextMenu(QAction*)));
				contextMenu.exec(mUITrainTrackTool->treeWidget->mapToGlobal(pos));
			}
		}

		void TrainTrackTool::ExecutContextMenu(QAction *action)
		{
			action->data().toString().toStdString();
			//allready checked it out
			//auto Data = mUITrainTrackTool->treeWidget->currentItem()->data(1,0);
			//QSF_LOG_PRINTS(INFO,"Data "<< action->text().toStdString())
			if (action->text().toStdString() == "Remove Node")
				RemoveNodeFromContextMenu();

			else if (action->text().toStdString() == "Change Description")
			{
				SetNewDescriptionFromContextMenu();
			}
			else if (action->text().toStdString() == "Change Name")
			{
				SetNewNameFromContextMenu();
			}
			else if (action->text().toStdString() == "Invert Direction")
			{
				InvertDirection();
			}
			else if (action->text().toStdString() == "Clone Track")
			{
				CloneTrack();
			}
		}




		void TrainTrackTool::AddFinallyNodesToCombinedTrack(qsf::Entity* CombinedTrack, qsf::Entity* DonatorEntity)
		{
			//case 1 : we dont have path component -> just copy stuff
			auto PC = CombinedTrack->getComponent<qsf::PathComponent>();
			if (PC == nullptr)
			{
				qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
				const glm::vec3 Position = em5::EntityHelper(DonatorEntity).getPosition();
				compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(CombinedTrack->getId(), qsf::TransformComponent::COMPONENT_ID, qsf::TransformComponent::POSITION, Position));
				compoundOperation2->pushBackOperation(new qsf::editor::base::CreateComponentOperation(CombinedTrack->getId(), qsf::StringHash("qsf::PathComponent")));
				QSF_EDITOR_OPERATION.push(compoundOperation2);
				//that was easy :)
				//now copy nodes
				auto DEPC = DonatorEntity->getComponent<qsf::PathComponent>();
				if (DEPC == nullptr)
					return; //shouldnt happen
				CombinedTrack->getComponent<qsf::PathComponent>()->setNodes(DEPC->getNodes());
				CombinedTrack->getComponent<qsf::PathComponent>()->setAllPropertyOverrideFlags(true);
			}
			else //work with what we have so far
			{
				bool DonatorReverse = false;
				bool CombinedTrackReverse = false;

				glm::vec3 CombinedTrackPos = em5::EntityHelper(CombinedTrack).getPosition();
				glm::vec3 DonatorTrackPos = em5::EntityHelper(DonatorEntity).getPosition();
				auto CT_CurrentNodes = PC->getNodes();
				auto DEPC = DonatorEntity->getComponent<qsf::PathComponent>();
				if (DEPC == nullptr)
					return; //shouldnt happen
				auto Donator_CurrentNodes = DEPC->getNodes();
				//distance check so we know what matches best together
				float Distance = glm::distance(CombinedTrackPos + CT_CurrentNodes.at(0).getPosition(), DonatorTrackPos + Donator_CurrentNodes.at(0).getPosition()); //0 - 0 heißt Donator muss Rückwärts aufgespielt werden und startet bei seinem größten index 

				if (glm::distance(CombinedTrackPos + CT_CurrentNodes.at(CT_CurrentNodes.size() - 1).getPosition(), DonatorTrackPos + Donator_CurrentNodes.at(0).getPosition()) < Distance) //end -trifft auf 0 <-> normale Reihenfolge
				{
					Distance = glm::distance(CombinedTrackPos + CT_CurrentNodes.at(CT_CurrentNodes.size() - 1).getPosition(), DonatorTrackPos + Donator_CurrentNodes.at(0).getPosition());
					CombinedTrackReverse = true;
				}
				if (glm::distance(CombinedTrackPos + CT_CurrentNodes.at(CT_CurrentNodes.size() - 1).getPosition(), DonatorTrackPos + Donator_CurrentNodes.at(Donator_CurrentNodes.size() - 1).getPosition()) < Distance) //Ende trifft auf Ende
				{
					Distance = glm::distance(CombinedTrackPos + CT_CurrentNodes.at(CT_CurrentNodes.size() - 1).getPosition(), DonatorTrackPos + Donator_CurrentNodes.at(Donator_CurrentNodes.size() - 1).getPosition());
					CombinedTrackReverse = true;
					DonatorReverse = true;
				}

				if (glm::distance(CombinedTrackPos + CT_CurrentNodes.at(0).getPosition(), DonatorTrackPos + Donator_CurrentNodes.at(Donator_CurrentNodes.size() - 1).getPosition()) < Distance) //Anfang auf Ende
				{
					Distance = glm::distance(CombinedTrackPos + CT_CurrentNodes.at(0).getPosition(), DonatorTrackPos + Donator_CurrentNodes.at(Donator_CurrentNodes.size() - 1).getPosition());
					CombinedTrackReverse = false;
					DonatorReverse = true;
				}
				//Now put nods together
				std::vector<qsf::Node> AllMyNodes;
				if (!DonatorReverse && !CombinedTrackReverse) // 0 - 0 treffen aufeinander
				{
					for (size_t t = Donator_CurrentNodes.size() - 1; t >= 0; t--)
					{
						qsf::Node MyNode = Donator_CurrentNodes.at(t);
						MyNode.setPosition(MyNode.getPosition() - CombinedTrackPos + DonatorTrackPos);
						AllMyNodes.push_back(MyNode);
						if (t == 0)
							break;
					}
					for (size_t t = 0; t < CT_CurrentNodes.size(); t++)
					{
						AllMyNodes.push_back(CT_CurrentNodes.at(t));
					}
				}
				else if (!DonatorReverse && CombinedTrackReverse) //normaler Fall 
				{
					for (size_t t = 0; t < CT_CurrentNodes.size(); t++)
					{
						AllMyNodes.push_back(CT_CurrentNodes.at(t));
					}
					for (size_t t = 0; t < Donator_CurrentNodes.size(); t++)
					{
						qsf::Node MyNode = Donator_CurrentNodes.at(t);
						MyNode.setPosition(MyNode.getPosition() - CombinedTrackPos + DonatorTrackPos);
						AllMyNodes.push_back(MyNode);
					}

				}
				else if (DonatorReverse && CombinedTrackReverse) //Ende auf Ende (Donator muss gespiegelt werden
				{
					for (size_t t = 0; t < CT_CurrentNodes.size(); t++)
					{
						AllMyNodes.push_back(CT_CurrentNodes.at(t));
					}
					for (size_t t = Donator_CurrentNodes.size() - 1; t >= 0; t--)
					{
						qsf::Node MyNode = Donator_CurrentNodes.at(t);
						MyNode.setPosition(MyNode.getPosition() - CombinedTrackPos + DonatorTrackPos);
						AllMyNodes.push_back(MyNode);
						if (t == 0)
							break;
					}

				}

				else if (DonatorReverse && !CombinedTrackReverse) //Anfang auf Ende <-> Donator muss zuerst bewegt werden aber alles richtig rum
				{
					for (size_t t = 0; t < Donator_CurrentNodes.size(); t++)
					{
						qsf::Node MyNode = Donator_CurrentNodes.at(t);
						MyNode.setPosition(MyNode.getPosition() - CombinedTrackPos + DonatorTrackPos);
						AllMyNodes.push_back(MyNode);
					}
					for (size_t t = 0; t < CT_CurrentNodes.size(); t++)
					{
						AllMyNodes.push_back(CT_CurrentNodes.at(t));
					}

				}
				//we collected data <-> now polish

				PolishNodes(AllMyNodes);
				CombinedTrack->getComponent<qsf::PathComponent>()->setNodes(AllMyNodes);
				CombinedTrack->getComponent<qsf::PathComponent>()->setAllPropertyOverrideFlags(true);
				//
			}
			UpdateTreeDebugVisual();
		}


		uint64 TrainTrackTool::BuildEntity(glm::vec3 position, uint32 LaneId, std::string layher)
		{
			//Create Unit <-> this is quite long
			qsf::Prototype* prototype = QSF_MAINPROTOTYPE.getPrefabByLocalAssetId("em5/prefab/debug/debug_box");
			if (prototype == nullptr)
			{
				return qsf::getUninitialized<uint64>();
			}

			const uint32 selectedLayerId = qsf::StringHash(layher);
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
				const qsf::Asset* asset = &qsf::Asset(qsf::AssetProxy(qsf::StringHash("em5/prefab/debug/debug_box")).getGlobalAssetId());
				const std::string* name = (nullptr != asset) ? &asset->getName() : nullptr;
				usedPrefabContent->processForEntityInstancing(originalPrototypes, globalPrefabAssetId, name, &position);
			}
			//if (usedPrefabContent->getPrototypes().at(0)->getComponent<qsf::MeshComponent>() != nullptr)
				//usedPrefabContent->getPrototypes().at(0)->destroyComponent<qsf::DebugBoxComponent>();
			// Build compound operation that creates a copy of the cloned prototypes in the map
			qsf::editor::base::CompoundOperation* compoundOperation = new qsf::editor::base::CompoundOperation();
			buildInstantiateTemporaryPrototypesOperation(*compoundOperation, usedPrefabContent->getPrototypes(), selectedLayerId, false);
			uint64 entityId = usedPrefabContent->getMainPrototype()->getId();
			QSF_EDITOR_OPERATION.push(compoundOperation);
			return entityId;
			//Unit is created now apply transform
		}


		void TrainTrackTool::buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select)
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

		void TrainTrackTool::ApplyNodes(const qsf::ai::Lane * lane, glm::vec3 entitypos, qsf::Entity* AffectedTrack)
		{
			std::vector<qsf::Node> Nodes;
			glm::vec3 OldPos = glm::vec3(0, 0, 0);
			glm::vec3 direction = glm::vec3(0, 0, 0);
			for (auto b : lane->getNodes())
			{
				if (glm::distance(OldPos, b.mPosition) > 5.f) //only interested if these nodes are at least 1 m away
				{

					//notice that we must substract the entity position as paths are always relative to entities
					Nodes.push_back(qsf::Node(b.mPosition - entitypos));
					//QSF_DEBUGDRAW.requestDraw(qsf::CircleDebugDrawRequest(b.mPosition, qsf::CoordinateSystem::getUp(), 0.4f, qsf::Color4::GREEN, true), qsf::DebugDrawLifetimeData(qsf::Time::fromSeconds(120.0f)));
					//store position only if we considered this entity
					OldPos = b.mPosition;
				}

			}
			PolishNodes(Nodes);
			//QSF_LOG_PRINTS(INFO, "log nodes end "<< Nodes.size())
			qsf::editor::base::CreateComponentOperation* createComponentOperation = new qsf::editor::base::CreateComponentOperation(AffectedTrack->getId(), qsf::StringHash("qsf::PathComponent"));
			QSF_EDITOR_OPERATION.push(createComponentOperation);
			AffectedTrack->getComponent<qsf::PathComponent>()->setNodes(Nodes);
			AffectedTrack->getComponent<qsf::PathComponent>()->setAllPropertyOverrideFlags(true);
			return;
		}

		uint64 TrainTrackTool::GetTreeEntity(std::string LaneName)
		{
			//extract name
			auto Layermanager = &QSF_MAINMAP.getLayerManager();
			qsf::Layer* Layer = Layermanager->getLayerByName(qsf::StringHash("KC_SINGLE_TRAINTRACKLAYER"));
			if (Layer == nullptr) //create Layer
			{
				return qsf::getUninitialized<uint64>();
			}
			for (auto a : Layer->getEntityIds())
			{
				auto ent = QSF_MAINMAP.getEntityById(a);
				if (ent == nullptr || ent->getComponent<qsf::MetadataComponent>() == nullptr)
					continue;
				if (ent->getComponent<qsf::MetadataComponent>()->getName() == LaneName)
					return ent->getId();
			}
			return qsf::getUninitialized<uint64>();
		}

		void TrainTrackTool::SetUpTree()
		{
			mUITrainTrackTool->treeWidget->setColumnCount(2);
			mUITrainTrackTool->treeWidget->clear(); //maybe improve it later and not clear
			//debug
			// Add root nodes
			/*addTreeRoot("A", "Root_first");
			addTreeRoot("B", "Root_second");
			addTreeRoot("C", "Root_third");*/

			auto Layermanager = &QSF_MAINMAP.getLayerManager();
			qsf::Layer* Layer = Layermanager->getLayerByName(qsf::StringHash("KC_COMBINED_TRAINTRACKS"));
			if (Layer == nullptr) //create Layer
			{
				qsf::editor::base::CreateLayerOperation* CLO = new qsf::editor::base::CreateLayerOperation(Layermanager->getRootLayer().getId(), qsf::StringHash("KC_COMBINED_TRAINTRACKS"));
				CLO->setText("KC_COMBINED_TRAINTRACKS");
				QSF_EDITOR_OPERATION.push(CLO);
				qsf::editor::base::SetLayerPropertyOperation* SLP = new qsf::editor::base::SetLayerPropertyOperation(qsf::StringHash("KC_COMBINED_TRAINTRACKS"), qsf::StringHash("Name"), camp::Value("KC_COMBINED_TRAINTRACKS"));
				QSF_EDITOR_OPERATION.push(SLP);
			}
			Layer = Layermanager->getLayerByName(qsf::StringHash("KC_COMBINED_TRAINTRACKS"));
			for (auto a : Layer->getEntityIds()) //Each entity is a traintrack :)
			{
				auto ent = QSF_MAINMAP.getEntityById(a);
				if (ent == nullptr)
					continue;
				qsf::MetadataComponent* MC = ent->getComponent<qsf::MetadataComponent>();
				if (MC == nullptr)
					continue;
				addTreeRoot(MC->getName().c_str(), MC->getDescription().c_str());
			}
			UpdateTreeDebugVisual();

		}

		void TrainTrackTool::addTreeRoot(QString name, QString description)
		{
			// QTreeWidgetItem(QTreeWidget * parent, int type = Type)
			QTreeWidgetItem *treeItem = new QTreeWidgetItem(mUITrainTrackTool->treeWidget);

			// QTreeWidgetItem::setText(int column, const QString & text)
			treeItem->setText(0, name);
			treeItem->setText(1, description);
			//addTreeChild(treeItem, "1", "yolo");
			//TrainTrackTool::addTreeChild(treeItem, QString(name + "A"), "Child_first");
			//TrainTrackTool::addTreeChild(treeItem, QString(name + "B"), "Child_second");
		}

		void TrainTrackTool::addTreeChild(QTreeWidgetItem * parent, QString name, QString description)
		{
			QTreeWidgetItem *treeItem = new QTreeWidgetItem();

			// QTreeWidgetItem::setText(int column, const QString & text)
			treeItem->setText(0, name);
			treeItem->setText(1, description);

			// QTreeWidgetItem::addChild(QTreeWidgetItem * child)
			parent->addChild(treeItem);
		}




		void TrainTrackTool::retranslateUi(qsf::editor::ToolboxView & toolboxView)
		{
			mUITrainTrackTool->retranslateUi(toolboxView.widget());
		}

		void TrainTrackTool::onShutdown(qsf::editor::ToolboxView & toolboxView)
		{
			if (QSF_DEBUGDRAW.isRequestIdValid(mDetailViewSingleTrack))
				QSF_DEBUGDRAW.cancelRequest(mDetailViewSingleTrack);
			if (QSF_DEBUGDRAW.isRequestIdValid(mUpdateTreeDebug))
				QSF_DEBUGDRAW.cancelRequest(mUpdateTreeDebug);
			if (QSF_DEBUGDRAW.isRequestIdValid(mIDVisualsDebug))
				QSF_DEBUGDRAW.cancelRequest(mIDVisualsDebug);
			if (QSF_DEBUGDRAW.isRequestIdValid(mSelectedNodeDebug))
				QSF_DEBUGDRAW.cancelRequest(mSelectedNodeDebug);

				//disconnect everything '(most important is entity selection)
			qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
			entitySelectionManager.Selected.disconnect(boost::bind(&TrainTrackTool::onSelectionChanged, this, _1));

			disconnect(mUITrainTrackTool->pushButton_fill_tree_view, SIGNAL(clicked(bool)), this, SLOT(CreatePathEntities(bool)));
			disconnect(mUITrainTrackTool->generateNewTrack, SIGNAL(clicked(bool)), this, SLOT(OnPushGenerateNewTrackButton(bool)));
			disconnect(mUITrainTrackTool->append_after, SIGNAL(clicked(bool)), this, SLOT(OnPushAppendButton(bool)));
			disconnect(mUITrainTrackTool->listWidget->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(mCurrentSelectionChanged(const QModelIndex&, const QModelIndex&)));
			disconnect(mUITrainTrackTool->listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(mDoubleClick(QListWidgetItem*)));
			disconnect(mUITrainTrackTool->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(TreeWidgetcurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
			disconnect(mUITrainTrackTool->treeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));
			disconnect(mUITrainTrackTool->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(mTreeViewDoubleClick(QTreeWidgetItem*, int)));


		}




		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		void TrainTrackTool::onPushSelectButton(const bool pressed)
		{
		}


		void TrainTrackTool::onPushTreeVoew(const bool pressed)
		{
			//QSF_LOG_PRINTS(INFO, "tree view pushed");
		}


		void TrainTrackTool::onUndoOperationExecuted(const qsf::editor::base::Operation& operation)
		{

		}

		void TrainTrackTool::onRedoOperationExecuted(const qsf::editor::base::Operation& operation)
		{



		}

		void TrainTrackTool::ReadMap(const bool pressed)
		{
			if (qsf::ai::WorldModelManager::getInstance().tryAcquireReadAccess(0) == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Train Track Manager Tool cant aquire read access for street network. Check if you allready calculated navigation map (0)")
					return;
			}

			else
			{
				mUITrainTrackTool->listWidget->clear();
			}
			const std::unique_ptr<qsf::ai::ManagedNavigationMapReadAccess> readAccess = qsf::ai::WorldModelManager::getInstance().acquireReadAccess(0);
			if (readAccess.get() == nullptr)
				return;
			const qsf::ai::WorldModel& requestedNavMap = readAccess.get()->get();
			const qsf::ai::TrafficLaneWorld* trafficLaneWorld = dynamic_cast<const qsf::ai::TrafficLaneWorld*>(&requestedNavMap);
			//qsf::ai::TrafficLaneWorld* trafficLaneWorld2 = const_cast<qsf::ai::TrafficLaneWorld*>(trafficLaneWorld);
			if (nullptr == trafficLaneWorld)
				return;
			// Cycle through all lanes
			qsf::Color4 color = qsf::Color4::RED;
			const qsf::ai::LaneCollection& laneCollection = trafficLaneWorld->getLanes();
			for (uint32 t = 0; t < laneCollection.getNumLanes() - 1; t++)
			{
				auto CurrentLane = laneCollection.tryGetLane((uint32)t);
				if (CurrentLane != nullptr && CurrentLane->getTypeId() == 4)
				{
					qsf::CompoundDebugDrawRequest SingleRequest = qsf::CompoundDebugDrawRequest();
					for (size_t x = 0; x < CurrentLane->getNumNodes() - 1; x++)
					{
						//SingleRequest.mSegments.insert(SingleRequest.mSegments.begin(), qsf::SegmentDebugDrawRequest(qsf::Segment::fromTwoPoints(CurrentLane->getNodes().at(x).mPosition, CurrentLane->getNodes().at(x + 1).mPosition), color));
					}
					CreateSingleTrack(t);
					LaneIds.push_back(t);
					//ReallyLargeDebugDrawRequest.push_back(SingleRequest);
				}
			}
			return;
			//with a deque you have push_front and push_back. So you can insert in both ways
			std::deque<qsf::ai::Lane> lanes;

			uint32 StartLane;// = FindClosestNode(laneCollection);

			if (StartLane == qsf::getUninitialized<uint32>())
				return;

			qsf::ai::Lane& lane = const_cast<qsf::ai::Lane&>(laneCollection.getLane(StartLane));

			uint32 NewStartNode = lane.getEndNodeId();
			uint32 NewEndNode = lane.getStartNodeId();

			lanes.push_back(lane);
			// find connected lanes until there are no more
			//while (FindLanes(lanes, laneCollection, NewStartNode)) {}
			//while (FindLanesBefore(lanes, laneCollection, NewEndNode)) {}
			std::vector<qsf::Node> Nodes;
			glm::vec3 OldPos = glm::vec3(0, 0, 0);
			glm::vec3 direction = glm::vec3(0, 0, 0);
			for (auto a : lanes)
			{
				for (auto b : a.getNodes())
				{
					if (glm::distance(OldPos, b.mPosition) > 5.f) //only interested if these nodes are at least 1 m away
					{

						//notice that we must substract the entity position as paths are always relative to entities

						//kc had to edit it out
						//Nodes.push_back(qsf::Node(b.mPosition - em5::EntityHelper(getEntity()).getPosition()));
						//QSF_DEBUGDRAW.requestDraw(qsf::CircleDebugDrawRequest(b.mPosition, qsf::CoordinateSystem::getUp(), 0.4f, qsf::Color4::GREEN, true), qsf::DebugDrawLifetimeData(qsf::Time::fromSeconds(120.0f)));
						//store position only if we considered this entity
						OldPos = b.mPosition;
					}

				}
			}

			//resize nodes
			PolishNodes(Nodes);
		}




		void TrainTrackTool::CreateSingleTrack(int id)
		{
			std::string laneString = "SingleTrack_" + boost::lexical_cast<std::string>(id);
			ItemList->append(laneString.c_str());
			mUITrainTrackTool->listWidget->addItem(laneString.c_str());
		}

		void TrainTrackTool::CreateSingleTrackWithFullName(std::string Name)
		{
			mUITrainTrackTool->listWidget->addItem(Name.c_str());
		}













		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user


/*

*/