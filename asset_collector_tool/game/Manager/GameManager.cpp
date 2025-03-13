// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool/game/Manager/GameManager.h>
#include <qsf_editor/plugin/Messages.h>
#include <qsf\log\LogSystem.h>
#include <qsf\QsfHelper.h>
#include <qsf_editor\EditorHelper.h>
#include <em5\plugin\Jobs.h>
#include <qsf_editor/menu/MenuBar.h>
#include <qsf\map\Map.h>
#include <qsf\map\Entity.h>
#include <qsf/map/component/EnvironmentComponent.h>
#include <qsf/renderer/compositor/DefaultCompositingComponent.h>
#include <QtWidgets\qinputdialog.h>
#include <QtWidgets\qerrormessage.h>
#include <qsf_editor_base/operation/component/SetComponentPropertyOperation.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include <asset_collector_tool\component\EditorToolsHelperComponent.h>
#include <qsf_editor_base/operation/component/CreateComponentOperation.h>

//building unit
#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include "qsf_editor/selection/layer/LayerSelectionManager.h"
#include <qsf/asset/Asset.h>
#include <qsf/prototype/helper/PrefabContent.h>
#include <qsf/prototype/BasePrototypeManager.h>
#include <qsf_editor_base/operation/entity/CreateEntityOperation.h>
#include <qsf_editor_base/operation/entity/DestroyEntityOperation.h>
#include <qsf_editor_base/operation/layer/CreateLayerOperation.h>
#include <qsf_editor_base/operation/layer/SetLayerPropertyOperation.h>
#include <qsf_editor_base/operation/data/BackupPrototypeOperationData.h>
#include <qsf/prototype/helper/PrefabContent.h>
#include <qsf/prototype/BasePrototypeManager.h>
#include <qsf/map/query/ComponentMapQuery.h>

#include <qsf/map/layer/LayerManager.h>
#include <qsf/map/layer/Layer.h>
#include <qsf/component/base/MetadataComponent.h>
#include <qsf_editor_base/operation/component/DestroyComponentOperation.h>

//execute views
#include <qsf_editor/application/MainWindow.h>
#include <asset_collector_tool\view\DebugUnitView.h>
#include <asset_collector_tool\view\indicator\ScenarioScriptTool.h>
#include <asset_collector_tool\view\ImageDecompiler.h>
#include <em5\EM5Helper.h>
#include <em5\game\Game.h>
#include <QtCore\qcoreapplication.h>
#include <qsf_editor/renderer/RenderView.h>
#include "qsf/renderer/window/RenderWindow.h"
#include <qsf/renderer/component/CameraComponent.h>
#include <qsf\platform\PlatformSystem.h>
#include <QtWidgets\qmessagebox.h>
#include <asset_collector_tool\extern\include\Magick++.h>
#include <fstream>
#include <filesystem>
#include <em5/plugin/Messages.h>
#include <asset_collector_tool/component/KCIndicatorComponent.h>
#include <qsf/plugin/PluginSystem.h>
#include <qsf/map/component/MapPropertiesBaseComponent.h>
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{

		GameManager* GameManager::instance = nullptr;




		GameManager::GameManager()
		{
			mStartupMessageProxy.registerAt(qsf::MessageConfiguration(em5::Messages::GAME_STARTUP_FINISHED), boost::bind(&GameManager::startup, this, _1));
			mShutdownMessageProxy.registerAt(qsf::MessageConfiguration(em5::Messages::GAME_SHUTDOWN_STARTING), boost::bind(&GameManager::shutdown, this, _1));
		}


		GameManager::~GameManager()
		{
			mStartupMessageProxy.unregister();
			mShutdownMessageProxy.unregister();

		}

		void GameManager::init()
		{
			if (instance == nullptr)
				instance = new GameManager();
			
		}
		std::vector<std::pair<uint64, int>> GameManager::LoadIndicatorComponents()
		{
			return std::vector<std::pair<uint64, int>>();
		}
		void GameManager::startup(const qsf::MessageParameters & parameters)
		{
			InitSavePath();
			auto Placehpolder = qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<kc::KCIndicatorComponent>();
			mOldList.clear();
			mOldList = Placehpolder.copyToVector();
		}
		void GameManager::shutdown(const qsf::MessageParameters & parameters)
		{
			//QSF_LOG_PRINTS(INFO,"GameManager Shutdown triggered")
			qsf::ComponentCollection::ComponentList<kc::KCIndicatorComponent> QueryFireComponents = qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<kc::KCIndicatorComponent>();
			if(QueryFireComponents.empty())
			return;
			if(mOldList.size() == QueryFireComponents.size())
			return;
			//Remove all preexisting entries
			auto newList = QueryFireComponents.copyToVector();
			for (auto a : mOldList)
			{
				newList.erase(std::remove(newList.begin(), newList.end(), a), newList.end());
			}
			mOldList = newList;
			//QSF_LOG_PRINTS(INFO, "store list")
			SaveComponentList();
			
		}

		void GameManager::InitSavePath()
		{
			for (auto a : QSF_PLUGIN.getPlugins())
			{
				if (a->getFilename().find("asset_collector_tool.dll") != std::string::npos)
				{
					path = a->getFilename();
					path.erase(path.end() - 24, path.end());


				}
			}
			QSF_LOG_PRINTS(INFO,"GameManager path " << path)
			return;
		}


		void GameManager::SaveComponentList()
		{
			{
				auto Mapname = QSF_MAINMAP.getCoreEntity().getComponent<qsf::MapPropertiesBaseComponent>()->getMapName();
				std::ofstream ofs(path +Mapname + "_Indicators.txt", std::ofstream::trunc);
				QSF_LOG_PRINTS(INFO, "here " << path << Mapname << "Indicators.txt")
				//first line tells us the map!
				ofs << Mapname;
				for (auto a : mOldList)
				{
					if(a->getEntity().getComponent<kc::KCIndicatorComponent>() == nullptr)
					continue;
					camp::UserObject UO = a->getEntity().getComponent<kc::KCIndicatorComponent>();
					ofs <<"\n"<< a->getEntityId() << " "<< UO.get(camp::StringId("Color")).to<std::string>();
				}
				ofs.close();
			}
		}

	} // editor
} // user
