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

//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
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
		*   this is a wrapper to have a better "view" menu
		*
		*  @note
		*    - The UI is created via source code
		*/
		class KC_AbstractView : public qsf::editor::View
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::KC_AbstractView" unique pluginable view ID


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
			KC_AbstractView(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~KC_AbstractView();


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
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::KC_AbstractView)




/*


//[QSF error] QSF noticed that prototype 12099955740153434413 is using prefab asset "38362" which apparently does not exist
done
[QSF warning] Prefab asset em5/prefab/beaverfield_vehicles/fd_bcfd_tanker52.json: QSF noticed that prototype 6084894884677994421 is referencing the base prototype 3710961177554599144, but the base prototype cannot be found in the prefab "em5/prefab/beaverfield_vehicles/fd_bffd_ladder18" (global asset ID 18667)

//low[QSF warning] QSF noticed an issue while parsing Boost ptree: Property unknown: "Speed Change" of "tnt::TrailerComponent"

//done might apply also to Skeletons? And Materials? But Materials are managed by meshs?
[QSF error] QSF failed to create OGRE entity "6301475433588001582" with mesh "47696". Exception caught: OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 47696 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)


em5/prefab/beaverfield_vehicles/pd_bcsd_cruiser241
*/