// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <qsf_editor/view/View.h>
#include <..\tmp\qt\uic\asset_collector_tool\ui_UnitViewer.h>

#include <camp/userobject.hpp>

#include <qsf\log\LogSystem.h>
#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include <QtWidgets\qtreewidgetitemiterator.h>
#include <qsf\map\Map.h>
#include <qsf/debug/request/CompoundDebugDrawRequest.h>
#include <qsf/job/JobProxy.h>
#include <qsf/math/Transform.h>
#include <asset_collector_tool\extern\include\Magick++.h>
#include <qsf\component\base\TransformComponent.h>
#include <chrono>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class UnitViewer;
}
namespace qsf
{
	namespace editor
	{
		class AssetEditHelper;
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
		*    Class that is responsible for the Asset Collection and provide the view (window)
		*	 note that I dont get where the filename "IndicatorView" comes from and therefor i was not able to change these names from the *.cpp, *.h and *.ui files 
		*
		*  @note
		*    - The UI is created via source code
		*/
		class UnitViewer : public qsf::editor::View
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::UnitViewer" unique pluginable view ID


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
			UnitViewer(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~UnitViewer();


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		protected:
			virtual void retranslateUi() override;
			virtual void changeVisibility(bool visible) override;


		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]
		protected:
			virtual void showEvent(QShowEvent* qShowEvent) override;
			virtual void hideEvent(QHideEvent* qHideEvent) override;


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			/**
			*  @brief
			*    Perform a GUI rebuild
			*/
			void rebuildGui();
		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		private Q_SLOTS:
			void onPushSelectButton(const bool pressed);
			void onPushOpenDoorButton(const bool pressed);
			void onPushCloseDoorButton(const bool pressed);
			void onSelectionChanged(uint64 Id);
			void OnPushBlinker(const bool pressed);
			void OnPushBlueLight(const bool pressed);
			void OnPushHeadLight(const bool pressed);
			void OnPushSetSimulating(const bool pressed);
			void OnPushRotateTires(const bool pressed);
			void onPushSelectEntity(const bool pressed);
			void onPushAddDebugCircles(const bool pressed);
			void onPushRemoveDebugCircles(const bool pressed);
			void onPushUpdateNodes(const bool pressed);
			void onpushAddAllNodes(const bool pressed);
			void onPushBritifyMap(const bool pressed);
			void onPushBritifyCrossing(const bool pressed);
			void onPushbrakeButton(const bool pressed);
			void onPushtrafficwarnerbutton(const bool pressed);
			void onPushreverseButtonbutton(const bool pressed);
			void onPushtinteriorbutton(const bool pressed);
			void onPushenviromentbutton(const bool pressed);

			void onPush_AnalyseMeshButton(const bool pressed);

			std::string  ReadSkeleton();
			void OnPushStartReadMaterial(const bool pressed);
		private:
			Magick::Image* image;
			QImage* mQImage;
			uint64 mOldAssetId;
			enum CrossState
			{
				Inactive,
				Active
			};
			CrossState mCrossState;
			struct CrossPixel
			{
				int Red;
				int Green;
				int Blue;
				int Alpha;
				int X;
				int Y;
			};
			int ImageRows;
			int Imagecolumns;
			std::vector<CrossPixel> MyCrossPixels;
			Ui::UnitViewer*	mUiUnitViewer;	///< UI view instance, can be a null pointer, we have to destroy the instance in case we no longer need it
		std::vector<qsf::Entity*> GetSelectedEntity();			
		qsf::JobProxy mTireJob;
		qsf::JobProxy mMaterialViewerJob;
			void MaterialViewerJob(const qsf::JobArguments& jobArguments);
			std::vector<qsf::Entity*> AffectedByTire;
			void TireJob(const qsf::JobArguments& jobArguments);
			void ResetWheelsAfterDeselection();
			glm::vec3 ApplyMasterTransformToNode(qsf::Transform* Transform, glm::vec3 NodePos);
			//Crossings
			std::vector<uint64> DebugEntitiesFullNodeView;
			std::vector<uint64> DebugEntities;
			qsf::CompoundDebugDrawRequest SelectedNodeDebug;
			uint32 mSelectedNodeDebug;
			//checks if it element of DebugEntities
			bool IsEntityAllreadySelected(uint64 Target,std::vector<uint64> CompareList);
			void UpdateStreetDebugNodes();
			void OnSelectionChange_SetAdditionalLightButtons(QPushButton* Buttonname,std::string Lightdescription);
			struct MaterialAssets
			{
				int LineItAppears =-1;
				int GID = qsf::getUninitialized<uint64>();
				std::string Name ="";
				bool operator<(const MaterialAssets& a) const
				{
					if (LineItAppears < a.LineItAppears)
						return true;
					return false;
				}
			};
			//we do not know how many childs to expect therefore uint64
			std::vector<std::pair<qsf::Transform,qsf::Entity*>> ChildTransforms;
			void CollectAllChildTransforms(qsf::Entity* ent);
			void ApplyChildTransforms(uint64 index);
			uint64 CurrentChildIndex;
			uint64 CurrentParentIndex;
			//we may measure time and improve timing
			void BritifyJob(const qsf::JobArguments& jobArguments);
			bool mBlockInput;
			int OldProgress;
			qsf::JobProxy mBritifyJob;

			enum BritifyState
			{
				Start,
				Collect_Childs,
				Collect_Childs_Finish,
				Apply_Transfroms_Parents,
				Update_StreetSections,
				Apply_Transforms_Childs,
				Finish
			};
			BritifyState mBritifyState;
			BritifyState mOldBritifyState;
			glm::vec3 mMidpoint;
			std::vector<qsf::TransformComponent*> mBritifyParents;
			std::chrono::time_point<std::chrono::steady_clock> mJobStartTime;
			void HandleStreetSection(qsf::Entity* ent);
			void SetNewJobTiming();
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
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::UnitViewer)




/*


//[QSF error] QSF noticed that prototype 12099955740153434413 is using prefab asset "38362" which apparently does not exist
done
[QSF warning] Prefab asset em5/prefab/beaverfield_vehicles/fd_bcfd_tanker52.json: QSF noticed that prototype 6084894884677994421 is referencing the base prototype 3710961177554599144, but the base prototype cannot be found in the prefab "em5/prefab/beaverfield_vehicles/fd_bffd_ladder18" (global asset ID 18667)

//low[QSF warning] QSF noticed an issue while parsing Boost ptree: Property unknown: "Speed Change" of "tnt::TrailerComponent"

//done might apply also to Skeletons? And Materials? But Materials are managed by meshs?
[QSF error] QSF failed to create OGRE entity "6301475433588001582" with mesh "47696". Exception caught: OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 47696 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)


em5/prefab/beaverfield_vehicles/pd_bcsd_cruiser241
*/