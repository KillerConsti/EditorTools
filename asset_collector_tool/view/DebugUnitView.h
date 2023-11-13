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
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class DebugUnitView;
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
		class DebugUnitView : public qsf::editor::View
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::DebugUnitView" unique pluginable view ID


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
			DebugUnitView(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~DebugUnitView();


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

			void OnMessageArrives(const qsf::LogMessage& Message);
			void OnPrototypeIdChanged();
			void OnPushSearchForPrototype(const bool pressed);
			void OnLoadLogFile(const bool pressed);
			void OnSaveLogFile(const bool pressed);


			void ShowContextMenu(const QPoint &pos);
			void ExecutContextMenu(QAction *action);

		private:
			void MissingComponentMessage(std::string Message);
			void MissingTextureMessage(std::string Message);
			void MissingMaterialMessage(std::string Message);
			void BrokenMeshMessage(std::string Message);
			void MissingPrototypeMessage(std::string Message);
			QTreeWidgetItem* GetItemAndClearTree(std::string RootName);
		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Ui::DebugUnitView*	mUiDebugUnitView;	///< UI view instance, can be a null pointer, we have to destroy the instance in case we no longer need it
			bool JsonContainsPrototypeFaster(std::string path);
			bool JsonContainsPrototypeSingleSearch(std::string path,uint64 prototype);
			bool JsonContainsBrokenMesh(std::string path);
			bool BrokenMeshAllreadyInList(int iterator,std::string CompareString,std::string Prototype);
			bool MaterialContainsPrototypeFaster(std::string path);
			bool FindPrototypeAndReferencesFromPrefabs(std::string path);
			void addTreeRoot(QString name, QString description);
			void readfile_ParseLine(std::string);
			QTreeWidgetItem* addTreeChild(QTreeWidgetItem *parent, QString name, QString description,QString AdditionalInfos ="",QString BrokenComponentName ="");

			struct _MissingComponent
			{
				std::string ComponentId="";
				std::string PrototTypeId ="";
				std::string Prefab ="";
				std::string BrokenComponentName ="";

				bool operator<(const _MissingComponent& a) const
				{
					if(boost::lexical_cast<uint64>(ComponentId) != boost::lexical_cast<uint64>(a.ComponentId))
						return boost::lexical_cast<uint64>(ComponentId) < boost::lexical_cast<uint64>(a.ComponentId);
					return boost::lexical_cast<uint64>(PrototTypeId) < boost::lexical_cast<uint64>(a.PrototTypeId);
				}
	 		};

			struct _MissingTextures
			{
				std::string GlobalAssetId = "";
				std::string MaterialId = "";
				std::string MaterialName = "";

				bool operator<(const _MissingTextures& a) const
				{
					if (boost::lexical_cast<uint64>(GlobalAssetId) != boost::lexical_cast<uint64>(a.GlobalAssetId))
						return boost::lexical_cast<uint64>(GlobalAssetId) < boost::lexical_cast<uint64>(a.GlobalAssetId);
					return boost::lexical_cast<uint64>(MaterialId) < boost::lexical_cast<uint64>(a.MaterialId);
				}
			};

			struct _BrokenMeshs
			{
				std::string Mesh_GlobalAssetId = "";
				std::string CurrentFaultyMesh;
				std::vector<std::pair<std::string, std::string>> PrefabUsingIt;
				
				std::string GuessWhatMightBeBetter;
				bool operator<(const _BrokenMeshs& a) const
				{
						//return boost::lexical_cast<uint64>(Mesh_GlobalAssetId) < boost::lexical_cast<uint64>(a.Mesh_GlobalAssetId);
					return boost::lexical_cast<std::string>(CurrentFaultyMesh )< boost::lexical_cast<std::string>(a.CurrentFaultyMesh);
				}
			};

			struct _PropertyUnknown
			{
				std::string ComponentId = "";
				std::string PropertyId = "";
				//std::vector<std::pair<std::string,std::string>> PrefabUsingIt;
				std::vector<std::string> PrefabUsingIt;
				bool operator<(const _PropertyUnknown& a) const
				{
					if (boost::lexical_cast<uint64>(ComponentId) != boost::lexical_cast<uint64>(a.ComponentId))
						return boost::lexical_cast<uint64>(ComponentId) < boost::lexical_cast<uint64>(a.ComponentId);
					return boost::lexical_cast<uint64>(PropertyId) < boost::lexical_cast<uint64>(a.PropertyId);
				}
			};

			struct _MissingPrototypes
			{
				std::string PrototypeIdSource = "";
				std::string PrototypeIdTarget = "";
				std::string GlobalAssetIdSource;
				std::string GlobalAssetIdTarget;
				std::vector<std::string> RefCounter_source;
				std::vector<std::string> RefCounter_target;

				bool operator<(const _MissingPrototypes& a) const
				{
					if (boost::lexical_cast<uint64>(PrototypeIdSource) != boost::lexical_cast<uint64>(a.PrototypeIdSource))
						return GlobalAssetIdTarget < a.GlobalAssetIdTarget;
					return boost::lexical_cast<uint64>(PrototypeIdSource) < boost::lexical_cast<uint64>(a.PrototypeIdSource);
				}
			};

			std::vector<_MissingPrototypes> MissingPrototypes;
			std::vector<_MissingComponent> MissingComponents; 
			std::vector<_MissingTextures> MissingTextures;
			std::vector<_BrokenMeshs> BrokenMeshs;
			void UpdateBrokenMeshs();
			void UpdateMissingComponents();
			void UpdateMissingTextures();
			void UpdateMissingPrototypes();

			bool WriteDebugLineMissingPrototype(std::string& output, int index);
			std::string GetLocalAssetName(std::string GlobalAssetId); //Wrapper for boost::lexical_cast
			struct MissingMeshs
			{
				std::string GlobalAssetId = "";
				std::vector<uint64> Requesting;
			};
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
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::DebugUnitView)




/*


//[QSF error] QSF noticed that prototype 12099955740153434413 is using prefab asset "38362" which apparently does not exist
done
[QSF warning] Prefab asset em5/prefab/beaverfield_vehicles/fd_bcfd_tanker52.json: QSF noticed that prototype 6084894884677994421 is referencing the base prototype 3710961177554599144, but the base prototype cannot be found in the prefab "em5/prefab/beaverfield_vehicles/fd_bffd_ladder18" (global asset ID 18667)

//low[QSF warning] QSF noticed an issue while parsing Boost ptree: Property unknown: "Speed Change" of "tnt::TrailerComponent"

//done might apply also to Skeletons? And Materials? But Materials are managed by meshs?
[QSF error] QSF failed to create OGRE entity "6301475433588001582" with mesh "47696". Exception caught: OGRE EXCEPTION(6:FileNotFoundException): Cannot locate resource 47696 in resource group QsfResourceGroup or any other group. in ResourceGroupManager::openResource at G:/Projects/qsf-external-source/ogre/ogre_v2-1-ofenberg/OgreMain/src/OgreResourceGroupManager.cpp (line 757)


em5/prefab/beaverfield_vehicles/pd_bcsd_cruiser241
*/