// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/IndicatorView.h"
#include <../../plugin_api/external/qt/include/QtWidgets/qfiledialog.h>
#include <asset_collector_tool/component/KCIndicatorComponent.h>
#include <qsf/plugin/PluginSystem.h>
#include <qsf\QsfHelper.h>
#include <qsf\plugin\Plugin.h>
#include <fstream>
#include <asset_collector_tool\game\Manager\GameManager.h>
#include <qsf/map/component/MapPropertiesBaseComponent.h>
#include <qsf\map/Map.h>
#include <qsf\QsfHelper.h>
#include <qsf\component\base\MetadataComponent.h>
#include <qsf/component/utility/AnnotationComponent.h>
#include <QtWidgets\qmenu.h>
#include <QtWidgets\qinputdialog.h>
#include <em5\map\EntityHelper.h>
#include <qsf/selection/SelectionManager.h>
#include <qsf_editor/application/Application.h>
#include <qsf_editor/selection/entity/EntitySelectionManager.h>
#include <qsf_editor/map/MapHelper.h>
#include <qsf_editor/EditorHelper.h>
#include <qsf_editor/application/Application.h>
#include <qsf_editor/application/manager/CameraManager.h>
#include <qsf_editor_base/operation/component/CreateComponentOperation.h>
#include <qsf_editor_base/operation/component/SetComponentPropertyOperation.h>
#include <qsf_editor_base/operation/component/DestroyComponentOperation.h>
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
		const uint32 IndicatorView::PLUGINABLE_ID = qsf::StringHash("user::editor::IndicatorView");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		IndicatorView::IndicatorView(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
			View(viewManager, qWidgetParent),
			mUI_IndicatorView(nullptr)
		{
			// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
			addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);

		}

		void IndicatorView::changeVisibility(bool visible)
		{

			// Lazy evaluation: If the view is shown the first time, create its content
			if (visible && nullptr == mUI_IndicatorView)
			{
				// Setup the view content
				QWidget* contentWidget = new QWidget(this);
				{

					// Load content to widget
					mUI_IndicatorView = new Ui::IndicatorView();
					mUI_IndicatorView->setupUi(contentWidget);
				}
				// Set content to view
				setWidget(contentWidget);
				// Connect Qt signals/slots
				connect(mUI_IndicatorView->loadexternallog, SIGNAL(clicked(bool)), this, SLOT(onPushLoadButton(bool)));
				mUI_IndicatorView->treeWidget->setColumnCount(3);
				addTreeRoot("Objects", "");
				mUI_IndicatorView->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
				GetSavePath();
				UpdateListView();
				if (!connect(mUI_IndicatorView->treeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &))))
				{
					QSF_LOG_PRINTS(INFO, "Slot connection treewidget custom context Menu failed")
				}
			//    void itemClicked(QTreeWidgetItem *item, int column);
			//if (!connect(mUI_IndicatorView->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(onitemClicked(QTreeWidgetItem*, int))))
			if (!connect(mUI_IndicatorView->treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(onitemClicked(QTreeWidgetItem *, int))))
			{
				QSF_LOG_PRINTS(INFO, "Slot connection item clicked failed")
			}
			}
			else if (!visible && nullptr == mUI_IndicatorView)
			{

			}
		}

		void IndicatorView::ShowContextMenu(const QPoint & pos)
		{
			auto CurrentItem = mUI_IndicatorView->treeWidget->currentItem();
			if (CurrentItem == nullptr)
				return;
			QMenu contextMenu("Action", mUI_IndicatorView->treeWidget);
			QAction action1("Kill Item", mUI_IndicatorView->treeWidget);
			QAction action2("Add Description", mUI_IndicatorView->treeWidget);
			contextMenu.addAction(&action1);
			contextMenu.addAction(&action2);
			connect(&contextMenu, SIGNAL(triggered(QAction*)), SLOT(ExecutContextMenu(QAction*)));
			contextMenu.exec(mUI_IndicatorView->treeWidget->mapToGlobal(pos));
		}

		void IndicatorView::ExecutContextMenu(QAction * action)
		{
			action->data().toString().toStdString();

			if (action->text().toStdString() == "Kill Item")
			{
				try
				{
					auto ent = QSF_MAINMAP.getEntityById(boost::lexical_cast<uint64>(mUI_IndicatorView->treeWidget->currentItem()->text(0).toStdString()));
					if(ent == nullptr)
					return;
					qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
					if (ent->getComponent<qsf::AnnotationComponent>() != nullptr)
					{
						compoundOperation2->pushBackOperation(new qsf::editor::base::DestroyComponentOperation(ent->getId(), qsf::StringHash("qsf::AnnotationComponent")));
					}
					if (ent->getComponent<kc::KCIndicatorComponent>() != nullptr)
					{
						compoundOperation2->pushBackOperation(new qsf::editor::base::DestroyComponentOperation(ent->getId(), qsf::StringHash("kc::KCIndicatorComponent")));
					}
					
					
					QSF_EDITOR_OPERATION.push(compoundOperation2);
					IndicatorView::UpdateListView();
				}
				catch (const std::exception&)
				{

				}
				//int column = mUI_IndicatorView->treeWidget->currentColumn();
				//QClipboard *clipboard = QGuiApplication::clipboard();
				//QString originalText = clipboard->text();
				// etc.
				//auto CurrentItemText = mUiDebugUnitView->treeWidget->currentItem()->text(column);
				//clipboard->setText(CurrentItemText);
			}
			else if("Add Description")
			{
				try
				{
					auto ent = QSF_MAINMAP.getEntityById(boost::lexical_cast<uint64>(mUI_IndicatorView->treeWidget->currentItem()->text(0).toStdString()));
					if (ent == nullptr)
						return;
					//QSF_LOG_PRINTS(INFO, "change desc of " << ent->getId())
						QInputDialog *dialog = new QInputDialog();
					dialog->setInputMode(QInputDialog::InputMode::IntInput);
					dialog->setWindowTitle("Set Description");
					dialog->setLabelText("New Description");
					dialog->setTextValue("");
					int ret = dialog->exec();
					QString text = dialog->textValue();
					if (ret == QDialog::Accepted && !text.isEmpty())
					{
						mUI_IndicatorView->treeWidget->currentItem()->setText(2,text);
					}
					qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
					if (ent->getComponent<qsf::AnnotationComponent>() == nullptr)
					{
						compoundOperation2->pushBackOperation(new qsf::editor::base::CreateComponentOperation(ent->getId(), qsf::StringHash("qsf::AnnotationComponent")));
					}
					compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(*ent->getOrCreateComponent<qsf::AnnotationComponent>(), qsf::AnnotationComponent::DESCRIPTION, text.toStdString()));

					QSF_EDITOR_OPERATION.push(compoundOperation2);
					//ent->getOrCreateComponent<qsf::AnnotationComponent>()->setDescription(text.toStdString());
				}
				catch (const std::exception&)
				{

				}
			}
		}

		void user::editor::IndicatorView::onitemClicked(QTreeWidgetItem * item, int column)
		{
			//QSF_LOG_PRINTS(INFO, "clicked item" << column << item->text(column).toStdString())
			if(column == 0)
				try
			{
				auto ent = QSF_MAINMAP.getEntityById(boost::lexical_cast<uint64>(mUI_IndicatorView->treeWidget->currentItem()->text(0).toStdString()));
				if (ent == nullptr)
					return;
				//QSF_LOG_PRINTS(INFO, "select ent? " << ent->getId())
				qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
				entitySelectionManager.clearSelection();

				entitySelectionManager.addIdToSelection(ent->getId());
				auto Pos = em5::EntityHelper(ent).getPosition();
				//we should compare to groundlevel first
				//Pos.y = QSF_EDITOR_APPLICATION.getCameraManager().getCameraComponent()->getEntity().getComponent<qsf::TransformComponent>()->getPosition().y;

				QSF_EDITOR_APPLICATION.getCameraManager().flyCameraToPosition(Pos);
			}
			catch (const std::exception&)
			{

			}
		}


		IndicatorView::~IndicatorView()
		{
			disconnect(mUI_IndicatorView->treeWidget, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ShowContextMenu(const QPoint &)));
		}

		void IndicatorView::GetSavePath()
		{
				for (auto a : QSF_PLUGIN.getPlugins())
				{
					if (a->getFilename().find("asset_collector_tool.dll") != std::string::npos)
					{
						path = a->getFilename();
						path.erase(path.end() - 24, path.end());


					}
				}
				QSF_LOG_PRINTS(INFO, "GameManager path " << path)
					return;
		}

		void IndicatorView::LoadFile(std::string Path)
		{
			std::ifstream myfile(Path);
			std::string line;
			bool first = true;
			int c = 0;
			if (myfile.is_open())
			{
				qsf::editor::base::CompoundOperation* compoundOperation2 = new qsf::editor::base::CompoundOperation();
				while (std::getline(myfile, line))
				{
					if (first)
					{
						auto Mapname = QSF_MAINMAP.getCoreEntity().getComponent<qsf::MapPropertiesBaseComponent>()->getMapName();
						first = false;
						if (line != Mapname)
						{
							QSF_LOG_PRINTS(INFO,"Map Missmatch! Interrupt loading! ")
							return;
						}
					}
					std::vector<std::string> splittedString;
					boost::split(splittedString, line, boost::is_any_of(" "), boost::token_compress_on);
					if (splittedString.size() != 2)
					{
						continue;
					}
						uint64 EntityId;
						std::string Color;
						try
						{
							EntityId = boost::lexical_cast<uint64>(splittedString.at(0));
							Color = splittedString.at(1);
							auto ent = QSF_MAINMAP.getEntityById(EntityId);
							if(ent == nullptr)
							continue;
							
							if (ent->getComponent<kc::KCIndicatorComponent>() == nullptr)
							{
								compoundOperation2->pushBackOperation(new qsf::editor::base::CreateComponentOperation(ent->getId(), qsf::StringHash("kc::KCIndicatorComponent")));
							}
							compoundOperation2->pushBackOperation(new qsf::editor::base::SetComponentPropertyOperation(*ent->getOrCreateComponent<kc::KCIndicatorComponent>(),camp::StringId("Color"),camp::Value(Color.c_str())));

							//camp::UserObject UO = ent->getOrCreateComponent<kc::KCIndicatorComponent>();
							//UO.set(camp::StringId("Color"),Color);
							c++;
						}
						catch (const std::exception& e)
						{
							QSF_LOG_PRINTS(INFO,e.what())
						}
					}
				QSF_LOG_PRINTS(INFO,"created Indicator for "<< c << " objects")
				myfile.close();
				QSF_EDITOR_OPERATION.push(compoundOperation2);
			}

			UpdateListView();
		}

		void IndicatorView::UpdateListView()
		{
			auto Item = GetItemAndClearTree("Objects");
			if (Item == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "cant resolve tree")
					return;
			}
			
			qsf::ComponentCollection::ComponentList<kc::KCIndicatorComponent> QueryFireComponents = qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<kc::KCIndicatorComponent>();
			//std::sort(BrokenMeshs.begin(), BrokenMeshs.end());//,compareByLength);
			for (auto a : QueryFireComponents)
			{
				std::string descriptopn ="No Description";
				if(a->getEntity().getComponent<qsf::AnnotationComponent>() != nullptr)
				descriptopn = a->getEntity().getComponent<qsf::AnnotationComponent>()->getDescription();
				auto node = addTreeChild(Item, boost::lexical_cast<std::string>(a->getEntityId()).c_str(), a->getEntity().getComponent<qsf::MetadataComponent>()->getName().c_str(), descriptopn.c_str(),"blo");
				/*for (auto b : a.PrefabUsingIt)
				{
					addTreeChild(node, "Prefab: ", b.first.c_str(), "Prototype: ", b.second.c_str());
				}*/
			}
		}

		QTreeWidgetItem* IndicatorView::addTreeChild(QTreeWidgetItem * parent, QString name, QString description, QString AdditionalInfos, QString BrokenComponentName)
		{
			QTreeWidgetItem *treeItem = new QTreeWidgetItem();

			// QTreeWidgetItem::setText(int column, const QString & text)
			treeItem->setText(0, name);
			treeItem->setText(1, description);
			if (AdditionalInfos != "")
				treeItem->setText(2, AdditionalInfos);
			parent->addChild(treeItem);
			return parent->child(parent->childCount() - 1); //return last child
		}
		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		void IndicatorView::retranslateUi()
		{
			mUI_IndicatorView->retranslateUi(this);
		}

		void IndicatorView::onPushLoadButton(const bool pressed)
		{
			QString filter = "File Description (*.txt)";
			auto Dialog = QFileDialog::getOpenFileName(nullptr, QString("Indicator File"), QString(path.c_str()), filter);
			if (Dialog.size() < 1)
				return;
			std::string Dia = Dialog.toStdString();
			LoadFile(Dia);
		}


		void IndicatorView::addTreeRoot(QString name, QString description)
		{
			// QTreeWidgetItem(QTreeWidget * parent, int type = Type)
			QTreeWidgetItem *treeItem = new QTreeWidgetItem(mUI_IndicatorView->treeWidget);

			// QTreeWidgetItem::setText(int column, const QString & text)
			treeItem->setText(0, name);
			treeItem->setText(1, description);
			//addTreeChild(treeItem, "1", "yolo");
			//TrainTrackTool::addTreeChild(treeItem, QString(name + "A"), "Child_first");
			//TrainTrackTool::addTreeChild(treeItem, QString(name + "B"), "Child_second");
		}

		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]
		void IndicatorView::showEvent(QShowEvent* qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);
			
		}

		void IndicatorView::hideEvent(QHideEvent* qHideEvent)
		{
			// Call the base implementation
			View::hideEvent(qHideEvent);
			// Disconnect Qt signals/slots
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::undoOperationExecuted, this, &IndicatorView::onUndoOperationExecuted);
			//disconnect(&QSF_EDITOR_OPERATION, &qsf::editor::OperationManager::redoOperationExecuted, this, &IndicatorView::onRedoOperationExecuted);
		}

		QTreeWidgetItem * IndicatorView::GetItemAndClearTree(std::string RootName)
		{
			QTreeWidgetItem *item = nullptr;
			for (int i = 0; i < mUI_IndicatorView->treeWidget->topLevelItemCount(); ++i)
			{

				item = mUI_IndicatorView->treeWidget->topLevelItem(i);
				if (item->data(0, 0).toString().toStdString() == RootName)
				{
					break;
				}
				// Do something with item ...
			}
			if (item == nullptr)
				return nullptr;
			for (size_t t = item->childCount(); t > 0; t--)
			{
				item->removeChild(item->takeChild((int)t - 1));
			}
			return item;
		}

		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
