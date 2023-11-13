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


#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class UnitPlacerView;
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
		class UnitPlacerView : public qsf::editor::View
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::UnitPlacerView" unique pluginable view ID


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
			UnitPlacerView(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~UnitPlacerView();


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

			void PaintJob(const qsf::JobArguments& jobArguments);
			qsf::JobProxy PaintJobProxy;
		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		private:
			bool IsActive;
			qsf::DebugDrawProxy			mDebugDrawProxy; ///< Debug draw proxy for text output
			/**
			*  @brief
			*    Perform a GUI rebuild
			*/
			void rebuildGui();

			/**
			*  @brief
			*    Inform the indicator view that a component has been created, maybe its an indicator component
			*
			*  @param[in] entityId
			*    ID of the entity the created component is in
			*/
			void componentCreated();

			/**
			*  @brief
			*    Inform the view that a component has been destroyed
			*/
			void componentDestroyed();

			/**
			*  @brief
			*    Inform the view that the indicator component has been changed
			*
			*  @param[in] entityId
			*    ID of the entity the updated component is in
			*/
			void updateComponentData(uint64 entityId);

			float MoveDelta;
			glm::vec3 OldPos;
			bool UnitPlacerView::getPositionUnderMouse(glm::vec3& position);
			void buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation& compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes, uint32 layerId, bool select);
			uint64 BuildEntity(glm::vec3 position, qsf::StringHash shash);

			void MarkNotGuiltyPrefabs();
		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		private Q_SLOTS:
			void onPushSelectButton(const bool pressed);
			void onCurrentIndexChanged(int index);
			// qsf::editor::OperationManager
			void onUndoOperationExecuted(const qsf::editor::base::Operation& operation);
			void onRedoOperationExecuted(const qsf::editor::base::Operation& operation);
			
			void CreateUnit(glm::vec3 pos);
			void onMinimumSliderChanged(const  int value);
			void onMaximumSliderChanged(const  int value);
			void onEditPrefab(const QString & text);
			void onPrefab_1SliderChanged(const  int value);
			void onPrefab_2SliderChanged(const  int value);
			void onPrefab_3SliderChanged(const  int value);
		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Ui::UnitPlacerView*	mUiUnitPlacerView;	///< UI view instance, can be a null pointer, we have to destroy the instance in case we no longer need it
			boost::container::flat_set <uint64> CreatedUnits;
			bool WasPressed;
			
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
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::UnitPlacerView)
