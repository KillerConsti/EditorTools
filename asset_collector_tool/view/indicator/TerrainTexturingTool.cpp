// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/indicator/TerrainTexturingTool.h"
#include <asset_collector_tool\qsf_editor\tools\TerrainTexturingToolbox.h>

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

#include <boost\property_tree\ptree.hpp>
#include <qsf\file\FileHelper.h>
#include <qsf\file\FileSystem.h>
#include "qsf/asset/Asset.h"
#include "qsf/asset/AssetSystem.h"

#include <qsf/renderer/RendererSystem.h>


#include "qsf/renderer/material/material/MaterialManager.h"
#include "qsf/renderer/material/MaterialSystem.h"
#include <qsf/renderer/terrain/TerrainDefinition.h>
#include <qsf/renderer/material/material/MaterialVariationManager.h>
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
		const uint32 TerrainTexturingTool::PLUGINABLE_ID = qsf::StringHash("user::editor::TerrainTexturingTool");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainTexturingTool::TerrainTexturingTool(qsf::editor::EditModeManager* editModeManager) :
			EditMode(editModeManager)
		{
			timer = 0;
			mSelectedLayerColor = qsf::getUninitialized<uint64>();
		}


		TerrainTexturingTool::~TerrainTexturingTool()
		{
		}


		bool TerrainTexturingTool::evaluateBrushPosition(const QPoint & mousePosition, glm::vec3 & position)
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


		void TerrainTexturingTool::PaintJob(const qsf::JobArguments & jobArguments)
		{
			GetSelectedLayerColor();
			mDebugDrawProxy.registerAt(QSF_MAINMAP.getDebugDrawManager());
			mDebugDrawProxy.addRequest(qsf::CircleDebugDrawRequest(oldmouspoint, qsf::CoordinateSystem::getUp(), Radius, qsf::Color4::GREEN));
			if (!mouseisvalid)
				return;

			if (!QSF_INPUT.getMouse().Left.isPressed())
				return;
			if (yo_mousepoint == oldmouspoint)
				return;
			yo_mousepoint = oldmouspoint;
			glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(oldmouspoint.x, oldmouspoint.z));
			Mappoint = Mappoint*Heighmapsize;
			RaiseTerrain(Mappoint);

		}

		float TerrainTexturingTool::GetCustomIntensity(float distancetoMidpoint, TerrainTexturingToolbox::TerrainEditMode2 Mode)
		{
			return 0.0f;
		}

		void TerrainTexturingTool::RaiseTerrain(glm::vec2 MapPoint)
		{
			//QSF_LOG_PRINTS(INFO, "Raise Terrain")
			if (TerrainEditGUI == nullptr)
				return;
			//QSF_LOG_PRINTS(INFO,"aha 1")
			float TotalRadius = Radius * Scale;
			float BrushIntensity = TerrainEditGUI->GetBrushIntensity()*0.25f;
			//QSF_LOG_PRINTS(INFO,"Mappoint "<< MapPoint.x <<" y " << MapPoint.y)
			int MapPointMinX = glm::clamp((int)glm::ceil(MapPoint.x - TotalRadius), 0, (int)Heighmapsize);
			int MapPointMaxX = glm::clamp((int)glm::floor(MapPoint.x + TotalRadius), 0, (int)Heighmapsize);
			int MapPointMinY = glm::clamp((int)glm::ceil(MapPoint.y - TotalRadius), 0, (int)Heighmapsize);
			int MapPointMaxY = glm::clamp((int)glm::floor(MapPoint.y + TotalRadius), 0, (int)Heighmapsize);
			int counter = 0;
			//QSF_LOG_PRINTS(INFO, "aha 2")
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
							{
								RaisePoint(glm::vec2(t, j), BrushIntensity);
								counter++;
								//break;
							}
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Cone)
							{
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = 1.0f - (Distance / TotalRadius);
								RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod);
								counter++;
								//break;
							}
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Dome)
							{
								//intensity formula here
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = glm::cos((Distance / TotalRadius) * glm::pi<float>()) * 0.5f + 0.5f;
								RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod);
								counter++;
								//break;

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
						RaisePoint(glm::vec2(t, j), BrushIntensity);
						counter++;
						//break;
					}
				}
			}
			//QSF_LOG_PRINTS(INFO, "RaisePoint update Terrain")
			RaisePoint(glm::vec2(1, 1), BrushIntensity);
			UpdateTerrains();
			//QSF_LOG_PRINTS(INFO, "RaisePoint Terrain 2")
			//QSF_LOG_PRINTS(INFO, "counter " << counter)

		}

		void TerrainTexturingTool::RaisePoint(glm::vec2 point, float Intensity)
		{
			QSF_LOG_PRINTS(INFO,"Raise Point 1")
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
			yRemaining = partsize - yRemaining;
			AffectedPoints[xTerrain][yTerrain].push_back(glm::vec2(xRemaining, yRemaining));
			//QSF_LOG_PRINTS(INFO, xTerrain << " " << yTerrain << " " << xRemaining << " " << yRemaining)

			auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(xTerrain, yTerrain);


			QSF_LOG_PRINTS(INFO, "Raise Point 2")
			//here was sth
		//we meed to mirror x for some unknown reason. Maybe this map is also "mirrored"
			QSF_LOG_PRINTS(INFO, "get blend map index")
		//now apply correct blendmap
			int BlendMapIndex = GetBlendMapWithTextureName(xTerrain, yTerrain);
			if (BlendMapIndex == -1)
				return;

			const uint32 maximumNumberOfLayers = TMG_getMaxLayers(Terrain);
			const uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(Terrain->getBlendTextureCount()));
			QSF_LOG_PRINTS(INFO, "numberOfLayers " << numberOfLayers << "a123 " << BlendMapIndex << " Blendtexturecount "<< Terrain->getBlendTextureCount())
				for (uint32 layerIndex = 1; layerIndex < 6; ++layerIndex)
				{
					QSF_LOG_PRINTS(INFO,"layerIndex" << layerIndex)
					if (Terrain->getLayerBlendMap(layerIndex) == nullptr)
						continue;
						if (layerIndex == BlendMapIndex)
						{
							//QSF_LOG_PRINTS(INFO, "found the layer")
							QSF_LOG_PRINTS(INFO, "layerIndex == BlendMapIndex and now change sth"<< layerIndex)
								Terrain->getLayerBlendMap(layerIndex)->setBlendValue(xRemaining, yRemaining, 1);
							//QSF_LOG_PRINTS(INFO, "match" << layerIndex)
						}
						else
						{
							QSF_LOG_PRINTS(INFO, "layerIndex == BlendMapIndex and now change reset it..." << layerIndex)
							Terrain->getLayerBlendMap(layerIndex)->setBlendValue(xRemaining, yRemaining, 0);
						}
				}

			/*

			ogreTerrain->getLayerBlendMap(1)->setBlendValue(xRemaining, yRemaining, 0);
			ogreTerrain->getLayerBlendMap(2)->setBlendValue(xRemaining, yRemaining, 0);
			ogreTerrain->getLayerBlendMap(3)->setBlendValue(xRemaining, yRemaining, 1);
			//QSF_LOG_PRINTS(INFO,ogreTerrain->getLayerBlendMapSize() << " x " <<xRemaining << " y " << yRemaining);
			ogreTerrain->getLayerBlendMap(4)->setBlendValue(xRemaining, yRemaining, 0);
			return;

			//never Executed

			//float newIntensity = glm::clamp(Intensity* 0.5f, 0.f, 5.f);
			//Terrain->setHeightAtPoint(xRemaining, yRemaining, Terrain->getHeightAtPoint(xRemaining, yRemaining) + Intensity);
			//QSF_LOG_PRINTS(INFO," a aha " <<(uint32)Terrain->getBlendTextureCount());
			//Terrain->getLayerBlendMap(0)->getLayerIndex()
			for (uint8 t = 1; t <= (uint32)Terrain->getBlendTextureCount() + 1; t++)
			{
				auto BlendMap = Terrain->getLayerBlendMap(t);
				if (BlendMap != nullptr)
				{

					//QSF_LOG_PRINTS(INFO, (uint32)t << " " <<Terrain->getBlendTextureName(t).c_str());
					QSF_LOG_PRINTS(INFO, "Texture Index is " << (uint32)Terrain->getBlendTextureIndex(t) << " " << Terrain->getBlendTextureName(t).c_str());
					QSF_LOG_PRINTS(INFO, "Channel Index " << (uint32)Terrain->getLayerBlendTextureIndex(t).first << " RGBA" << (uint32)Terrain->getLayerBlendTextureIndex(t).second);
				}
				else
					QSF_LOG_PRINTS(INFO, "Blendmap layer " << (uint32)t << " is a nullptr")
			}*/

		}




		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]





		glm::vec3 TerrainTexturingTool::getPositionUnderMouse()
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





		void TerrainTexturingTool::SaveMap(const qsf::MessageParameters & parameters)
		{
			SaveTheFuckingMap();
		}

		void TerrainTexturingTool::SaveTheFuckingMap()
		{
			if (TerrainEditGUI == nullptr)
				return;
			if (TerrainMaster.get() == nullptr)
				return;
			auto path = TerrainEditGUI->GetSavePath();
			for (long t = 0; t <= (mParts - 1); t++)
			{
				for (long i = 0; i <= (mParts - 1); i++)
				{

					//QSF_LOG_PRINTS(INFO, " t " << t << " i " << i)
					if (TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i) != nullptr)
					{
						TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i)->save(path + "\\a" + boost::lexical_cast<std::string>(t) + "_" + boost::lexical_cast<std::string>((i)) + ".chunk");
					}
				}
			}
			//qsf::Asset* asset = nullptr;
			//asset = mAssetEditHelper->duplicateAsset(a, OrginalPackage, nullptr, a);


			//we need to push this stuff in a assetpackage!
		}

		void TerrainTexturingTool::LoadOldMap()
		{

			//SaveTheFuckingMap();
			if (TerrainEditGUI == nullptr)
				return;
			if (TerrainMaster.get() == nullptr)
				return;
			auto path = TerrainEditGUI->GetSavePath();
			TerrainMaster->getOgreTerrainGroup()->removeAllTerrains();
			QSF_RENDERER.getOgreRoot()->addResourceLocation(path, "", "./chunks");
			for (long x = 0; x <= (mParts - 1); x++)
			{
				for (long y = 0; y <= (mParts - 1); y++)
				{
					if (x == 0 && y == 0)
					{
						TerrainMaster->getOgreTerrainGroup()->defineTerrain(0, 0, "em5/unknown/default/0_0");
						QSF_LOG_PRINTS(INFO, x << " 1" << y)
					}
					else
					{
						TerrainMaster->getOgreTerrainGroup()->defineTerrain(x, y, Ogre::String("chunks/a" + boost::lexical_cast<std::string>(x) + "_" + boost::lexical_cast<std::string>((y)) + ""));
						QSF_LOG_PRINTS(INFO, x << " " << y)
					}
				}
			}

			TerrainMaster->getOgreTerrainGroup()->loadAllTerrains(true);
			TerrainMaster->getOgreTerrainGroup()->freeTemporaryResources();
			QSF_LOG_PRINTS(INFO, "Load old map done!")
		}


		std::string TerrainTexturingTool::GetCurrentTimeForFileName()
		{
			auto time = std::time(nullptr);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&time), "%F_%T"); // ISO 8601 without timezone information.
			auto s = ss.str();
			std::replace(s.begin(), s.end(), ':', '-');
			return s;
		}

		float TerrainTexturingTool::ReadValue(glm::vec2 point)
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

				//QSF_LOG_PRINTS(INFO, "Terrain is a nullptr" << xTerrain << " " << yTerrain)
				return 0.f;
			}
			return Terrain->getHeightAtPoint(xRemaining, yRemaining);//TerrainEditGUI->GetHeight());
		}

		inline void TerrainTexturingTool::mousePressEvent(QMouseEvent & qMouseEvent)
		{
			if (Qt::RightButton == qMouseEvent.button() && TerrainEditGUI != nullptr && TerrainEditGUI->GetEditMode() == TerrainEditGUI->Set)
			{
				glm::vec3 mousepos2;
				if (evaluateBrushPosition(qMouseEvent.pos(), mousepos2))
				{
					glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(mousepos2.x, mousepos2.z));
					Mappoint = Mappoint*Heighmapsize;
					float value = ReadValue(glm::vec2(glm::round(Mappoint.x), glm::round(Mappoint.y)));
					//TerrainEditGUI->SetHeight(value);
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

		inline void TerrainTexturingTool::mouseMoveEvent(QMouseEvent & qMouseEvent)
		{
			glm::vec3 mousepos2;
			if (TerrainEditGUI != nullptr)
				Radius = TerrainEditGUI->GetBrushRadius();
			if (evaluateBrushPosition(qMouseEvent.pos(), mousepos2))
			{
				//QSF_LOG_PRINTS(INFO, mousepos2);
				oldmouspoint = mousepos2;
				mouseisvalid = true;
				WriteTerrainTextureList();
				//QSF_LOG_PRINTS(INFO, "Radius is" << Radius)
				return;
			}
			else
				//QSF_LOG_PRINTS(INFO, "invalid");
				mouseisvalid = false;
		}

		void TerrainTexturingTool::WriteTerrainTextureList()
		{
			//find Terrain
			glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(oldmouspoint.x, oldmouspoint.z));
			Mappoint = Mappoint*Heighmapsize;
			int xTerrain = 0;
			int xRemaining = (int)Mappoint.x;
			int yTerrain = 0;
			int yRemaining = (int)Mappoint.y;
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
			//QSF_LOG_PRINTS(INFO, "we are here" << xTerrain << " " << yTerrain )
			//QSF_LOG_PRINTS(INFO, xTerrain << " " << yTerrain << " " << xRemaining << " " << yRemaining)

			auto Terrain = TerrainMaster->getOgreTerrainGroup2()->getTerrain(xTerrain, yTerrain);
			if(Terrain == nullptr)
			{
				//QSF_LOG_PRINTS(INFO,"Terrain is a nullptr")
				return;
			}

			const uint32 maximumNumberOfLayers = TMG_getMaxLayers(Terrain);
			const uint32 numberOfLayers = std::min(maximumNumberOfLayers,(uint32)Terrain->getLayerCount());
			std::vector<std::string> TerrainNames;
			//QSF_LOG_PRINTS(INFO, "we are here"<< (int)maximumNumberOfLayers<<  " ...."<< numberOfLayers  <<"...."<<  (uint32)Terrain->getLayerCount())
			for (uint32 layerIndex = 0; layerIndex < numberOfLayers; ++layerIndex)
			{
				const std::string layerIndexAsString = std::to_string(layerIndex);
				// Inside the first texture name of the terrain layer we store the global asset ID of the QSF material the terrain layer is using, we need nothing more
				const std::string globalAssetIdAsString = Terrain->getLayerTextureName(layerIndex, 0);
				//QSF_LOG_PRINTS(INFO, "Terrainname" << xTerrain << " " << yTerrain << " " << globalAssetIdAsString)
				if(qsf::AssetProxy(globalAssetIdAsString).getAsset() != nullptr)
				//QSF_LOG_PRINTS(INFO, globalAssetIdAsString << " " << qsf::AssetProxy(boost::lexical_cast<uint64>(globalAssetIdAsString)).getLocalAssetName() << " " << layerIndex)
				TerrainNames.push_back(qsf::AssetProxy(globalAssetIdAsString).getLocalAssetName());
				//TerrainEditGUI->
				

			}
			TerrainEditGUI->SetCurrentTerrainData(TerrainNames, xTerrain, yTerrain);
		}

		uint64 TerrainTexturingTool::GetSelectedLayerColor()
		{
			std::string LayerColorName = TerrainEditGUI->GetLayerColor();
			mSelectedLayerColor = qsf::AssetProxy(LayerColorName).getGlobalAssetId();
			//QSF_LOG_PRINTS(INFO, "selected color" << mSelectedLayerColor);
			return mSelectedLayerColor;
		}


		int TerrainTexturingTool::GetBlendMapWithTextureName(int xTerrain, int yTerrain)
		{
			auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(xTerrain, yTerrain);
			const uint32 maximumNumberOfLayers = TMG_getMaxLayers(Terrain);
			const uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(Terrain->getLayerCount()));
			std::vector<std::string> TerrainNames;
			for (uint32 layerIndex = 0; layerIndex < numberOfLayers; ++layerIndex)
			{
				try
				{
					//QSF_LOG_PRINTS(INFO, "layer Index" << Terrain->getLayerTextureName(layerIndex, 0).c_str() << " vs " << mSelectedLayerColor)
					if (qsf::AssetProxy(Terrain->getLayerTextureName(layerIndex, 0).c_str()).getAsset() != nullptr && qsf::AssetProxy(Terrain->getLayerTextureName(layerIndex, 0).c_str()).getAsset()->getGlobalAssetId() == mSelectedLayerColor)
					{
						QSF_LOG_PRINTS(INFO, "found layer Index" << mSelectedLayerColor);
						return layerIndex;
					}
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO, Terrain->getLayerTextureName(layerIndex, 0) << " and selected color is " << mSelectedLayerColor)
					QSF_LOG_PRINTS(INFO, e.what());
				}


			}
			return -1;
		}

		uint8 TerrainTexturingTool::TMG_getMaxLayers(const Ogre::Terrain * ogreTerrain) const
		{
			// Count the texture units free
			uint8 freeTextureUnits = OGRE_MAX_TEXTURE_LAYERS;
			--freeTextureUnits;	// Color map
			--freeTextureUnits;	// Normal map

								// Each layer needs 2.25 units(1x_crgb_ha, 1x_nag_sr_gb, 0.25xblend)
			return static_cast<Ogre::uint8>(freeTextureUnits / 2.25f);
		}


		glm::vec2 TerrainTexturingTool::ConvertWorldPointToRelativePoint(glm::vec2 WorldPoint)
		{
			glm::vec2 copy = WorldPoint;
			copy = (WorldPoint + Offset) / TerrainMaster->getTerrainWorldSize();
			copy.y = 1.f - copy.y; //we need to mirror Y
			return copy;
		}

		glm::vec2 TerrainTexturingTool::ConvertMappointToWorldPoint(glm::vec2 Mappoint)
		{
			glm::vec2 copy = Mappoint;
			copy = copy / (float)Heighmapsize;
			copy = copy * TerrainMaster->getTerrainWorldSize();
			copy = copy - Offset;
			copy.y = -1.f * copy.y;
			return copy;
		}

		void TerrainTexturingTool::UpdateTerrains()
		{
		QSF_LOG_PRINTS(INFO,"Update Started")
			//we should make sure that we have a 4 x 4 Terrain-pattern (16 tiles).
			//we assume it here by t = x and y = i
			for (long t = 0; t <= (mParts - 1); t++)
			{
				for (long i = 0; i <= (mParts - 1); i++)
				{

					//QSF_LOG_PRINTS(INFO, " t " << t << " i " << i)
					if (TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i) != nullptr)
					{
						//QSF_LOG_PRINTS(INFO, "Update works on "<< t << " " << i)
						//Write Blend Map Data
						if(AffectedPoints[t][i].empty())
							continue;
						int BlendMapIndex = GetBlendMapWithTextureName((long)t, (long)i);
						if (BlendMapIndex == -1)
							continue;
						auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i);

						const uint32 maximumNumberOfLayers = TMG_getMaxLayers(Terrain);
						const uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(Terrain->getBlendTextureCount()));
						QSF_LOG_PRINTS(INFO, "numberOfLayers " << numberOfLayers << "a123 " << BlendMapIndex)
						//Terrain->getMaterial()-> MaterialPropertyValue::fromResourceName(ogreTerrain->getGlobalColourMap()->getName()));
						//QSF_LOG_PRINTS(INFO, "TerrainName " << Terrain->getMaterialName().c_str());
						//Terrain->getMaterial()->get
						//TerrainMaster->context
							for (uint32 layerIndex = 1; layerIndex < 6; ++layerIndex)
							{
							auto CurrentBlendMap = Terrain->getLayerBlendMap(layerIndex);
							if(CurrentBlendMap == nullptr)
							continue;
								//if (Terrain->getLayerBlendMap(layerIndex) == nullptr)
									//continue;
								for (auto points : AffectedPoints[t][i])
								{

									try
									{
										if (layerIndex == BlendMapIndex)
										{
											//QSF_LOG_PRINTS(INFO, "found the layer")
											CurrentBlendMap->setBlendValue(points.x, points.y, 1);
											//QSF_LOG_PRINTS(INFO, "match" << layerIndex)
										}
										else
											CurrentBlendMap->setBlendValue(points.x, points.y, 0);
									}
									catch (const std::exception& e)
									{
										QSF_LOG_PRINTS(INFO,"layer index "<< layerIndex << " " <<e.what())
										continue;
									}
								}
								
								
								if (Terrain->getLayerBlendMap(layerIndex) != nullptr)
								{
									CurrentBlendMap->dirty();
									CurrentBlendMap->update();
								}
								//End

							}
						AffectedPoints[t][i].clear();
						QSF_LOG_PRINTS(INFO, "Clear list and update stuff")

					}
				}
			}
		QSF_LOG_PRINTS(INFO, "Update finished")
		}


		bool TerrainTexturingTool::onStartup(EditMode * previousEditMode)
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
			//TerrainMaster->setEditing(true);
			//auto terrain = TES.at(0)->getOgreTerrain();
			QSF_LOG_PRINTS(INFO, TerrainMaster->getTerrainWorldSize());
			QSF_LOG_PRINTS(INFO, TerrainMaster->getHeightMapSize());
			QSF_LOG_PRINTS(INFO, "Blendmapsize" << TerrainMaster->getBlendMapSize());
			QSF_LOG_PRINTS(INFO, "partsize" << TerrainMaster->getOgreTerrainGroup()->getTerrainSize());

			Offset = (float)(TerrainMaster->getTerrainWorldSize() / 2);
			percentage = 1.f / (float)TerrainMaster->getTerrainWorldSize(); /// (float)TerrainMaster->getHeightMapSize();
			Heighmapsize = (float)TerrainMaster->getHeightMapSize();
			Heighmapsize = (float)TerrainMaster->getBlendMapSize();
			partsize = TerrainMaster->getOgreTerrainGroup()->getTerrainSize() - 1;


			QSF_LOG_PRINTS(INFO, "scale" << Heighmapsize / TerrainMaster->getTerrainWorldSize() << " units per meter");
			Scale = Heighmapsize / TerrainMaster->getTerrainWorldSize();
			mParts = floor(Heighmapsize / (partsize - 1.f));
			QSF_LOG_PRINTS(INFO, "we have " << mParts << " x " << mParts << "  Parts");
			EditMode::onStartup(previousEditMode);

			user::editor::TerrainTexturingToolbox* TET = static_cast<user::editor::TerrainTexturingToolbox*>(this->getManager().getToolWhichSelectedEditMode());
			if (TET == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "TET is a nullptr");
				return false;
			}
			TerrainEditGUI = TET;

			//this is it finally!
			//QSF_MATERIAL.getMaterialManager().

			


			/*for(size_t t=0; t <15; t++ )
			{
				Ogre::TerrainLayerBlendMap* CurrentBlendMap = nullptr;
				QSF_LOG_PRINTS(INFO,"t is " << t);
				try
				{
					 CurrentBlendMap = TerrainMaster->getOgreTerrainGroup()->getTerrain((uint8)1, (uint8)1)->getLayerBlendMap((uint8)t);
					 std::string prefix = "testtesttest" + boost::lexical_cast<std::string>(t);
					 QSF_LOG_PRINTS(INFO,TerrainMaster->getColorMapSize())
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO,e.what())
					continue;
				}
				if(CurrentBlendMap == nullptr)
				continue;
				for (size_t x = 0; x < 20; x++)
				{
					for (size_t y = 0; y < 20; y++)
					{
						QSF_LOG_PRINTS(INFO, "x and y  " << x << " " << y);
						CurrentBlendMap->setBlendValue((uint8)x,(uint8)y,1/(20*20)*x*y);
						QSF_LOG_PRINTS(INFO, "value was set");
					}
				}
				QSF_LOG_PRINTS(INFO, "start update");
				CurrentBlendMap->dirty();
				CurrentBlendMap->update();
				QSF_LOG_PRINTS(INFO, "finish update");
			}
			QSF_LOG_PRINTS(INFO, "done");*/
			/*QSF_LOG_PRINTS(INFO, "do find out matname")
			auto OE = TerrainMaster->getOgreTerrain();
			if (OE == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "sth happened 3")
				return false;
			}
			if (OE == nullptr || &OE->getMaterial() == nullptr)
				return false;
			QSF_LOG_PRINTS(INFO, "sth happened 1")
			auto OEM = OE->getMaterial();
			QSF_LOG_PRINTS(INFO, "sth happened")

			{
				QSF_LOG_PRINTS(INFO,"Materialname :" << OEM.get()->getName());
			}*/


			/*if (QSF_FILE.exists(TerrainMaster->getTerrainAsset().getLocalAssetName() +".json"))
				{

			boost::property_tree::ptree root;
			QSF_LOG_PRINTS(INFO,"terrain asset is " << TerrainMaster->getTerrainAsset().getLocalAssetName())
			qsf::FileStream stream(TerrainMaster->getTerrainAsset().getLocalAssetName()+".json", qsf::File::READ_MODE);
			qsf::FileHelper::readJson(stream, root);

			boost::optional<uint64> GetBlinker = root.get_child("Properties").get_optional<uint64>("ColorMap");
			if (GetBlinker)
			{
				const Ogre::String& texture_name = qsf::AssetProxy(GetBlinker.get()).getAbsoluteCachedAssetDataFilename();
				Ogre::String& texture_path = qsf::AssetProxy(GetBlinker.get()).getAbsoluteCachedAssetDataFilename();
				bool image_loaded = false;
				std::ifstream ifs(texture_path.c_str(), std::ios::binary | std::ios::in);
				if (ifs.is_open())
				{
					Ogre::String tex_ext;
					Ogre::String::size_type index_of_extension = texture_path.find_last_of('.');
					if (index_of_extension != Ogre::String::npos)
					{
						tex_ext = texture_path.substr(index_of_extension + 1);
						Ogre::DataStreamPtr data_stream(new Ogre::FileStreamDataStream(texture_path, &ifs, false));
						Ogre::Image img;
						img.load(data_stream, tex_ext);
						Ogre::TextureManager::getSingleton().loadImage(texture_name,
							Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, img, Ogre::TEX_TYPE_2D, 0, 1.0f);
						image_loaded = true;
						//QSF_LOG_PRINTS(INFO,img.getColourAt(0,0,0).r << img.getSize())
							auto path = TerrainEditGUI->GetSavePath();
						//img.save(path + "\\colormap + "+"__date__" + GetCurrentTimeForFileName() + ".tif");
						TerrainMaster->getOgreTerrain()->setGlobalColourMapEnabled(true,2000);
						TerrainMaster->getOgreTerrain()->getGlobalColourMap().get()->loadImage(img);



					}
					ifs.close();
				}
				QSF_LOG_PRINTS(INFO,"loaded" << image_loaded)

			}
			else
			{
				QSF_LOG_PRINTS(INFO, "Didnt find colormap");
			}
			//try to access paint tools
			}*/
			//else
				//QSF_LOG_PRINTS(INFO, "Didnt find terrain asset");
			//return true;
			if (!PaintJobProxy.isValid())
				PaintJobProxy.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&TerrainTexturingTool::PaintJob, this, _1));
			mSaveMapProxy.registerAt(qsf::MessageConfiguration(qsf::MessageConfiguration("kc::save_heightmap")), boost::bind(&TerrainTexturingTool::SaveMap, this, _1));

			//LoadOldMap();
			return true;
		}

		void TerrainTexturingTool::onShutdown(EditMode * nextEditMode)
		{
			EditMode::onShutdown(nextEditMode);
			mSaveMapProxy.unregister();
			PaintJobProxy.unregister();
			mDebugDrawProxy.unregister();
			SaveTheFuckingMap();
			QSF_LOG_PRINTS(INFO, "Shutdown")
		}




		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
