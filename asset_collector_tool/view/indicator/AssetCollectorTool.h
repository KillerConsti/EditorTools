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
#include <qsf\asset\Asset.h>
#include <qsf/prototype/Prototype.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
//[-------------------------------------------------------]
//[ Forward declarations                                  ]
//[-------------------------------------------------------]
namespace Ui
{
	class AssetCollectorTool;
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
		class AssetCollectorTool : public qsf::editor::View
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
			static const uint32 PLUGINABLE_ID;	///< "user::editor::AssetCollectorTool" unique pluginable view ID


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
			AssetCollectorTool(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent);

			/**
			*  @brief
			*    Destructor
			*/
			virtual ~AssetCollectorTool();


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

			//list of assets that should be copied (written in onPushSelectButton and used inside onPushCopyButton)
			std::vector<uint64> mylist;
			//a function that is triggered when we are finished to copy the files into a temp folder
			//this will be needed once we try to make it compatible for the online editor
			//but right now it has no function at all
			void onAssetReady();
			/*
			this is the helper which let us duplicate files
			look at the em5 examples where i have copied it from
			*/

			//DeepScan creates a protounit and reads throught everything
			//out GlobalAssetListToAdd
			static std::vector<uint64> DeepScan(uint64 PrefabGlobalAssetId,std::vector<uint64>& AllreadyScannedMaterials);
			//avoid Nestling For-loops
			static bool MaterialAllreadyScanned(std::vector<uint64> AllreadyScannedMaterials,uint64 TestMaterial);
			static uint64 BuildEntity(uint64 GlobalAssetId);
			static void buildInstantiateTemporaryPrototypesOperation(qsf::editor::base::CompoundOperation & compoundOperation, const std::vector<qsf::Prototype*>& temporaryPrototypes);
			
			std::shared_ptr<qsf::editor::AssetEditHelper>	mAssetEditHelper;
		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]
		private Q_SLOTS:
		/*Action which is performed once the Load Button is pressed
		-build a list of projects (mods) which are ignored from the user-input
		-use the asset dependency collector to collect all required assets
		-copy the asset numbers which are not ignored,mounted and not corrupted into "mylist" vector
		*/

			void onPushSelectButton(const bool pressed);
			/*Action which is performed once the "Copy Unit" Button is pressed
			-duplicates the files (which are identificated by the "mylist-vector") into the current destination project
			-in case of changes (i.e. ignore list changed) it is required to push the other button first 
			*/
			void onPushCopyButton(const bool pressed);

			//redirects to the web page
			void onPushHelpButton(const bool pressed);
		private:
			static int DoSthCool(int a , int b);
			static std::vector<uint64> GiveMeCategoryStuff(std::string Category,bool DeepScan,std::string IgnoreList);
			void WorkForCategory(std::string);
			static std::vector<uint64> ReadAssetDependeciesAndFilter(qsf::Assets AssetsIn);
			static qsf::Assets ReadCategory(std::string);
			

			static std::string m_Cat;
			static bool m_DeepScan;
			static std::string m_IgnoreList;
		//[-------------------------------------------------------]
		//[ Private data                                          ]
		//[-------------------------------------------------------]
		private:
			Ui::AssetCollectorTool*	mUiAssetCollectorTool;	///< UI view instance, can be a null pointer, we have to destroy the instance in case we no longer need it
			

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
QSF_CAMP_TYPE_NONCOPYABLE(user::editor::AssetCollectorTool)
