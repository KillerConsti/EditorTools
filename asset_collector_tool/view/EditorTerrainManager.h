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

#include <qsf\log\LogSystem.h>
#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include <QtWidgets\qtreewidgetitemiterator.h>
#include <asset_collector_tool\kc_terrain\TerrainComponent.h>
#include <ogre\Ogre.h>
#include <qsf\map\Entity.h>
#include <ui_EditorTerrainManager.h>
#include <qsf/job/JobProxy.h>
#include <asset_collector_tool\editmode\PlaceUnitEditMode.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class EditorTerrainManager;
}



//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace kc_terrain
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
		class EditorTerrainManager : public qsf::editor::View
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::EditorTerrainManager" unique pluginable view ID


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
			EditorTerrainManager(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~EditorTerrainManager();

			static EditorTerrainManager* GetInstance();
			void setLabelName(std::string i);

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
			Ui::EditorTerrainManager*					mUiEditorTerrainManager;

			void ActivateStickToGnd(bool active);
			static EditorTerrainManager* Instance;
		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		private Q_SLOTS:
			void onPressStickToGnd(const bool pressed);
			void onSelectionChanged(uint64 Id);
			bool PushObjectOnTopmostTerrain(uint64 id);
		private:
			qsf::JobProxy mWaitUntilIsInEditMode;
			void WaitUntilIsInEditMode(const qsf::JobArguments& jobArguments);
			void ReplaceEditMode(bool IsGood);
			user::editor::PlaceUnitEditMode* mPlaceUnitEditMode;

		//[-------------------------------------------------------]
		//[ CAMP reflection system                                ]
		//[-------------------------------------------------------]
			QSF_CAMP_RTTI()	// Only adds the virtual method "campClassId()", nothing more


		};


	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
} // user


//[-------------------------------------------------------]
//[ CAMP reflection system                                ]
//[-------------------------------------------------------]
QSF_CAMP_TYPE_NONCOPYABLE(kc_terrain::EditorTerrainManager)




/*


//[QSF error] QSF noticed that prototype 12099955740153434413 is using prefab asset "38362" which apparently does not exist
done
[QSF warning] Prefab asset em5/prefab/beaverfield_vehicles/fd_bcfd_tanker52.json: QSF noticed that prototype 6084894884677994421 is referencing the base prototype 3710961177554599144, but the base prototype cannot be found in the prefab "em5/prefab/beaverfield_vehicles/fd_bffd_ladder18" (global asset ID 18667)

//low[QSF warning] QSF noticed an issue while parsing Boost ptree: Property unknown: "Speed Change" of "tnt::TrailerComponent"

//done might apply also to Skeletons? And Materials? But Materials are managed by meshs?
[QSF error] QSF failed to create OGRE entity "6301475433588001582" with mesh "47696". Exception caught: OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 47696 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)


em5/prefab/beaverfield_vehicles/pd_bcsd_cruiser241
*/