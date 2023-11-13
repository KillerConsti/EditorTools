// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"


// Is this project built as shared library?
#ifdef _USRDLL

	//[-------------------------------------------------------]
	//[ Includes                                              ]
	//[-------------------------------------------------------]
	#include "asset_collector_tool/Export.h"
	#include "asset_collector_tool/plugin/Plugin.h"


	//[-------------------------------------------------------]
	//[ Global functions                                      ]
	//[-------------------------------------------------------]
	EDITORPLUGIN_FUNCTION_EXPORT qsf::Plugin* CreatePluginInstance_asset_collector_tool()
	{
		return new user::editor::Plugin();
	}

#endif
