// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/qsf_editor/asset/terrain/TerrainEditHelper.h"
#include "qsf_editor/asset/import/AssetPackageImportHelper.h"
#include "qsf_editor/asset/AssetEditHelper.h"
#include "qsf_editor/asset/AssetStorageManager.h"
#include "qsf_editor/selection/entity/EntitySelectionManager.h"
#include "qsf_editor/settings/EditorSettingsGroup.h"
#include "qsf_editor/EditorHelper.h"

#include <qsf_editor_base/asset/compiler/TextureAssetCompiler.h>
#include <qsf_editor_base/operation/component/SetComponentPropertyOperation.h>
#include <qsf_editor_base/operation/entity/SelectEntityHiddenOperation.h>
#include <qsf_editor_base/operation/entity/DeselectEntityHiddenOperation.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include <qsf_editor_base/operation/CommitOperation.h>

#include <qsf/asset/Asset.h>
#include <qsf/asset/AssetProxy.h>
#include <qsf/asset/AssetSystem.h>
#include <qsf/asset/project/AssetPackage.h>
#include <qsf/asset/project/Project.h>
#include <qsf/file/FileSystem.h>
#include <qsf/map/Map.h>
#include <qsf/map/Entity.h>
#include <qsf/math/Math.h>
#include <qsf/plugin/QsfAssetTypes.h>
#include <qsf/renderer/terrain/TerrainComponent.h>
#include <qsf/renderer/terrain/TerrainDefinition.h>
#include <qsf/component/base/TransformComponent.h>
#include <qsf/QsfHelper.h>

#include <boost/filesystem.hpp>

#include <OGRE/Terrain/OgreTerrainGroup.h>

#include <sstream>
#include <iomanip>

#undef ERROR
//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
namespace
{
	namespace detail
	{


		

//[-------------------------------------------------------]
//[ Anonymous detail namespace                            ]
//[-------------------------------------------------------]
	} // detail
}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace qsf
{
	namespace editor2
	{


		


		TerrainEditHelper::~TerrainEditHelper()
		{
		}

		//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // qsf
