// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Header guard                                          ]
//[-------------------------------------------------------]
#pragma once


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include <qsf/message/MessageProxy.h>
#include <qsf/job/JobProxy.h>
#include <QtCore\qobject.h>
#include <QtWidgets\qaction.h>

#include <QtCore\qcoreapplication.h>
#include <asset_collector_tool/component/KCIndicatorComponent.h>
#include <qsf/map/query/ComponentMapQuery.h>

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
		*   stores settings*/
		class GameManager// : QObject
		{
			//Q_OBJECT

		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		public:
			/**
		
			/**
			*  @brief
			*    Destructor
			*/
			static GameManager* instance;
			GameManager();
			~GameManager();

			static void init();
			static std::vector<std::pair<uint64,int>> LoadIndicatorComponents();
		private:
			void startup(const qsf::MessageParameters& parameters);
			void shutdown(const qsf::MessageParameters& parameters);
			qsf::MessageProxy		mStartupMessageProxy;
			qsf::MessageProxy		mShutdownMessageProxy;
			std::vector<kc::KCIndicatorComponent*> mOldList;
			void GameManager::InitSavePath();
			void GameManager::SaveComponentList();
			std::string path;
		};


	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
	} // editor
} // user

