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
#include <asset_collector_tool\qsf_editor\tools\TerrainTexturingToolbox.h>
#include <asset_collector_tool/view/indicator/TerrainTexturingTool.h>
#include <asset_collector_tool/qsf_editor/tools/TerrainEditColorMapToolbox.h>
#include "asset_collector_tool/view/indicator/TerrainEditmodeColorMap.h"
#include <asset_collector_tool\tools\TrainTrackTool.h>
#include <asset_collector_tool\view\DebugUnitView.h>
#include <asset_collector_tool\view\UnitViewer.h>
#include <asset_collector_tool\\Manager\GuiManager.h>
#include <asset_collector_tool\view\EditorTerrainManager.h>
#include <qsf_editor/EditorHelper.h>
#include <asset_collector_tool\component\EditorToolsHelperComponent.h>
#include <asset_collector_tool\view\KC_AbstractView.h>
#include <asset_collector_tool\kc_terrain\TerrainComponent.h>
#include <qsf/plugin/QsfAssetTypes.h>
#include <qsf/renderer/terrain/TerrainComponent.h>
#include <asset_collector_tool\view\OrderInfoPictureCreator.h>
#include <fstream>
#include <filesystem>
#include <asset_collector_tool\editmode\PlaceUnitEditMode.h>
#include <asset_collector_tool\view\UnoImageWriter.h>
#include <qsf/renderer/component/RendererComponent.h>
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
				QSF_START_CAMP_CLASS_EXPORT(kc_terrain::TerrainComponent, "KC TerrainComponent" ,"Killers first terrain")
#ifndef NoQSFTerrain
					QSF_CAMP_IS_COMPONENT_DERIVED(qsf::TerrainComponent)
#else
					QSF_CAMP_IS_COMPONENT_DERIVED(qsf::RendererComponent)
