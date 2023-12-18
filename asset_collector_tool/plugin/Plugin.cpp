// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/plugin/Plugin.h"
#include "asset_collector_tool/view/indicator/AssetCollectorTool.h"
#include "asset_collector_tool/view/indicator/UnitPlacerView.h"
#include <em5/plugin/version/PluginVersion.h>
#include <em5/reflection/CampDefines.h>

#include <qsf_editor/reflection/CampQWidget.h>		// Required, else CAMP will not be able to create instances because the CAMP type "QWidget" will be unknown
#include <qsf_editor/view/ViewManager.h>			// Required, else CAMP will not be able to create instances because the CAMP type "qsf::editor::ViewManager" will be unknown
#include <asset_collector_tool\view\indicator\ScenarioScriptTool.h>
#include <asset_collector_tool/view/indicator/TerrainEditTool.h>
#include <asset_collector_tool\qsf_editor\tools\TerrainEditToolbox.h>
#include <asset_collector_tool\qsf_editor\tools\TerrainpaintingToolbox.h>
#include <asset_collector_tool/view/indicator/TerrainPaintTool.h>
#include <asset_collector_tool/qsf_editor/tools/TerrainEditColorMapToolbox.h>
#include "asset_collector_tool/view/indicator/TerrainEditmodeColorMap.h"
#include <asset_collector_tool\tools\TrainTrackTool.h>
#include <asset_collector_tool\view\DebugUnitView.h>
#include <asset_collector_tool\view\UnitViewer.h>
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		Plugin::Plugin() :
			qsf::Plugin(new em5::PluginVersion())
		{
			// Nothing to do in here
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::Plugin methods                 ]
		//[-------------------------------------------------------]
		bool Plugin::onInstall()
		{
			try
			{
				// Declare CAMP reflection system classes
				// -> Use Qt's "QT_TR_NOOP()"-macro in order to enable Qt's "lupdate"-program to find the internationalization texts
				
				addCampClass(
					camp::Class::declare<AssetCollectorTool>()
					.tag("Name", QT_TR_NOOP("[KC] Asset Collector Tool"))			// Text: "[KC] Asset Collector Tool"
					.tag("Description", QT_TR_NOOP("KC_USEREDITOR_VIEW_AssetCollectorTool_DESCRIPTION"))	// Text: "Indicator browser"
					.tag("Shortcut", "")															// Internal, no translation required
					.base<qsf::editor::View>()
					.constructor2<qsf::editor::ViewManager*, QWidget*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<UnitPlacerView>()
					.tag("Name", QT_TR_NOOP("[KC] UnitPlacerView"))			// Text: "[KC] Asset Collector Tool"
					.tag("Description", QT_TR_NOOP("KC_UnitPlacerView_DESCRIPTION"))	// Text: "Indicator browser"
					.tag("Shortcut", "")															// Internal, no translation required
					.base<qsf::editor::View>()
					.constructor2<qsf::editor::ViewManager*, QWidget*>()
					.getClass()
				);
				addCampClass(
					camp::Class::declare<ScenarioScriptTool>()
					.tag("Name", QT_TR_NOOP("[KC] ScenarioScriptTool"))			// Text: "[KC] Asset Collector Tool"
					.tag("Description", QT_TR_NOOP("ScenarioScriptTool_DESCRIPTION"))	// Text: "Indicator browser"
					.tag("Shortcut", "")															// Internal, no translation required
					.base<qsf::editor::View>()
					.constructor2<qsf::editor::ViewManager*, QWidget*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<TerrainEditTool>()
					.tag("Name", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_NAME21"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::EditMode>()
					.constructor1<qsf::editor::EditModeManager*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<TerrainPaintTool>()
					.tag("Name", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_NAME21"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::EditMode>()
					.constructor1<qsf::editor::EditModeManager*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<TerrainEditmodeColorMap>()
					.tag("Name", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_NAME21"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::EditMode>()
					.constructor1<qsf::editor::EditModeManager*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<TerrainEditToolbox>()
					.tag("Name", QT_TR_NOOP("[KC] Terrain Modelling Tool"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::Tool>()
					.constructor1<qsf::editor::ToolManager*>()
					.getClass()
				);
#ifdef FinalBuild


				addCampClass(
					camp::Class::declare<TerrainpaintingToolbox>()
					.tag("Name", QT_TR_NOOP("[KC] Terrain Texturing Tool"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::Tool>()
					.constructor1<qsf::editor::ToolManager*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<TerrainEditColorMapToolbox>()
					.tag("Name", QT_TR_NOOP("[KC] Terrain Painting Tool"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::Tool>()
					.constructor1<qsf::editor::ToolManager*>()
					.getClass()
				);
#endif // !FinalBuild
				addCampClass(
					camp::Class::declare<TrainTrackTool>()
					.tag("Name", QT_TR_NOOP("[KC] TrainTrackTool"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::Tool>()
					.constructor1<qsf::editor::ToolManager*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<DebugUnitView>()
					.tag("Name", QT_TR_NOOP("[KC] Debug Prefab"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::View>()
					.constructor2<qsf::editor::ViewManager*, QWidget*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<UnitViewer>()
					.tag("Name", QT_TR_NOOP("[KC] Unit Viewer"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::View>()
					.constructor2<qsf::editor::ViewManager*, QWidget*>()
					.getClass()
				);

				//we dont want a log message here because we see this window pretty easy in the editor (or not?)
				// Done
				return true;
			}
			catch (const std::exception& e)
			{
				// Error!
				QSF_ERROR("Failed to install the plugin '" << getName() << "'. Exception caught: " << e.what(), QSF_REACT_NONE);
				return false;
			}
		}

		bool Plugin::onStartup()
		{
			// Nothing to do in here

			// Done
			return true;
		}

		void Plugin::onShutdown()
		{
			// Nothing to do in here
		}

		void Plugin::onUninstall()
		{
			// Removing classes is not possible within the CAMP reflection system

			// Nothing to do in here
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // user
