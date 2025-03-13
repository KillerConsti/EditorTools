// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/manager/Settingsmanager.h"
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
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{

		Settingsmanager* Settingsmanager::instance = nullptr;




		Settingsmanager::Settingsmanager()
		{
			BrushSize = 15.f;
			BrushIntensity = 20.f;
			Brushshape = 0; 
		}


		Settingsmanager::~Settingsmanager()
		{
		

		}

		void Settingsmanager::init()
		{
			if (instance == nullptr)
				instance = new Settingsmanager();
		}
		float Settingsmanager::GetBrushSize()
		{
			return BrushSize;
		}
		void Settingsmanager::SetBrushSize(float _BrushSize)
		{
			BrushSize = _BrushSize;
		}
		void Settingsmanager::SetBrushIntensity(float newintensity)
		{
			BrushIntensity = newintensity;
		}
		float Settingsmanager::GetBrushIntensity()
		{
			return BrushIntensity;
		}
		int Settingsmanager::GetBrushShape()
		{
			return Brushshape;
		}
		void Settingsmanager::SetBrushShape(int NewShape)
		{
			Brushshape = NewShape;
		}
		void Settingsmanager::SaveSettings()
		{
		}
		void Settingsmanager::LoadSettings()
		{
		}
	} // editor
} // user
