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

#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include "qsf_editor/editmode/EditMode.h"
#include "qsf_editor/editmode/EditModeManager.h"
#include <qsf/renderer/terrain/TerrainComponent.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class TerrainEditToolbox;
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
		class TerrainEditToolbox : public QObject, public qsf::editor::Tool
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::TerrainEditToolbox" unique pluginable view ID


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
			explicit TerrainEditToolbox(qsf::editor::ToolManager* toolManager);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~TerrainEditToolbox();
			Ui::TerrainEditToolbox*					mUITerrainEditToolbox;			///< UI EM5 fire simulation tool instance, always valid, we have to destroy the instance in case we no longer need it
			//[-------------------------------------------------------]
			//[ Protected virtual qsf::editor::Tool methods           ]
			//[-------------------------------------------------------]

			bool GetMirrorX();
			bool GetMirrorY();
			float GetTerrainHeightByRayCast(int xPos,int yPos,int xTerrain,int yTerrain);
			public: //brush settings

			float GetBrushRadius();

			float GetHeight();
			enum BrushShape
			{
					Dome,
					Cone,
					Circle,
					Quad
			};

			BrushShape GetBrushShape();

			int GetBrushIntensity();
			enum TerrainEditMode2
			{
				Set,
				Raise,
				Smooth,
				Lower
			};
			TerrainEditMode2 GetEditMode();
			TerrainEditMode2 mMode;
			void SetHeight(float NewHeight);
			std::string path;
			//save stuff should be removed as we use the asset system
			std::string GetSavePath();			
			std::string InitSavePath();
			
			std::string mSavepath;
			static TerrainEditToolbox* GetInstance();
		protected:
			virtual bool onStartup(qsf::editor::ToolboxView& toolboxView) override;
			virtual void retranslateUi(qsf::editor::ToolboxView& toolboxView) override;
			virtual void onShutdown(qsf::editor::ToolboxView& toolboxView) override;
			//[-------------------------------------------------------]
			//[ Private Qt slots (MOC)                                ]
			//[-------------------------------------------------------]
			private Q_SLOTS:
			void onPushSelectButton(const bool pressed);
			// qsf::editor::OperationManager
			void onUndoOperationExecuted(const qsf::editor::base::Operation& operation);
			void onRedoOperationExecuted(const qsf::editor::base::Operation& operation);

			void OnBrushIntensitySliderChanged(const int value);
			void onRadiusSliderChanged(const  int value);

			void onPushSetHeight(const bool pressed);
			void onPushRaiseLower(const bool pressed);
			void onPushLower(const bool pressed);
			void onPushSmooth(const bool pressed);
			void onSetSaveDirectory(const bool pressed);
			void onPushLoadMap(const bool pressed);
			void onChangeBrushType(const int Type);
			//void onEditPrefab(QT::QString String);
			void onPushSaveMap(const bool pressed);
			void onCopyQSFMAP(const bool pressed);
			
			//[-------------------------------------------------------]
			//[ Private data                                          ]
			//[-------------------------------------------------------]
		private:
			boost::container::flat_set <uint64> CreatedUnits;
			bool WasPressed;
			static TerrainEditToolbox* instance;
			
			void LoadSettings();
			void SaveSettings();
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
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::TerrainEditToolbox)