#endif
					//QSF_ADD_CAMP_PROPERTY(New Global Glossiness, EditorToolsHelperComponent::GetGlobalGlossiness, EditorToolsHelperComponent::SetGlobalGlossiness, "set it directly with the tool", 0.75f)
					QSF_ADD_CAMP_PROPERTY_DIRECT_ACCESS(Do Not Edit above the line,kc_terrain::TerrainComponent::uneditabld,"All values above are from old terrain","------------------------------------").tag("Serializable", false)
					QSF_ADD_CAMP_PROPERTY(New Color Map, kc_terrain::TerrainComponent::GetColorMap, kc_terrain::TerrainComponent::SetNewColorMap, "set it directly with the tool", qsf::getUninitialized<uint64>()).tag("AssetType", qsf::QsfAssetTypes::TEXTURE.getName())
					QSF_ADD_CAMP_PROPERTY(Set Position Offset, kc_terrain::TerrainComponent::getPosition, kc_terrain::TerrainComponent::SetPosition, "set it directly with the tool", glm::vec3())
					QSF_ADD_CAMP_PROPERTY(KC Terrain size, kc_terrain::TerrainComponent::getTerrainWorldSize, kc_terrain::TerrainComponent::setTerrainWorldSize, "set it directly with the tool", 1500)
					QSF_ADD_CAMP_PROPERTY_DIRECT_ACCESS(Automaticly generated, kc_terrain::TerrainComponent::automaticly_created, "All values above are from old terrain", "generated when using terrain edit tools. Used for saving and loading terrains").tag("Serializable", false)
					QSF_ADD_CAMP_PROPERTY(Min Height,kc_terrain::TerrainComponent::GetMinHeight, kc_terrain::TerrainComponent::SetMinHeight,"you should not set sth for now... just for display - maybe later for scaling",0.f)
					QSF_ADD_CAMP_PROPERTY(Max Height, kc_terrain::TerrainComponent::GetMaxHeight, kc_terrain::TerrainComponent::SetMaxHeight, "you should not set sth for now... just for display - maybe later for scaling", 0.f)
					QSF_ADD_CAMP_PROPERTY(Height Map Asset, kc_terrain::TerrainComponent::GetNewHeightMap, kc_terrain::TerrainComponent::SetNewHeightMap, "this points to the heightmap asset. Notice it may not be a dds file because ogre cannot read pixel values of this file. Use *.png or *.tif pls", 0.f).tag("AssetType", qsf::QsfAssetTypes::TEXTURE.getName())
					QSF_ADD_CAMP_PROPERTY(Terrain Texture Asset (1), kc_terrain::TerrainComponent::GetNewTextureMap1_4, kc_terrain::TerrainComponent::SetNewTextureMap1_4, "this points to the texture map asset. It stores the layers 1-4 (0 is not stored and 5 is stored in the next entry).  Notice it may not be a dds file because ogre cannot read pixel values of this file. Use *.png or *.tif pls", 0.f).tag("AssetType", qsf::QsfAssetTypes::TEXTURE.getName())
					QSF_ADD_CAMP_PROPERTY(Terrain Texture Asset (2), kc_terrain::TerrainComponent::GetNewTextureMap5_8, kc_terrain::TerrainComponent::SetNewTextureMap5_8, "this points to the texture map asset. It stores the layers 5 (0 is not stored and 1-4 are stored in the entry before). Notice it may not be a dds file because ogre cannot read pixel values of this file. Use *.png or *.tif pls", 0.f).tag("AssetType", qsf::QsfAssetTypes::TEXTURE.getName())
					QSF_ADD_CAMP_PROPERTY(Terrain Texture Layer List, kc_terrain::TerrainComponent::GetTerrainLayerList, kc_terrain::TerrainComponent::SetTerrainLayerList, "this points to the layer list asset. It stores the names of the layers in a json file", 0.f).tag("AssetType", qsf::QsfAssetTypes::TEXTURE.getName())
					QSF_ADD_CAMP_PROPERTY(Reload, kc_terrain::TerrainComponent::GetUpdate, kc_terrain::TerrainComponent::SetUpdate, "use this after applying new assets - reloads whole terrain. Must be used after changing Chunk Map size or blend/height map size", false).tag("Serializable", false)
					//updating posiition does not work
					//QSF_ADD_CAMP_PROPERTY(Refresh Material only, kc_terrain::TerrainComponent::GetUpdatePosition, kc_terrain::TerrainComponent::UpdatePosition, "refreshs material - can be used if ", false).tag("Serializable", false)
					QSF_ADD_CAMP_PROPERTY(Refresh Material only, kc_terrain::TerrainComponent::GetReloadAllMaterials, kc_terrain::TerrainComponent::SetReloadAllMaterials, "refreshs material - can be used as reload -light- because it does not update Materials but unload and reloads them", false).tag("Serializable", false)
					QSF_ADD_CAMP_PROPERTY(Do Not Load, kc_terrain::TerrainComponent::GetDoNotLoadNextTime, kc_terrain::TerrainComponent::SetDoNotLoadNextTime, "you may force the terrain to not load so you can destroy this object without crash", false)
					QSF_ADD_CAMP_PROPERTY(Blendmapsize , kc_terrain::TerrainComponent::GetBlendtMapSize, kc_terrain::TerrainComponent::SetBlendMapSize, "you can adjust the size of the blendmap here. Must be power of 2 like 2048", 1024)
					QSF_ADD_CAMP_PROPERTY(HeightMapSize, kc_terrain::TerrainComponent::GetHeightMapSize, kc_terrain::TerrainComponent::SetHeightMapSize, "you can adjust the size of the heightmap here. Must be power of 2+1 like 2049", 1025)
					QSF_ADD_CAMP_PROPERTY(KC Chunks per Edge, kc_terrain::TerrainComponent::kc_getTerrainChunksPerEdge,kc_terrain::TerrainComponent::kc_setTerrainChunksPerEdge,"number of chunks per edge e.g. 8 or 16 or 32. Must be lower as height and blendmapsize",16)

					//QSF_ADD_CAMP_PROPERTY(SetEverythingVisible, kc_terrain::TerrainComponent::GetEverythingVisible, kc_terrain::TerrainComponent::SetEverythingVisible, "test", false).tag("Serializable", false)
					QSF_END_CAMP_CLASS_EXPORT

					QSF_START_CAMP_CLASS_EXPORT(EditorToolsHelperComponent, "This sets global glossiness", "you may just attach it to core entity")
					QSF_CAMP_IS_COMPONENT
					QSF_ADD_CAMP_PROPERTY(New Global Glossiness, EditorToolsHelperComponent::GetGlobalGlossiness, EditorToolsHelperComponent::SetGlobalGlossiness, "set it directly with the tool", 0.75f)
					QSF_END_CAMP_CLASS_EXPORT
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO, e.what())
					return false;
			}
			{
			}
			try
			{
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
					camp::Class::declare<OrderInfoPictureCreator>()
					.tag("Name", QT_TR_NOOP("[KC] OrderInfoPictureCreator"))			// Text: "[KC] Asset Collector Tool"
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
					camp::Class::declare<TerrainEditTool>()
					.tag("Name", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_NAME21"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::EditMode>()
					.constructor1<qsf::editor::EditModeManager*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<TerrainTexturingTool>()
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
					camp::Class::declare<PlaceUnitEditMode>()
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

				addCampClass(
					camp::Class::declare<TerrainTexturingToolbox>()
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

				addCampClass(
					camp::Class::declare<kc_terrain::EditorTerrainManager>()
					.tag("Name", QT_TR_NOOP("[KC] EditorTerrainManager"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::View>()
					.constructor2<qsf::editor::ViewManager*, QWidget*>()
					.getClass()
				);

				addCampClass(
					camp::Class::declare<TrainTrackTool>()
					.tag("Name", QT_TR_NOOP("[KC] TrainTrackTool"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::Tool>()
					.constructor1<qsf::editor::ToolManager*>()
					.getClass()
				);

				/*addCampClass(
					camp::Class::declare<DebugUnitView>()
					.tag("Name", QT_TR_NOOP("[KC] Debug Prefab"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<KC_AbstractView>()
					.constructor2<qsf::editor::ViewManager*, QWidget*>()
					.getClass()
				);
				addCampClass(
					camp::Class::declare<ScenarioScriptTool>()
					.tag("Name", QT_TR_NOOP("[KC] ScenarioScriptTool"))			// Text: "[KC] Asset Collector Tool"
					.tag("Description", QT_TR_NOOP("ScenarioScriptTool_DESCRIPTION"))	// Text: "Indicator browser"
					.tag("Shortcut", "")															// Internal, no translation required
					.base<KC_AbstractView>()
					.constructor2<qsf::editor::ViewManager*, QWidget*>()
					.getClass()
				);*/

				addCampClass(
					camp::Class::declare<UnitViewer>()
					.tag("Name", QT_TR_NOOP("[KC] Unit Viewer"))			// Text: "Fire entity"
					.tag("Description", QT_TR_NOOP("ID_EM5EDITOR_EDITMODE_FIRECOMPONENT_DESCRIPTION21"))	// Text: "Fire entity edit mode"
					.base<qsf::editor::View>()
					.constructor2<qsf::editor::ViewManager*, QWidget*>()
					.getClass()
				);


				addCampClass(
					camp::Class::declare<user::editor::UnoImageWriter>()
					.tag("Name", QT_TR_NOOP("[KC] UnoImageWriter"))			// Text: "Fire entity"
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
				QSF_ERROR("Failed to install the plugin  [KC] Editor Tools. Exception caught: " << e.what() << "This message also appears if you play instead of using editor", QSF_REACT_NONE);
				return true;
			}
		}

		bool Plugin::onStartup()
		{
			QSF_LOG_PRINTS(INFO,"startup editor tools")
			//crash :(
			//auto path = std::experimental::filesystem::current_path();
			//std::string path_string{ path.u8string() };
			//Magick::InitializeMagick(path_string.c_str());
			//Magick::InitializeMagick((char *)NULL);
			GUIManager::init();
			// Done
			QSF_LOG_PRINTS(INFO, "startup editor tools finished")
			return true;
		}

		void Plugin::onShutdown()
		{
			GUIManager::instance->~GUIManager();
			QSF_SAFE_DELETE(GUIManager::instance);
			// Nothing to do in here
		}

		void Plugin::onUninstall()
		{
			// Removing classes is not possible within the CAMP reflection system

			// Nothing to do in here
		}

	

		/*playtime 1*/
		/*QSF_LOG_PRINTS(INFO, "plugin startup");
		if (QSF_EDITOR_APPLICATION.getMainWindow() == nullptr)
		QSF_LOG_PRINTS(INFO, "plugin startup failed");
		// Nothing to do in here
		auto Bar2 = QSF_EDITOR_APPLICATION.getMainWindow()->findChildren<QMenuBar *>("");
		for (size_t t = 0; t < Bar2.size(); t++)
		{
		QSF_LOG_PRINTS(INFO, Bar2.at((int)t)->windowTitle().toStdString());
		auto actions = Bar2.at((int)t)->actions();
		for (auto a : actions)
		{
		QSF_LOG_PRINTS(INFO,a->text().toStdString());
		Bar2.at((int)t)->insertSeparator(a);
		}
		}
		QSF_LOG_PRINTS(INFO, "found " << Bar2.size() << " Menu Bars")*/

		/*playtime end*/
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // user
