// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/indicator/TerrainEditTool.h"
#include <asset_collector_tool\qsf_editor\tools\TerrainEditToolbox.h>
#include "ui_TerrainEditTool.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

#include <qsf_editor/operation/utility/RebuildGuiOperation.h>
#include <qsf_editor/application/manager/CameraManager.h>
#include <qsf_editor/EditorHelper.h>

#include <qsf_editor_base/operation/entity/CreateEntityOperation.h>
#include <qsf_editor_base/operation/entity/DestroyEntityOperation.h>
#include <qsf_editor_base/operation/component/CreateComponentOperation.h>
#include <qsf_editor_base/operation/component/DestroyComponentOperation.h>
#include <qsf_editor_base/operation/component/SetComponentPropertyOperation.h>

#include <qsf/map/Map.h>
#include <qsf/map/Entity.h>
#include <qsf/selection/EntitySelectionManager.h>
#include <qsf/QsfHelper.h>
#include <qsf\component\base\MetadataComponent.h>
#include <em5\plugin\Jobs.h>
#include <qsf/debug/request/CircleDebugDrawRequest.h>
#include <qsf/math/CoordinateSystem.h>
#include <qsf/debug/DebugDrawLifetimeData.h>
#include <qsf/input/InputSystem.h>
#include <qsf/input/device/MouseDevice.h>
#include <qsf/map/query/RayMapQuery.h>
#include <qsf/map/query/GroundMapQuery.h>
#include <em5/application/Application.h>
#include "em5/game/groundmap/GroundMaps.h"
#include "qsf/application/WindowApplication.h"
#include "qsf/window/WindowSystem.h"
#include <qsf/window/Window.h>
#include <qsf/renderer/window/RenderWindow.h>
#include <qsf/application/Application.h>
#include <qsf_editor/application/Application.h>
#include <qsf/renderer/RendererSystem.h>
#include <qsf/map/query/ComponentMapQuery.h>
#include <qsf/renderer/component/CameraComponent.h>
#include <qsf/renderer/utility/CameraControlComponent.h>
#include <qsf/renderer/helper/RendererHelper.h>
#include <qsf/component/base/TransformComponent.h>
#include <qsf/math/Random.h>
#include <qsf/map/layer/LayerManager.h>
#include <qsf/map/layer/Layer.h>
#include <qsf/math/EulerAngles.h>
#include <qsf_editor/map/MapHelper.h>
#include <qsf/map/EntityHelper.h>
#include <qsf/selection/SelectionManager.h>
#include <qsf_editor/application/Application.h>
#include <qsf_editor/selection/entity/EntitySelectionManager.h>


#include <qsf_editor_base/user/User.h>
#include <qsf_editor_base/operation/CompoundOperation.h>
#include <qsf_editor_base/operation/entity/CreateEntityOperation.h>
#include <qsf_editor_base/operation/entity/DestroyEntityOperation.h>
#include <qsf_editor_base/operation/layer/CreateLayerOperation.h>
#include <qsf_editor_base/operation/layer/SetLayerPropertyOperation.h>
#include <qsf_editor_base/operation/data/BackupPrototypeOperationData.h>
#include <qsf_editor_base/operation/data/BackupComponentOperationData.h>
#include <qsf_editor_base/operation/component/CreateComponentOperation.h>
#include <qsf_editor_base/operation/component/DestroyComponentOperation.h>
#include <qsf_editor_base/operation/component/SetComponentPropertyOperation.h>
#include <qsf_editor/operation/entity/EntityOperationHelper.h>

#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include "qsf_editor/selection/layer/LayerSelectionManager.h"
#include <qsf/asset/Asset.h>
#include <qsf/prototype/helper/PrefabContent.h>
#include <qsf/prototype/BasePrototypeManager.h>
#include <qsf/input/device/KeyboardDevice.h>

#include <qsf/renderer/mesh/MeshComponent.h>
#include <qsf/renderer/debug/DebugBoxComponent.h>
#include <qsf_editor/editmode/EditMode.h>
#include <qsf_editor/renderer/RenderView.h>
#include "qsf/renderer/window/RenderWindow.h"
#include <qsf/math/Convert.h>
#include <ogre\Ogre.h>
#include <OGRE/OgreRay.h>
#include <qsf/math/Plane.h>
#include <qsf/math/Math.h>
#include <qsf_editor/asset/terrain/TerrainEditManager.h>
#include <qsf\log\LogSystem.h>
#include <../../plugin_api/external/qt/include/QtGui/qevent.h>

#include <ogre\Terrain\OgreTerrainGroup.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <qsf/input/device/KeyboardDevice.h>
#include <qsf/debug/DebugDrawManager.h>

