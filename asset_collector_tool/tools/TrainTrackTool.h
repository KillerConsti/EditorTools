// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <qsf_editor/view/View.h>

#include <camp/userobject.hpp>
#include <qsf/job/JobProxy.h>
#include <qsf/debug/DebugDrawProxy.h>
#include <qsf_editor/tool/Tool.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include <qsf_ai/worldModel/trafficLanes/TrafficLaneWorld.h>
#include <QtCore\qstringlist.h>
#include <QtWidgets\QListWidgetItem>
#include <qsf/debug/request/CompoundDebugDrawRequest.h>
#include <QtWidgets\qtreewidgetitemiterator.h>
#include <qsf\map\Map.h>
#include <qsf/component/nodes/PathComponent.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class TrainTrackTool;
}
namespace qsf
{
	namespace editor
	{
		namespace base
		{
			class Operation;
		}
	}
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{


		//[-------------------------------------------------------]
		//[ Classes                                               ]
		//[-------------------------------------------------------]
		/**
		*  @brief
		*    Indicator view class
		*
		*  @note
		*    - The UI is created via source code
		*/
		class TrainTrackTool : public QObject, public qsf::editor::Tool
		{


			//[-------------------------------------------------------]
			//[ Qt definitions (MOC)                                  ]
			//[-------------------------------------------------------]
			Q_OBJECT	// All files using the Q_OBJECT macro need to be compiled using the Meta-Object Compiler (MOC) of Qt, else slots won't work!
						// (VisualStudio: Header file -> Right click -> Properties -> "Custom Build Tool")


						//[-------------------------------------------------------]
						//[ Public definitions                                    ]
						//[-------------------------------------------------------]
		public:
			static const uint32 PLUGINABLE_ID;	///< "user::editor::TrainTrackTool" unique pluginable view ID


												//[-------------------------------------------------------]
												//[ Public methods                                        ]
												//[-------------------------------------------------------]
		public:
			/**
			*  @brief
			*    Constructor
			*
			*  @param[in] viewManager
			*    Optional pointer to the view manager this view should be registered to, can be a null pointer
			*  @param[in] qWidgetParent
			*    Pointer to parent Qt widget, can be a null pointer (in this case you're responsible for destroying this view instance)
			*/
			explicit TrainTrackTool(qsf::editor::ToolManager* toolManager);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~TrainTrackTool();
			Ui::TrainTrackTool*					mUITrainTrackTool;			///< UI EM5 fire simulation tool instance, always valid, we have to destroy the instance in case we no longer need it
			//[-------------------------------------------------------]
			//[ Protected virtual qsf::editor::Tool methods           ]
			//[-------------------------------------------------------]
		protected:
			virtual bool onStartup(qsf::editor::ToolboxView& toolboxView) override;
			virtual void retranslateUi(qsf::editor::ToolboxView& toolboxView) override;
			virtual void onShutdown(qsf::editor::ToolboxView& toolboxView) override;
			//[-------------------------------------------------------]
			//[ Private Qt slots (MOC)                                ]
			//[-------------------------------------------------------]
			private Q_SLOTS:
			void onPushSelectButton(const bool pressed);
			void onPushTreeVoew(const bool pressed);
			// qsf::editor::OperationManager
			void onUndoOperationExecuted(const qsf::editor::base::Operation& operation);
			void onRedoOperationExecuted(const qsf::editor::base::Operation& operation);

			void ReadMap(const bool pressed);
			void mCurrentSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
			void mDoubleClick(QListWidgetItem* yop);

			void CreatePathEntities(const bool pressed);

			void TreeWidgetcurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
			void OnPushGenerateNewTrackButton(const bool pressed);

			void OnPushAppendButton(const bool pressed);

			void ShowContextMenu(const QPoint &pos);
			void ExecutContextMenu(QAction *action);

			void mTreeViewDoubleClick(QTreeWidgetItem *item, const int column);
			//indexesMoved(const QModelIndexList &indexes);

						void onSelectionChanged(uint64 Id);
						void onMapChanged();
			//[-------------------------------------------------------]
			//[ Private data                                          ]
			//[-------------------------------------------------------]
		private:
			QStringList* ItemList;
			void CreateSingleTrack(int id);
			void CreateSingleTrackWithFullName(std::string Name);
			std::vector<uint32> LaneIds;
			uint64 BuildEntity(glm::vec3 position, uint32 LaneId,std::string layer);
			void TrainTrackTool::buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select);

			qsf::CompoundDebugDrawRequest DetailViewSingleTrack;
			uint32 mDetailViewSingleTrack;

			qsf::CompoundDebugDrawRequest IDVisualsDebug;
			uint32 mIDVisualsDebug;


			void ApplyNodes(const qsf::ai::Lane* lane,glm::vec3 entityposition,qsf::Entity* TrackEntity);

			uint64 GetTreeEntity(std::string LaneName);
			void SetUpTree();

			//tree widget


			void addTreeRoot(QString name, QString description);
			void addTreeChild(QTreeWidgetItem *parent,QString name, QString description);

			void AddFinallyNodesToCombinedTrack(qsf::Entity* CombinedTrack, qsf::Entity* DonatorEntity);

			void UpdateTreeDebugVisual();
			void UpdateTreeSubNodes(qsf::Entity* CurrentEntity);
			qsf::CompoundDebugDrawRequest UpdateTreeDebug;
			uint32 mUpdateTreeDebug;

			qsf::CompoundDebugDrawRequest SelectedNodeDebug;
			uint32 mSelectedNodeDebug;
			qsf::Entity* GetTreeViewEntity();
			QTreeWidgetItem* GetCurrentTreeWidgetItem();
			bool IsCurrentTreeWidgetItemANode();
			void PolishNodes(std::vector<qsf::Node> &InputNodes);

			void RemoveNodeFromContextMenu();
			void SetNewNameFromContextMenu();
			void SetNewDescriptionFromContextMenu();
			void InvertDirection();

			void CloneTrack();
			//[-------------------------------------------------------]
			//[ CAMP reflection system                                ]
			//[-------------------------------------------------------]
			QSF_CAMP_RTTI()	// Only adds the virtual method "campClassId()", nothing more


		};


		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user


  //[-------------------------------------------------------]
  //[ CAMP reflection system                                ]
  //[-------------------------------------------------------]
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::TrainTrackTool)