#include <qsf/file/FileSystem.h>
#include <experimental/filesystem> 
#include <boost\filesystem.hpp>
#include <qsf/plugin/QsfAssetTypes.h>
#include <qsf/asset/project/AssetPackage.h>
#include <qsf_editor/asset/import/AssetImportManager.h>
#include <QtWidgets\qinputdialog.h>
#include <windows.h>
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{

		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		const uint32 TerrainEditTool::PLUGINABLE_ID = qsf::StringHash("user::editor::TerrainEditTool");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainEditTool::TerrainEditTool(qsf::editor::EditModeManager* editModeManager) :
			EditMode(editModeManager)
		{
			timer = 0;

		}


		TerrainEditTool::~TerrainEditTool()
		{

		}


		bool TerrainEditTool::evaluateBrushPosition(const QPoint & mousePosition, glm::vec3 & position)
		{
			float closestDistance = -1.0f;
			mTerrainComponent = nullptr;

			// Get the camera component the render window is using
			auto renderWindow = &getRenderView().getRenderWindow();
			auto cameraComponent = renderWindow->getCameraComponent();
			if (nullptr != cameraComponent)
			{
				// Get the normalized mouse position
				const glm::vec2 normalizedPosition = renderWindow->getNormalizedWindowSpaceCoords(mousePosition.x(), mousePosition.y());

				// Get a ray at the given viewport position
				const qsf::Ray ray = cameraComponent->getRayAtViewportPosition(normalizedPosition.x, normalizedPosition.y);
				Ogre::Ray ogreRay = qsf::Convert::getOgreRay(ray);
				for (kc_terrain::TerrainComponent* terrainComponent : qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<kc_terrain::TerrainComponent>())
					//for (qsf::TerrainComponent* terrainComponent : qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<qsf::TerrainComponent>())
				{
					if (terrainComponent->getTerrainHitBoundingBoxByRay(ogreRay))
					{
						// Get terrain intersection
						glm::vec3 hitPosition;
						if (terrainComponent->getTerrainHitByRay(ogreRay, &hitPosition))
						{
							// Get distance to intersection
							const float distance = glm::length(hitPosition - qsf::Convert::getGlmVec3(ogreRay.getOrigin()));
							if (distance < closestDistance || closestDistance < 0.0f)
							{
								closestDistance = distance;
								position = hitPosition;
								mTerrainComponent = terrainComponent;
							}
						}
					}
				}

				if (closestDistance < 0.0f)
				{
					const qsf::Plane plane = qsf::Plane(position, qsf::Math::GLM_VEC3_UNIT_Y);
					qsf::Math::intersectRayWithPlane(ray, plane, &position, nullptr);
					closestDistance = 0.0f;
				}
			}

			if (closestDistance < 0.0f)
			{
				mTerrainComponent = nullptr;
			}
			//skip this!
			if (nullptr != mTerrainComponent)
			{
				/*mTerrainEditHelper = QSF_EDITOR_TERRAINEDIT.findTerrainEditHelper(mTerrainComponent->getEntityId());
				if (nullptr == mTerrainEditHelper || !mTerrainEditHelper->isGood() || !mTerrainEditHelper->isReady())
				{
					mTerrainComponent = nullptr;
				}*/
			}
			else
			{
				// Do not invalidate the terrain edit helper instance in here, we might be in the middle of feeding an operation
				// mTerrainEditHelper = nullptr;
			}
			return (nullptr != mTerrainComponent);
		}






		//[-------------------------------------------------------]
		//[ Protected virtual QWidget methods                     ]
		//[-------------------------------------------------------]


		void TerrainEditTool::PaintJob(const qsf::JobArguments & jobArguments)
		{
			if (QSF_DEBUGDRAW.isRequestIdValid(mDetailViewSingleTrack))
				QSF_DEBUGDRAW.cancelRequest(mDetailViewSingleTrack);
			DebugRequsts.mCircles.clear();
			DebugRequsts.mSegments.clear();
			//mDebugDrawProxy.registerAt(QSF_MAINMAP.getDebugDrawManager());
			if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Quad)
			{
				glm::vec3 up = oldmouspoint + glm::vec3(Radius, 0, Radius);
				glm::vec3 down = oldmouspoint + glm::vec3(-Radius, 0, -Radius);
				DebugRequsts.mSegments.push_back(qsf::SegmentDebugDrawRequest(qsf::Segment::fromTwoPoints(up, glm::vec3(up.x, up.y, down.z)), qsf::Color4::GREEN));
				DebugRequsts.mSegments.push_back(qsf::SegmentDebugDrawRequest(qsf::Segment::fromTwoPoints(up, glm::vec3(down.x, up.y, up.z)), qsf::Color4::GREEN));
				DebugRequsts.mSegments.push_back(qsf::SegmentDebugDrawRequest(qsf::Segment::fromTwoPoints(down, glm::vec3(up.x, up.y, down.z)), qsf::Color4::GREEN));
				DebugRequsts.mSegments.push_back(qsf::SegmentDebugDrawRequest(qsf::Segment::fromTwoPoints(down, glm::vec3(down.x, up.y, up.z)), qsf::Color4::GREEN));
			}
			else
			{
				DebugRequsts.mCircles.push_back(qsf::CircleDebugDrawRequest(oldmouspoint, qsf::CoordinateSystem::getUp(), Radius, qsf::Color4::GREEN));
			}
			mDetailViewSingleTrack = QSF_DEBUGDRAW.requestDraw(DebugRequsts);

			if (!mouseisvalid)
				return;
			if (!QSF_INPUT.getMouse().Left.isPressed())
				return;
			if (QSF_INPUT.getKeyboard().anyControlPressed())
				return;
			glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(oldmouspoint.x, oldmouspoint.z));
			Mappoint = Mappoint*Heighmapsize;
			//QSF_LOG_PRINTS(INFO,"outpoint")
			switch (TerrainEditGUI->GetEditMode()) //Special Handlings
			{
			case 0: //Set
			{
				if (yo_mousepoint == oldmouspoint)
					return;
				yo_mousepoint = oldmouspoint;
				SetHeight(Mappoint);
				return;
			}
			case 1: //Raise
			{
				RaiseTerrain(Mappoint);
				return;
			}
			case 2: //Smooth
			{
				SmoothMap(Mappoint);
				return;
			}
			case 3:
			{
				LowerTerrain(Mappoint);
				return;
			}
			}

		}

		void TerrainEditTool::RaiseTerrain(glm::vec2 MapPoint)
		{

			if (TerrainEditGUI == nullptr)
				return;
			timer++;
			if (timer >= 5)
				timer = 0;
			else
				return;
			float TotalRadius = Radius * Scale;
			float BrushIntensity = TerrainEditGUI->GetBrushIntensity()*0.25f;
			int MapPointMinX = glm::clamp((int)glm::ceil(MapPoint.x - TotalRadius), 0, (int)Heighmapsize);
			int MapPointMaxX = glm::clamp((int)glm::floor(MapPoint.x + TotalRadius), 0, (int)Heighmapsize);
			int MapPointMinY = glm::clamp((int)glm::ceil(MapPoint.y - TotalRadius), 0, (int)Heighmapsize);
			int MapPointMaxY = glm::clamp((int)glm::floor(MapPoint.y + TotalRadius), 0, (int)Heighmapsize);
			//Raise only one point
			if (TerrainEditGUI->GetBrushShape() != TerrainEditGUI->Quad)
			{
				//circle
				for (int t = MapPointMinX; t < MapPointMaxX; t++)
				{
					for (int j = MapPointMinY; j < MapPointMaxY; j++)
					{
						if (TotalRadius > glm::sqrt((t - MapPoint.x)* (t - MapPoint.x) + (j - MapPoint.y) * (j - MapPoint.y)))
						{
							if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Circle)
								RaisePoint(glm::vec2(t, j), BrushIntensity, false);
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Cone)
							{
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = 1.0f - (Distance / TotalRadius);
								RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod, false);
							}
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Dome)
							{
								//intensity formula here
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = glm::cos((Distance / TotalRadius) * glm::pi<float>()) * 0.5f + 0.5f;
								RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod, false);

							}
						}

					}

				}
			}
			else
			{
				//shape square
				for (int t = MapPointMinX; t < MapPointMaxX; t++)
				{
					for (int j = MapPointMinY; j < MapPointMaxY; j++)
					{
						RaisePoint(glm::vec2(t, j), BrushIntensity, false);
					}
				}
			}
			UpdateTerrains();
		}

		void TerrainEditTool::LowerTerrain(glm::vec2 MapPoint)
		{
			if (TerrainEditGUI == nullptr)
				return;
			timer++;
			if (timer >= 5)
				timer = 0;
			else
				return;
			float TotalRadius = Radius * Scale;
			float BrushIntensity = TerrainEditGUI->GetBrushIntensity()*0.25f;
			int MapPointMinX = glm::clamp((int)glm::ceil(MapPoint.x - TotalRadius), 0, (int)Heighmapsize);
			int MapPointMaxX = glm::clamp((int)glm::floor(MapPoint.x + TotalRadius), 0, (int)Heighmapsize);
			int MapPointMinY = glm::clamp((int)glm::ceil(MapPoint.y - TotalRadius), 0, (int)Heighmapsize);
			int MapPointMaxY = glm::clamp((int)glm::floor(MapPoint.y + TotalRadius), 0, (int)Heighmapsize);
			//Raise only one point
			if (TerrainEditGUI->GetBrushShape() != TerrainEditGUI->Quad)
			{
				//circle
				for (int t = MapPointMinX; t < MapPointMaxX; t++)
				{
					for (int j = MapPointMinY; j < MapPointMaxY; j++)
					{
						if (TotalRadius > glm::sqrt((t - MapPoint.x)* (t - MapPoint.x) + (j - MapPoint.y) * (j - MapPoint.y)))
						{
							if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Circle)
								RaisePoint(glm::vec2(t, j), BrushIntensity, true);
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Cone)
							{
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = 1.0f - (Distance / TotalRadius);
								RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod, true);
							}
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Dome)
							{
								//intensity formula here
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = glm::cos((Distance / TotalRadius) * glm::pi<float>()) * 0.5f + 0.5f;
								RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod, true);

							}
						}

					}

				}
			}
			else
			{
				//shape square
				for (int t = MapPointMinX; t < MapPointMaxX; t++)
				{
					for (int j = MapPointMinY; j < MapPointMaxY; j++)
					{
						RaisePoint(glm::vec2(t, j), BrushIntensity, true);
					}
				}
			}
			UpdateTerrains();
		}

		void TerrainEditTool::RaisePoint(glm::vec2 point, float Intensity, bool Decrease)
		{
			int xTerrain = 0;
			int xRemaining = (int)point.x;
			int yTerrain = 0;
			int yRemaining = (int)point.y;
			//QSF_LOG_PRINTS(INFO,point.x << " " << point.y);
			//we have a pattern like 4x4 (so in total 16 Terrains ... now find the correct one)
			//remaining is the point on the selected Terrain
			while (true)
			{
				if ((xRemaining - partsize) >= 0)
				{
					xTerrain++;
					xRemaining = xRemaining - partsize;
				}
				else
					break;
			}
			while (true)
			{
				if ((yRemaining - partsize) >= 0)
				{
					yTerrain++;
					yRemaining = yRemaining - partsize;
				}
				else
					break;
			}
			//QSF_LOG_PRINTS(INFO, xTerrain << " " << yTerrain << " " << xRemaining << " " << yRemaining)
			auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(xTerrain, yTerrain);
			if (Terrain == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Terrain is a nullptr")
					return;
			}
			float newIntensity = glm::clamp(Intensity* 0.5f, 0.2f, 5.f);
			if (Decrease)
				newIntensity = -1.f*newIntensity;
			auto old = Terrain->getHeightAtPoint(xRemaining, yRemaining);
			auto NewHeight = old + newIntensity;
			Terrain->setHeightAtPoint(xRemaining, yRemaining, NewHeight);
			//QSF_LOG_PRINTS(INFO, Terrain->getHeightAtPoint(xRemaining, yRemaining) << " old " << old)
		}


		void TerrainEditTool::SetHeight(glm::vec2 MapPoint)
		{
			float height = 0.1f;
			//OldHeighs.push_back(glm::vec3(point.x, Terrain->getHeightAtPoint(xRemaining, yRemaining), point.y));
			if (TerrainEditGUI != nullptr)
				height = TerrainEditGUI->GetHeight();
			int MapPointMinX = glm::clamp((int)glm::ceil(MapPoint.x - (Radius * Scale)), 0, (int)Heighmapsize);
			int MapPointMaxX = glm::clamp((int)glm::floor(MapPoint.x + (Radius * Scale)), 0, (int)Heighmapsize);
			int MapPointMinY = glm::clamp((int)glm::ceil(MapPoint.y - (Radius * Scale)), 0, (int)Heighmapsize);
			int MapPointMaxY = glm::clamp((int)glm::floor(MapPoint.y + (Radius * Scale)), 0, (int)Heighmapsize);
			if (TerrainEditGUI->GetBrushShape() != TerrainEditGUI->Quad)
			{
				//circle
				for (int t = MapPointMinX; t < MapPointMaxX; t++)
				{
					for (int j = MapPointMinY; j < MapPointMaxY; j++)
					{
						if ((Radius * Scale) > glm::sqrt((t - MapPoint.x)* (t - MapPoint.x) + (j - MapPoint.y) * (j - MapPoint.y)))
							IncreaseHeight(glm::vec2(t, j), height);
					}
				}
			}
			else
			{
				//shape square
				for (int t = MapPointMinX; t < MapPointMaxX; t++)
				{
					for (int j = MapPointMinY; j < MapPointMaxY; j++)
					{
						IncreaseHeight(glm::vec2(t, j), height);
					}
				}
			}
			UpdateTerrains();
		}




		void TerrainEditTool::IncreaseHeight(glm::vec2 point, float NewHeight)
		{
			int xTerrain = 0;
			int xRemaining = (int)point.x;
			int yTerrain = 0;
			int yRemaining = (int)point.y;
			//QSF_LOG_PRINTS(INFO,point.x << " " << point.y);
			//we have a pattern like 4x4 (so in total 16 Terrains ... now find the correct one)
			//remaining is the point on the selected Terrain
			int official_partsize = partsize;

			//xpair and ypair are a lil bit special - let the bitmap be 1025*1025 and with 16 parts
			/*
			each part will have 65 polys. To let them match each other it seems that index 64 and index 0 of the next chunk have to be the same (height). This was the first working attempt to get a map loaded without relicts.
			So there are 16 parts with 65 polys but since they overlap each other you get 1024 + 1 at the end (or beginning) which is not overlapping. Notice if you are at index x= 64 y = 64 you have 4 chunks to feed in all other cases its just 1 or 2
			*/

			
			std::vector<glm::vec2> xPairs;
			std::vector<glm::vec2> yPairs;
			while (true)
			{
				if ((xRemaining - official_partsize) > 0)
				{
					xTerrain++;
					xRemaining = xRemaining - official_partsize;
				}
				else if(xRemaining - official_partsize == 0)
				{
					//we need to edit both terrains
					xPairs.push_back(glm::vec2(xTerrain,xRemaining));
					xPairs.push_back(glm::vec2(xTerrain+1,0));
					break;
				}
				else

					break;
			}
			while (true)
			{
				if ((yRemaining - official_partsize) > 0)
				{
					yTerrain++;
					yRemaining = yRemaining - official_partsize;
				}
				else if (yRemaining - official_partsize == 0)
				{
					//we need to edit both terrains
					yPairs.push_back(glm::vec2(yTerrain,yRemaining));
					yPairs.push_back(glm::vec2( yTerrain + 1,0));
					break;
				}
				else
					break;
			}
			if(xPairs.empty() && yPairs.empty())
			{
			//QSF_LOG_PRINTS(INFO, xTerrain << " " << yTerrain << " " << xRemaining << " " << yRemaining)
			auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(xTerrain, yTerrain);
			if (Terrain == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Terrain is a nullptr" << xTerrain <<" " << yTerrain<< "Remaining x and y "<< xRemaining <<" "<< yRemaining<< " orig point "<< point.x << " " << point.y)
					return;
			}

			Terrain->setHeightAtPoint(xRemaining, yRemaining, NewHeight);//TerrainEditGUI->GetHeight());
			}
			else if(yPairs.empty())
			{
				for (auto a : xPairs)
				{
					auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain((int)a.x, yTerrain);
					if (Terrain == nullptr)
					{
						
						QSF_LOG_PRINTS(INFO, "Terrain is a nullptr [mode 1]" << a.x << " " << yTerrain << "Remaining x and y " << a.y << " " << yRemaining << " orig point " << point.x << " " << point.y)
							continue;
					}
					Terrain->setHeightAtPoint(a.y, yRemaining, NewHeight);//TerrainEditGUI->GetHeight());
				}
			}
			else if (xPairs.empty())
			{
				for (auto a : yPairs)
				{
					auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(xTerrain,(int)a.x);
					if (Terrain == nullptr)
					{
						
						QSF_LOG_PRINTS(INFO, "Terrain is a nullptr [mode 2]" << xTerrain << " " << a.x << "Remaining x and y " << xRemaining << " " << a.y << " orig point " << point.x << " " << point.y)
							continue;
					}
					Terrain->setHeightAtPoint(xRemaining, a.y, NewHeight);//TerrainEditGUI->GetHeight());
				}
			}
			else
			{
				for (auto x_1 : xPairs)
				{
					for (auto y_1 : yPairs)
					{
						auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain((int)x_1.x, (int)y_1.x);
						if (Terrain == nullptr)
						{

							QSF_LOG_PRINTS(INFO, "Terrain is a nullptr [mode 3]" << (int)x_1.x << " " << (int)y_1.x << "Remaining x and y " << x_1.y << " " << y_1.y << " orig point " << point.x << " " << point.y)
								continue;
						}
						Terrain->setHeightAtPoint(x_1.y, y_1.y, NewHeight);//TerrainEditGUI->GetHeight());
					}
				}
			}
			//Terrain->update(false);
		}

		void TerrainEditTool::SmoothMap(glm::vec2 MapPoint)
		{
			if (TerrainEditGUI == nullptr)
				return;
			timer++;
			if (timer >= 5)
				timer = 0;
			else
				return;

			float BrushIntensity = TerrainEditGUI->GetBrushIntensity()*0.25f;
			int MapPointMinX = glm::clamp((int)glm::ceil(MapPoint.x - (Radius * Scale)), 0, (int)Heighmapsize);
			int MapPointMaxX = glm::clamp((int)glm::floor(MapPoint.x + (Radius * Scale)), 0, (int)Heighmapsize);
			int MapPointMinY = glm::clamp((int)glm::ceil(MapPoint.y - (Radius * Scale)), 0, (int)Heighmapsize);
			int MapPointMaxY = glm::clamp((int)glm::floor(MapPoint.y + (Radius * Scale)), 0, (int)Heighmapsize);
			std::vector<glm::vec3> SmoothTerrains;
			if (TerrainEditGUI->GetBrushShape() != TerrainEditGUI->Quad)
			{
				//circle
				for (int t = MapPointMinX; t < MapPointMaxX; t++)
				{
					for (int j = MapPointMinY; j < MapPointMaxY; j++)
					{
						if ((Radius * Scale) > glm::sqrt((t - MapPoint.x)* (t - MapPoint.x) + (j - MapPoint.y) * (j - MapPoint.y)))

						{
							glm::vec3 newIndex = ApplySmooth(glm::vec2(t, j), BrushIntensity);
							if (newIndex != glm::vec3())
								SmoothTerrains.push_back(newIndex);
						}
					}
				}
			}
			else
			{
				//shape square
				for (int t = MapPointMinX; t < MapPointMaxX; t++)
				{
					for (int j = MapPointMinY; j < MapPointMaxY; j++)
					{
						glm::vec3 newIndex = ApplySmooth(glm::vec2(t, j), BrushIntensity);
						if (newIndex != glm::vec3())
							SmoothTerrains.push_back(newIndex);
					}
				}
			}
			for (auto a : SmoothTerrains)
			{
				IncreaseHeight(glm::vec2(a.x, a.y), a.z);
			}
			UpdateTerrains();
		}

		glm::vec3 TerrainEditTool::ApplySmooth(glm::vec2 Point, float Intensity)
		{
			const int smoothWindow = 5;
			int counter = 0;
			float value = 0;
			for (size_t x = Point.x - smoothWindow; x < (Point.x + smoothWindow); x++)
			{
				if (x < 0 || x >= Heighmapsize)
					continue;
				for (size_t y = Point.y - smoothWindow; y < (Point.y + smoothWindow); y++)
				{
					if (y < 0 || y >= Heighmapsize)
						continue;
					value += ReadValue(glm::vec2(x, y));
					counter++;
				}
			}
			if (counter == 0)
				return glm::vec3();
			float Oldvalue = ReadValue(Point);
			float newIntensity = glm::clamp(Intensity* 0.5f, 0.f, 1.f);
			float DifValue = ((value / (float)counter) - Oldvalue) * newIntensity;
			return glm::vec3(Point.x, Point.y, Oldvalue + DifValue);

		}


		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]





		glm::vec3 TerrainEditTool::getPositionUnderMouse()
		{
			glm::vec2 mousePosition = QSF_INPUT.getMouse().getPosition();
			qsf::RayMapQueryResponse response = qsf::RayMapQueryResponse(qsf::RayMapQueryResponse::POSITION_RESPONSE);
			//QSF_WINDOW.getWindows().at(0)->getNativeWindowHandle()
			qsf::ComponentCollection::ComponentList<qsf::CameraControlComponent> QueryFireComponents = qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<qsf::CameraControlComponent>();
			for (auto a : QueryFireComponents)
			{

				auto CC = a->getEntity().getComponent<qsf::CameraComponent>();
				if (CC == nullptr)
					continue;
				if (CC->getRenderWindows().size() == 0)
					continue;
				//this creates a ray which does not have a standart PUBLIC constructor?
				auto pos = CC->getRayAtViewportPosition(mousePosition.x, mousePosition.y);
				qsf::RendererHelper::getViewportRayAtWindowPosition(pos, *CC->getRenderWindows().at(0), mousePosition.x, mousePosition.y);
				float t = a->getTargetPosition().y - pos.getOrigin().y;
				t = t / pos.getDirection().y;
				//QSF_LOG_PRINTS(INFO, t << " " << pos.getPoint(t).x << "  " << pos.getPoint(t).y << "  " << pos.getPoint(t).z)
				return glm::vec3(pos.getPoint(t));

			}
			return glm::vec3(0, 0, 0);
		}





		void TerrainEditTool::SaveMap(const qsf::MessageParameters & parameters)
		{
			SaveTheFuckingMap();
		}

		void TerrainEditTool::SaveTheFuckingMap()
		{
			if (TerrainEditGUI == nullptr)
				return;
			if (TerrainMaster.get() == nullptr)
				return;
			auto path = TerrainEditGUI->GetSavePath();
			//scalemap


			std::string widthandheight = boost::lexical_cast<std::string>(Heighmapsize) + "x" + boost::lexical_cast<std::string>(Heighmapsize);
			Magick::Image* MagImage = new Magick::Image();
			MagImage->size(widthandheight);
			MagImage->magick("TIF");
			MagImage->type(Magick::ImageType::GrayscaleType);

			float lowestpoint = 99999.f;
			float highestpoint = -999999.f;
			for (size_t t = 0; t < Heighmapsize - 1; t++)
			{
				for (size_t j = 0; j < Heighmapsize - 1; j++)
				{
					auto val = ReadValue(glm::vec2(t, j));
					if (lowestpoint > val)
						lowestpoint = val;
					if (highestpoint < val)
						highestpoint = val;
				}
			}
			std::string low = boost::lexical_cast<std::string>(lowestpoint);
			std::string height = boost::lexical_cast<std::string>(highestpoint);
			//highestpoint = highestpoint - lowestpoint;

			/*uint8* buffer = new uint8[Heighmapsize * Heighmapsize * 8];
			Ogre::Image ogreImage;
			ogreImage.loadDynamicImage(buffer, Heighmapsize, Heighmapsize, Ogre::PixelFormat::PF_FLOAT32_GR);
			for (size_t t = 0; t < Heighmapsize - 1; t++)
			{
				for (size_t j = 0; j < Heighmapsize - 1; j++)
				{
					float value = (ReadValue(glm::vec2(t, j)) - lowestpoint) / (highestpoint-lowestpoint);
					//QSF_LOG_PRINTS(INFO,"value" << value << " high " )
					const Ogre::ColourValue ogreColorValue = Ogre::ColourValue(value, value, value);
					ogreImage.setColourAt(ogreColorValue, t, j, 0);
					//float = (Richtiger Punkt - nP) / (hP - np)
					//120 (bei 180 und 90 als Okt)
					//120-90 = 30
					// 30 / (180-90) =1/3
				}
				//ogreImage.setColourAt()
			}
			ogreImage.flipAroundX();*/
			QSF_LOG_PRINTS(INFO, "Heightmap channelcount" << MagImage->channels() << Heighmapsize);
			MagickCore::Quantum*  Pixels = MagImage->getPixels(0, 0, Heighmapsize, Heighmapsize);
			for (size_t x = 0; x < Heighmapsize; x++)
			{
				for (size_t y = 0; y < Heighmapsize; y++)
				{
					float value = (ReadValue(glm::vec2(x, y)) - lowestpoint) / (highestpoint - lowestpoint);
					uint64 offset = (y* Heighmapsize + x) * MagImage->channels(); //4 is because we use 4 channels
					*(Pixels + offset) = value * 65535.f;
				}
			}

			MagImage->flip();
			MagImage->syncPixels();

			uint64 globalMapAssetId = QSF_MAINMAP.getGlobalAssetId();
			/*if (qsf::AssetProxy(globalMapAssetId).getGlobalAssetId() == qsf::getUninitialized<qsf::GlobalAssetId>())
			{
				ogreImage.save(path+"\\heightmap___min__" + low + "__max__" + height + "__date__"+GetCurrentTimeForFileName()+".tif");
			}
			else
			{
				auto mapname = qsf::AssetProxy(globalMapAssetId).getLocalAssetName();
				std::vector<std::string> splittedString;
				boost::split(splittedString, mapname, boost::is_any_of("/"), boost::token_compress_on);
				if (!splittedString.empty())
				{
					QSF_LOG_PRINTS(INFO, "Mapname " << splittedString.at(splittedString.size()-1));
					ogreImage.save(path + "\\heightmap__"+splittedString.at(splittedString.size()-1)+"__min__" + low + "__max__" + height + "__date__" + GetCurrentTimeForFileName() + ".tif");
				}
				else
					ogreImage.save(path + "\\heightmap___min__" + low + "__max__" + height + "__date__" + GetCurrentTimeForFileName() + ".tif");
			}*/
			//put it into our project
			mAssetEditHelper = std::shared_ptr<qsf::editor::AssetEditHelper>(new qsf::editor::AssetEditHelper());
			auto IAP = mAssetEditHelper->getIntermediateAssetPackage();
			if (TerrainMaster->GetNewHeightMap().getAsset() == nullptr)
			{

				//QSF_LOG_PRINTS(INFO, "asset not found " << LocalAssetName)
				try
				{
					//a add asset
					auto TimeStamp = GetCurrentTimeForFileName();
					auto Name = TerrainMaster->getEntity().getComponent<qsf::MetadataComponent>()->getName();


					//b create folder structure in assethelper (to read and write)
					if (!boost::filesystem::exists((QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap")))
						boost::filesystem::create_directories(QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap");
					//c write to new folder -I think it will copy to our direction we wrote before
					//ogreImage.save(QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap/heightmap_" + Name + "_" + TimeStamp +".tif");
					auto relAssetDirectory = QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackage()->getRelativeDirectory();
					if (!boost::filesystem::exists((QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/heightmap")))
						boost::filesystem::create_directories(QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/heightmap");



					//MagImage->write(QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/heightmap/heightmap_" + Name  +  ".tif");
					QSF_LOG_PRINTS(INFO, "saved ogre img to " << QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap/heightmap_" + Name + ".tif")
						//d learn our assets a few thing <-> this seems not needed
						//delete[] buffer;

						auto Asset = mAssetEditHelper->addAsset(QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackage()->getName(), qsf::QsfAssetTypes::TEXTURE, "heightmap", "heightmap_" + Name);
					MagImage->write(QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap/heightmap_" + Name + ".tif");
					if (Asset == nullptr)
						QSF_LOG_PRINTS(INFO, "error occured " << Name << " could not create an asset")
					else
					{
						auto CachedAsset = mAssetEditHelper->getCachedAsset(Asset->getGlobalAssetId());
						if (CachedAsset == nullptr)
							CachedAsset = &qsf::CachedAsset(Asset->getGlobalAssetId());
						if (CachedAsset == nullptr)
						{
							QSF_LOG_PRINTS(INFO, "still a nullptr")
						}
						CachedAsset->setType("tif");
						//QSF_LOG_PRINTS(INFO, qsf::AssetProxy(Asset->getGlobalAssetId()).getAbsoluteCachedAssetDataFilename())

						if (mAssetEditHelper->setAssetUploadData(Asset->getGlobalAssetId(), true, true))
							QSF_LOG_PRINTS(INFO, "Caching asset was not succesfull")
					}
					TerrainMaster->SetNewHeightMap(qsf::AssetProxy(Asset->getGlobalAssetId()));
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO, e.what())
				}

			}
			else //tell them that asset was changed :)
			{

				//QSF_LOG_PRINTS(INFO,"for some reasons it exists")
				//ogreImage.save(TerrainMaster->GetNewHeightMap().getAbsoluteCachedAssetDataFilename());
				MagImage->write(TerrainMaster->GetNewHeightMap().getAbsoluteCachedAssetDataFilename());
				std::string TargetAssetName = qsf::AssetProxy(TerrainMaster->GetNewHeightMap()).getAssetPackage()->getName();
				mAssetEditHelper->tryEditAsset(qsf::AssetProxy(TerrainMaster->GetNewHeightMap()).getGlobalAssetId(), TargetAssetName);
				auto CachedAsset = mAssetEditHelper->getCachedAsset(qsf::AssetProxy(TerrainMaster->GetNewHeightMap()).getAsset()->getGlobalAssetId());
				/*if (CachedAsset == nullptr)
				QSF_LOG_PRINTS(INFO, "cached asset is a nullptr")
				else
				QSF_LOG_PRINTS(INFO, "cached asset isnot null?")*/
				mAssetEditHelper->setAssetUploadData(qsf::AssetProxy(TerrainMaster->GetNewHeightMap()).getAsset()->getGlobalAssetId(), true, true);
				//delete[] buffer;
			}

			// Cleanup

			QSF_LOG_PRINTS(INFO, "Map was saved succesfully")
				mAssetEditHelper->submit();
			mAssetEditHelper->callWhenFinishedUploading(boost::bind(&TerrainEditTool::WaitForSaveTerrain, this, boost::function<void(bool)>(boost::bind(&TerrainEditTool::onWaitForSave_TerrainDone, this, _1))));
			TerrainMaster->SetMaxHeight(highestpoint);
			TerrainMaster->SetMinHeight(lowestpoint);
			TerrainMaster->setAllPropertyOverrideFlags(true);
		}

		void TerrainEditTool::WaitForSaveTerrain(boost::function<void(bool)> resultCallback)
		{
		}

		void TerrainEditTool::onWaitForSave_TerrainDone(bool isGood)
		{
			/*QSF_LOG_PRINTS(INFO, "Created Splitmaps func 2")
			QSF_CHECK(nullptr != mAssetEditHelper, "qsf::editor::TerrainEditHelper::onSaveTerrainDone(): nullptr != mAssetEditHelper", QSF_REACT_THROW);
			mAssetEditHelper->reset();
			QSF_EDITOR_APPLICATION.getAssetImportManager().setDefaultDestinationAssetPackage(m_OldAssetPackage->getProject().getName(), m_OldAssetPackage->getName());
			m_OldAssetPackage = nullptr;*/
		}


		void TerrainEditTool::CopyFromQSFMap(const qsf::MessageParameters & parameters)
		{

			QSF_LOG_PRINTS(INFO, "Copy QSF Terrain started")
				//get old qsf terrain
				qsf::TerrainComponent* CopyFromTerrain = nullptr;
			for (qsf::TerrainComponent* terrainComponent : qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<qsf::TerrainComponent>())
			{
				if (terrainComponent->getEntity().getComponent<kc_terrain::TerrainComponent>() == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "found a old qsf terrain")
						CopyFromTerrain = terrainComponent;
					break;
				}
			}
			if (CopyFromTerrain == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "No QSF Terrain found")
					return;
			}
			int CopyFromHeightMapSize = CopyFromTerrain->getHeightMapSize();
			if (CopyFromHeightMapSize != TerrainMaster->getHeightMapSize())
			{
				QSF_LOG_PRINTS(INFO, "Cant do that partsize does not match. Source has " << CopyFromHeightMapSize << " and Target has " << TerrainMaster->getHeightMapSize())
					return;
			}
			auto it = CopyFromTerrain->getOgreTerrainGroup()->getTerrainIterator();
			int counter = 0; // because my ID start at 0
			while (it.hasMoreElements()) // add the layer to all terrains in the terrainGroup
			{
				Ogre::TerrainGroup::TerrainSlot* a = it.getNext();
				counter++;
			}
			int CopyFromParts = CopyFromTerrain->getOgreTerrainGroup()->getTerrainSize();
			int OrigParts = CopyFromTerrain->getOgreTerrainGroup()->getTerrainSize();
			if (CopyFromParts != OrigParts)
			{
				QSF_LOG_PRINTS(INFO, "Cant do that number of parts not matching. Source has " << CopyFromParts << " and Target has " << OrigParts)
					return;
			}
			auto it_source = CopyFromTerrain->getOgreTerrainGroup()->getTerrainIterator();
			auto it_target = TerrainMaster->getOgreTerrainGroup()->getTerrainIterator();

			while (it_source.hasMoreElements()) // add the layer to all terrains in the terrainGroup
			{
				Ogre::TerrainGroup::TerrainSlot* TS_source = it_source.getNext();
				Ogre::TerrainGroup::TerrainSlot* TS_target = it_target.getNext();

				for (size_t x = 0; x < partsize; x++)
				{
					for (size_t y = 0; y < partsize; y++)
					{
						TS_target->instance->setHeightAtPoint((long)x, (long)y, TS_source->instance->getHeightAtPoint((long)x, (long)y));
					}
				}
				TS_target->instance->update();
			}

		}

		void TerrainEditTool::LoadMap(std::string realfilename, std::string realfilepath)
		{
			auto filename = realfilename;
			auto pathcopy = realfilepath;
			float lowestpoint = 0.f;
			float highestpoint = 0.f;
			if (TerrainEditGUI == nullptr)
				return;
			if (TerrainMaster.get() == nullptr)
				return;
			auto path = TerrainEditGUI->GetSavePath();
			//scalemap
			QSF_LOG_PRINTS(INFO, "load heightmap" << filename);
			auto id = filename.find("__min__");
			if (std::string::npos == id)
			{
				bool ok;
				QString text = QInputDialog::getText(0, "Could not find Min Height",
					"Min Height", QLineEdit::Normal,
					"", &ok);
				if (ok && !text.isEmpty()) {
					try
					{
						lowestpoint = boost::lexical_cast<float>(text.toStdString());
					}
					catch (const std::exception& e)
					{
						QSF_LOG_PRINTS(INFO, e.what())
							return;
					}
				}
				else
				{
					return;
				}

				text = QInputDialog::getText(0, "Could not find Max Height",
					"Max Height", QLineEdit::Normal,
					"", &ok);
				if (ok && !text.isEmpty()) {
					try
					{
						highestpoint = boost::lexical_cast<float>(text.toStdString());
					}
					catch (const std::exception& e)
					{
						QSF_LOG_PRINTS(INFO, e.what())
							return;
					}
				}
				else
				{
					return;

				}
				if (lowestpoint >= highestpoint)
				{
					QSF_LOG_PRINTS(INFO,"error lowestpoint and highestpoint are same level")
					return;
				}
			}
			if (lowestpoint == 0 && highestpoint == 0)
			{
				filename.erase(0, id + 7);
				id = filename.find("__max__");
				if (std::string::npos == id)
					return;
				std::string min, max;
				min = filename;
				min = min.erase(id, min.size() - 1);
				filename.erase(0, id + 7);
				max = filename;
				id = filename.find("__date__");
				if (std::string::npos == id)
					return;
				max = max.erase(id, max.size() - 1);

				lowestpoint = boost::lexical_cast<float>(min);
				highestpoint = boost::lexical_cast<float>(max);
			}
			QSF_LOG_PRINTS(INFO, highestpoint);
			QSF_LOG_PRINTS(INFO, lowestpoint);
			Ogre::String Heightmaps = "FileSystem";
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(pathcopy.c_str(), Heightmaps);
			Ogre::Image OI;
			OI.load(Ogre::String(realfilename).c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			if (&OI == nullptr)
			{
				QSF_LOG_PRINTS(ERROR, "sth went wrong with importing texture")
					return;
			}
			auto width = OI.getHeight();
			auto height = OI.getWidth();

			OI.flipAroundX();
			QSF_LOG_PRINTS(INFO, "Reshaping the world");
			for (size_t t = 0; t < height - 1; t++) //improve height map check?
			{
				if (t > Heighmapsize)
					continue;
				for (size_t j = 0; j < width - 1; j++)
				{
					if (j > Heighmapsize)
						continue;
					auto val = OI.getColourAt(t, j, 0).r;
					//Punktval = (Richtiger Punkt - nP) / (hP - np)  
					//Richtiger Punkkt = Punktval *(hP-nP)+nP
					float RealPointHeight = val * (highestpoint - lowestpoint) + lowestpoint;
					IncreaseHeight(glm::vec2(t, j), RealPointHeight);
					if(RealPointHeight < 20)
					QSF_LOG_PRINTS(INFO,t << " j "<< j << " "<< RealPointHeight)
				}
				//ogreImage.setColourAt()
			}
			QSF_LOG_PRINTS(INFO, "Done with reshaping the world");
			UpdateTerrains();

		}

		std::string TerrainEditTool::GetCurrentTimeForFileName()
		{
			auto time = std::time(nullptr);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&time), "%F_%T"); // ISO 8601 without timezone information.
			auto s = ss.str();
			std::replace(s.begin(), s.end(), ':', '-');
			return s;
		}

		float TerrainEditTool::ReadValue(glm::vec2 point)
		{
			int xTerrain = 0;
			int xRemaining = (int)point.x;
			int yTerrain = 0;
			int yRemaining = (int)point.y;
			//QSF_LOG_PRINTS(INFO,point.x << " " << point.y);
			//we have a pattern like 4x4 (so in total 16 Terrains ... now find the correct one)
			//remaining is the point on the selected Terrain
			while (true)
			{
				if ((xRemaining - partsize) >= 0)
				{
					xTerrain++;
					xRemaining = xRemaining - partsize;
				}
				else
					break;
			}
			while (true)
			{
				if ((yRemaining - partsize) >= 0)
				{
					yTerrain++;
					yRemaining = yRemaining - partsize;
				}
				else
					break;
			}
			//QSF_LOG_PRINTS(INFO, xTerrain << " " << yTerrain << " " << xRemaining << " " << yRemaining)
			auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(xTerrain, yTerrain);
			if (Terrain == nullptr)
			{

				QSF_LOG_PRINTS(INFO, "Terrain is a nullptr" << xTerrain << " " << yTerrain)
					return 0.f;
			}
			return Terrain->getHeightAtPoint(xRemaining, yRemaining);//TerrainEditGUI->GetHeight());
		}

		inline void TerrainEditTool::mousePressEvent(QMouseEvent & qMouseEvent)
		{
			if (Qt::LeftButton == qMouseEvent.button() && TerrainEditGUI != nullptr && TerrainEditGUI->GetEditMode() == TerrainEditGUI->Set) //copy height with ctrl + left
			{
				if (QSF_INPUT.getKeyboard().anyControlPressed())
				{
					glm::vec3 mousepos2;
					if (evaluateBrushPosition(qMouseEvent.pos(), mousepos2))
					{
						glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(mousepos2.x, mousepos2.z));
						Mappoint = Mappoint*Heighmapsize;
						float value = ReadValue(glm::vec2(glm::round(Mappoint.x), glm::round(Mappoint.y)));
						TerrainEditGUI->SetHeight(value);
						return;
					}
					return;
				}
			}
			if (Qt::LeftButton != qMouseEvent.button()) //only left button
				return;

			glm::vec3 mousepos2;
			if (evaluateBrushPosition(qMouseEvent.pos(), mousepos2))
			{
				//QSF_LOG_PRINTS(INFO,mousepos2);
				oldmouspoint = mousepos2;
				mouseisvalid = true;

				if (TerrainEditGUI != nullptr)
					Radius = TerrainEditGUI->GetBrushRadius();
				//QSF_LOG_PRINTS(INFO, "Radius is" << Radius)
				return;

			}
			else
				//QSF_LOG_PRINTS(INFO,"invalid");
				mouseisvalid = false;

			//TerrainEditGUI.set(TET);
			if (TerrainEditGUI != nullptr)
				Radius = TerrainEditGUI->GetBrushRadius();
			//QSF_LOG_PRINTS(INFO,"Radius is"<< Radius)
		}

		inline void TerrainEditTool::mouseMoveEvent(QMouseEvent & qMouseEvent)
		{
			glm::vec3 mousepos2;
			if (TerrainEditGUI != nullptr)
				Radius = TerrainEditGUI->GetBrushRadius();
			if (evaluateBrushPosition(qMouseEvent.pos(), mousepos2))
			{
				//QSF_LOG_PRINTS(INFO, mousepos2);
				oldmouspoint = mousepos2;
				mouseisvalid = true;

				//QSF_LOG_PRINTS(INFO, "Radius is" << Radius)
				return;
			}
			else
				//QSF_LOG_PRINTS(INFO, "invalid");
				mouseisvalid = false;
		}

		glm::vec2 TerrainEditTool::ConvertWorldPointToRelativePoint(glm::vec2 WorldPoint)
		{
			glm::vec2 copy = WorldPoint;
			qsf::TransformComponent* TC = TerrainMaster->getEntity().getComponent<qsf::TransformComponent>();
			glm::vec3 OffsetPos = /*TC->getRotation()**/TC->getPosition();
			copy = copy - glm::vec2(OffsetPos.x, OffsetPos.z);
			copy = (copy + Offset) / TerrainMaster->getTerrainWorldSize();
			copy.y = 1.f - copy.y; //we need to mirror Y
			return copy;
		}

		glm::vec2 TerrainEditTool::ConvertMappointToWorldPoint(glm::vec2 Mappoint)
		{
			glm::vec2 copy = Mappoint;
			copy = copy / (float)Heighmapsize;
			copy = copy * TerrainMaster->getTerrainWorldSize();
			copy = copy - Offset;
			copy.y = -1.f * copy.y;
			return copy;
		}

		void TerrainEditTool::UpdateTerrains()
		{
			//we should make sure that we have a 4 x 4 Terrain-pattern (16 tiles).
			//we assume it here by t = x and y = i
			for (long t = 0; t <= (mParts - 1); t++)
			{
				for (long i = 0; i <= (mParts - 1); i++)
				{
					//QSF_LOG_PRINTS(INFO, " t " << t << " i " << i)
					if (TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i) != nullptr)
					{
						TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i)->update(true);
					}

				}

			}
		}




		bool TerrainEditTool::onStartup(EditMode * previousEditMode)
		{


			//prevent crashs if terrain is not there yet
			if (qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<kc_terrain::TerrainComponent>() == nullptr)
				return false;
			TerrainMaster = qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<kc_terrain::TerrainComponent>();
			if (!TerrainMaster.valid())
				return false;
			if (TerrainMaster->getOgreTerrainGroup() == nullptr)
			{
				TerrainMaster.set(nullptr);
				return false;
			}
			//auto terrain = TES.at(0)->getOgreTerrain();
			QSF_LOG_PRINTS(INFO, TerrainMaster->getTerrainWorldSize());
			QSF_LOG_PRINTS(INFO, TerrainMaster->getHeightMapSize());
			QSF_LOG_PRINTS(INFO, "partsize" << TerrainMaster->getOgreTerrainGroup()->getTerrainSize());

			Offset = (float)(TerrainMaster->getTerrainWorldSize() / 2);
			percentage = 1.f / (float)TerrainMaster->getTerrainWorldSize(); /// (float)TerrainMaster->getHeightMapSize();
			Heighmapsize = (float)TerrainMaster->getHeightMapSize();
			partsize = TerrainMaster->getOgreTerrainGroup()->getTerrainSize() - 1;

			if (!PaintJobProxy.isValid())
				PaintJobProxy.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&TerrainEditTool::PaintJob, this, _1));
			//PaintJobProxy.changeTimeBetweenCalls(qsf::Time::fromMilliseconds(50.f));
			QSF_LOG_PRINTS(INFO, "scale" << Heighmapsize / TerrainMaster->getTerrainWorldSize() << " units per meter");
			Scale = Heighmapsize / TerrainMaster->getTerrainWorldSize();
			mParts = floor(Heighmapsize / (partsize - 1.f));
			QSF_LOG_PRINTS(INFO, "we have " << mParts << " x " << mParts << "  Parts");
			EditMode::onStartup(previousEditMode);

			user::editor::TerrainEditToolbox* TET = static_cast<user::editor::TerrainEditToolbox*>(this->getManager().getToolWhichSelectedEditMode());
			if (TET == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "TET is a nullptr");
				return false;
			}
			TerrainEditGUI = TET;
			mSaveMapProxy.registerAt(qsf::MessageConfiguration(qsf::MessageConfiguration("kc::save_heightmap")), boost::bind(&TerrainEditTool::SaveMap, this, _1));
			mCopyFromQSFMap.registerAt(qsf::MessageConfiguration(qsf::MessageConfiguration("kc::copy_heightmap")), boost::bind(&TerrainEditTool::CopyFromQSFMap, this, _1));
			//LoadMap("heightmap__beaverfield__min__20__max__143.136826__date__2023-10-14_13-46-59.tif");
			return true;
		}

		void TerrainEditTool::onShutdown(EditMode * nextEditMode)
		{
			UpdateTerrains();
			EditMode::onShutdown(nextEditMode);
			mSaveMapProxy.unregister();
			PaintJobProxy.unregister();
			if (QSF_DEBUGDRAW.isRequestIdValid(mDetailViewSingleTrack))
				QSF_DEBUGDRAW.cancelRequest(mDetailViewSingleTrack);
			SaveTheFuckingMap();
			QSF_LOG_PRINTS(INFO, "Shutdown")
		}



		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
