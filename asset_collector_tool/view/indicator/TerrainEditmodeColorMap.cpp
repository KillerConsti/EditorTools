// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/indicator/TerrainEditmodeColorMap.h"
#include <asset_collector_tool\qsf_editor\tools\TerrainEditColorMapToolbox.h>
#include "ui_TerrainEditmodeColorMap.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

#include <qsf_editor/EditorHelper.h>

#include <qsf/map/Map.h>
#include <qsf/map/Entity.h>
#include <qsf/QsfHelper.h>
#include <qsf\component\base\MetadataComponent.h>
#include <em5\plugin\Jobs.h>
#include <qsf/debug/request/CircleDebugDrawRequest.h>
#include <qsf/math/CoordinateSystem.h>
#include <qsf/input/InputSystem.h>
#include <qsf/input/device/MouseDevice.h>
#include <qsf/map/query/RayMapQuery.h>
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


#include <qsf_editor_base/user/User.h>


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
#include <qsf/renderer/material/material/MaterialManager.h>
#include <qsf/renderer/material/MaterialSystem.h>

#include <qsf/debug/DebugDrawManager.h>
#include <qsf/plugin/PluginSystem.h>
#include <qsf\plugin\Plugin.h>
#include <chrono>

#include <qsf/asset/project/AssetPackage.h>
#include <qsf_editor\application\Application.h>
#include <qsf_editor/asset/import/AssetImportManager.h>
#include <qsf/asset/project/Project.h>
#include <qsf_editor/asset/import/AssetPackageImportHelper.h>
#include <qsf/asset/project/Project.h>
#include <em5\EM5Helper.h>
#include <em5/modding/ModSystem.h>
#include <qsf/plugin/QsfAssetTypes.h>
#include <qsf_editor_base/asset/compiler/CopyAssetCompiler.h>
#include <qsf/file/FileSystem.h>
#include <experimental/filesystem> 
#include <boost\filesystem.hpp>
#include <qsf_editor/asset/AssetEditHelper.h>
#include <experimental/filesystem>
#include  <qsf/renderer/terrain/TerrainDefinition.h>
#include <qsf/map/component/MapPropertiesBaseComponent.h>
#include <asset_collector_tool\Manager\Settingsmanager.h>
using namespace std::chrono;


namespace user
{
	namespace editor
	{

		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		const uint32 TerrainEditmodeColorMap::PLUGINABLE_ID = qsf::StringHash("user::editor::TerrainEditmodeColorMap");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainEditmodeColorMap::TerrainEditmodeColorMap(qsf::editor::EditModeManager* editModeManager) :
			EditMode(editModeManager)
		{
			timer = 0;
			//QSF_LOG_PRINTS(INFO, "start Edit Mode Color Map")
		}


		TerrainEditmodeColorMap::~TerrainEditmodeColorMap()
		{
			
			if (TerrainEditColorMapToolbox::GetInstance() == nullptr) //call on shutdown if our gui was shutdowned
			{
				return;
			}
			if(!PaintJobProxy.isValid())
			return;
			PaintJobProxy.unregister();
			mDebugDrawProxy.unregister();
			QSF_LOG_PRINTS(INFO, "TerrainEditmodeColorMap Shutdown")
			OnFinishEditing();
			//QSF_LOG_PRINTS(INFO, "left Edit Mode Color Map")
		}



		bool TerrainEditmodeColorMap::evaluateBrushPosition(const QPoint & mousePosition, glm::vec3 & position)
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


		void TerrainEditmodeColorMap::PaintJob(const qsf::JobArguments & jobArguments)
		{
			if (TerrainEditColorMapToolbox::GetInstance() == nullptr || !TerrainMaster.valid()) //call on shutdown if our gui was shutdowned
			{
				onShutdown(nullptr);
				return;
			}
			if(!TerrainEditGUI->IsUnlocked())
			return;
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
			{
				PaintJobProxy.changeTimeBetweenCalls(qsf::Time::fromMilliseconds(30));
				return;
			}

			glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(oldmouspoint.x, oldmouspoint.z));
			Mappoint = Mappoint*mColorMapSize;
			/*switch (TerrainEditGUI->GetEditMode()) //Special Handlings
			{
			case 0: //Set
			{*/
			//if (yo_mousepoint == oldmouspoint)
				//return;
			yo_mousepoint = oldmouspoint;
			RaiseTerrain(Mappoint);
			return;

		}

		void TerrainEditmodeColorMap::RaiseTerrain(glm::vec2 MapPoint)
		{
			mJobStartTime = high_resolution_clock::now();
			if (TerrainEditGUI == nullptr)
				return;
			timer++;
			if (timer >= 1)
				timer = 0;
			else
				return;
			auto ColorMapToRead = TerrainMaster->GetColorMap();
			if (ColorMapToRead.getAsset() == nullptr)
			{
				//QSF_LOG_PRINTS(INFO,"no color map is set in terrain component")
				PaintJobProxy.unregister();
				return;
			}
			if (image == nullptr)
			{
				//QSF_LOG_PRINTS(INFO, "image is null")
				PaintJobProxy.unregister();
				return;
			}
			float TotalRadius = Radius*mScale;
			//QSF_LOG_PRINTS(INFO,"Radius "<<TotalRadius)
			float BrushIntensity = TerrainEditGUI->GetBrushIntensity();
			int MapPointMinX = glm::clamp((int)glm::ceil(MapPoint.x - TotalRadius), 0, (int)mColorMapSize);
			int MapPointMaxX = glm::clamp((int)glm::floor(MapPoint.x + TotalRadius), 0, (int)mColorMapSize);
			int MapPointMinY = glm::clamp((int)glm::ceil(MapPoint.y - TotalRadius), 0, (int)mColorMapSize);
			int MapPointMaxY = glm::clamp((int)glm::floor(MapPoint.y + TotalRadius), 0, (int)mColorMapSize);
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
								RaisePoint(glm::vec2(t, j), BrushIntensity);
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Cone)
							{
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = 1.0f - (Distance / TotalRadius);
								RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod);
							}
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Dome)
							{
								//intensity formula here
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = glm::cos((Distance / TotalRadius) * glm::pi<float>()) * 0.5f + 0.5f;
								RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod);

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
					}
				}
			}
			UpdateTerrains();
		}

		bool TerrainEditmodeColorMap::RaisePoint(glm::vec2 point, float Intensity)
		{
			int xTerrain = 0;
			int xRemaining = (int)point.x;
			int yTerrain = 0;
			int yRemaining = (int)point.y;
			//we have a pattern like 4x4 (so in total 16 Terrains ... now find the correct one)
			//remaining is the point on the selected Terrain
			//old
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
			//new
			/*QSF_LOG_PRINTS(INFO," x1 " <<xTerrain << " xR1 " << xRemaining << " y1 " << yTerrain << " yR1 "<< yRemaining)
			//kc do other terrain calc
			 xTerrain = 0;
			 xRemaining = (int)point.x;
			 yTerrain = 0;
			 yRemaining = (int)point.y;
			xTerrain = xRemaining /(int)partsize;
			xRemaining = xRemaining % (int)partsize;
			yTerrain = yRemaining /(int)partsize;
			yRemaining = yRemaining % (int)partsize;

			QSF_LOG_PRINTS(INFO, " x2 " << xTerrain << " xR2 " << xRemaining << " y2 " << yTerrain << " yR2 " << yRemaining)*/
			//Mirror

			/*yTerrain = mParts-1-yTerrain;
			yRemaining = partsize -1- yRemaining;*/
			//is y inverted?
			//this calcs newY = 16-1 -(0...15)
			int inv_yTerrain = mParts - 1 - yTerrain;
			int y_inv_Remaining = partsize - 1 - yRemaining;

			//QSF_LOG_PRINTS(INFO, "Old Color" << "Red" << (int)(OldColor.r * 256) << " green " << (int)(OldColor.g * 256) << " Blue " << (int)(OldColor.b * 256) << " Alpha " << (int)(OldColor.a))
			//how to mix colors?
			auto NewColorASRGBA = TerrainEditGUI->GetSelectedColor();
			qsf::Color4 NewColor = qsf::Color4((float)NewColorASRGBA.red(), (float)NewColorASRGBA.green(), (float)NewColorASRGBA.blue(), (float)NewColorASRGBA.alpha());

			if (SetNewColor(NewColor, Intensity, glm::vec2(xRemaining, y_inv_Remaining), xTerrain, inv_yTerrain))
				NeedUpdates.push_back(Terrains(xTerrain, inv_yTerrain));
			return true;


		}


		qsf::Color4 TerrainEditmodeColorMap::GetOldColorFromSmallMaps(glm::vec2 & Mappoint, int x, int y)
		{
			qsf::Color4 Color = qsf::Color4();
			auto IMG = GetSmallImageByTerrainId(x, y).first;
			if (IMG == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "image to write was not found")
					return Color;
			}
			//QSF_LOG_PRINTS(INFO, GetSmallImageByTerrainId(x, y).second << " x " << x << " y " << y << " MP" << Mappoint)
			//QSF_LOG_PRINTS(INFO,MapPoint)
			int col = (int)IMG->columns();
			int row = (int)IMG->rows();
			int channels = (int)IMG->channels();
			//editor img is flipped so we need to switch Mappoint.y
			//Mappoint.y = row - (int)Mappoint.y;
			MagickCore::Quantum *pixels = IMG->getPixels(0, 0, col, row);
			if (channels == 3) //rgb
			{
				uint64 offset = (row* Mappoint.y + Mappoint.x) * channels; //4 is because we use 4 channels
				float Red = (float)(*(pixels + offset) / 255);
				float Green = (float)(*(pixels + offset + 1) / 255);
				float Blue = (float)(*(pixels + offset + 2) / 255);
				//QSF_LOG_PRINTS(INFO,"Red"<< Red << " green "<< Green << " Blue "<< Blue)
				return qsf::Color4(Red, Green, Blue, 1.f);
			}
			else if (channels == 4) //rgba
			{
				uint64 offset = (row* Mappoint.y + Mappoint.x) * channels; //4 is because we use 4 channels
				float Red = (float)(*(pixels + offset) / 255);
				float Green = (float)(*(pixels + offset + 1) / 255);
				float Blue = (float)(*(pixels + offset + 2) / 255);
				float Alpha = (float)(*(pixels + offset + 3) / 255);
				//QSF_LOG_PRINTS(INFO, "Red" << (int)(Red) << " green " << (int)(Green) << " Blue " << (int)(Blue)<< " Alpha " << (int)(Alpha))
				return qsf::Color4(Red, Green, Blue, Alpha);
			}
			return qsf::Color4();
		}

		bool TerrainEditmodeColorMap::SetNewColor(qsf::Color4 NewColor, int Intensity, glm::vec2 & Mappoint, int x, int y)
		{
			qsf::Color4 ReallyNewColor;
			auto IMG = GetSmallImageByTerrainId(x, y).first;
			if (IMG == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "image to write was not found")
			}
			//QSF_LOG_PRINTS(INFO, GetSmallImageByTerrainId(x, y).second << " x " << x << " y " << y << " MP" << Mappoint)
			//QSF_LOG_PRINTS(INFO,MapPoint)
			int col = (int)IMG->columns();
			int row = (int)IMG->rows();
			int channels = (int)IMG->channels();
			//editor img is flipped so we need to switch Mappoint.y
			//Mappoint.y = row - (int)Mappoint.y;
			MagickCore::Quantum *pixels = IMG->getPixels(0, 0, col, row);
			if (channels == 3) //rgb
			{
				uint64 offset = (row* Mappoint.y + Mappoint.x) * channels; //4 is because we use 4 channels
				float Red = (float)(*(pixels + offset));
				float Green = (float)(*(pixels + offset + 1));
				float Blue = (float)(*(pixels + offset + 2));
				//QSF_LOG_PRINTS(INFO,"Red"<< Red << " green "<< Green << " Blue "<< Blue)

				ReallyNewColor.r = glm::round((Red*(100.f - Intensity) + NewColor.r*Intensity * 256) / 100.f);
				ReallyNewColor.g = glm::round((Green*(100.f - Intensity) + NewColor.g*Intensity * 256) / 100.f);
				ReallyNewColor.b = glm::round((Blue*(100.f - Intensity) + NewColor.b*Intensity * 256) / 100.f);

				if (glm::abs(Red - ReallyNewColor.r) < 550 && glm::abs(Green - ReallyNewColor.g) < 550 && glm::abs(Blue - ReallyNewColor.b) < 550)
				{
					//QSF_LOG_PRINTS(INFO,"same colour")
					return false; //no need to update
				}
				*(pixels + offset) = ReallyNewColor.r;
				*(pixels + offset + 1) = ReallyNewColor.g;
				*(pixels + offset + 2) = ReallyNewColor.b;
			}
			else if (channels == 4) //rgba
			{
				uint64 offset = (row* Mappoint.y + Mappoint.x) * channels; //4 is because we use 4 channels
				float Red = (float)(*(pixels + offset));
				float Green = (float)(*(pixels + offset + 1));
				float Blue = (float)(*(pixels + offset + 2));
				float Alpha = (float)(*(pixels + offset + 3));
				//QSF_LOG_PRINTS(INFO, "Red" << (int)(Red) << " green " << (int)(Green) << " Blue " << (int)(Blue)<< " Alpha " << (int)(Alpha))
				ReallyNewColor.r = glm::round((Red*(100.f - Intensity) + NewColor.r*Intensity * 256) / 100.f);
				ReallyNewColor.g = glm::round((Green*(100.f - Intensity) + NewColor.g*Intensity * 256) / 100.f);
				ReallyNewColor.b = glm::round((Blue*(100.f - Intensity) + NewColor.b*Intensity * 256) / 100.f);
				ReallyNewColor.a = glm::round((Alpha*(100.f - Intensity) + NewColor.a*Intensity * 256) / 100.f);
				if (NewColor.a < 50)
				{
					ReallyNewColor.a = 0;
				}
				else
				{
					NewColor.a = 65535.f; //no alpha
				}
				if (glm::abs(Red - ReallyNewColor.r) < 550 && glm::abs(Green - ReallyNewColor.g) < 550 && glm::abs(Blue - ReallyNewColor.b) < 550 && glm::abs(Alpha - ReallyNewColor.a) < 550)
				{
					//QSF_LOG_PRINTS(INFO,"same colour")
					return false; //no need to update
				}
				*(pixels + offset) = ReallyNewColor.r;
				*(pixels + offset + 1) = ReallyNewColor.g;
				*(pixels + offset + 2) = ReallyNewColor.b;
				*(pixels + offset + 3) = ReallyNewColor.a;
				//QSF_LOG_PRINTS(INFO, "Red1" << (int)(NewColor.r) << " green " << (int)(NewColor.g) << " Blue " << (int)(NewColor.b)<< " Alpha " << (int)(NewColor.a))
				//QSF_LOG_PRINTS(INFO, "Red2" << (int)(ReallyNewColor.r) << " green " << (int)(ReallyNewColor.g) << " Blue " << (int)(ReallyNewColor.b) << " Alpha " << (int)(ReallyNewColor.a))

			}
			return true;
		}




		//[-------------------------------------------------------]
		//[ Private Qt slots (MOC)                                ]
		//[-------------------------------------------------------]





		glm::vec3 TerrainEditmodeColorMap::getPositionUnderMouse()
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


		std::string TerrainEditmodeColorMap::GetCurrentTimeForFileName()
		{
			auto time = std::time(nullptr);
			std::stringstream ss;
			ss << std::put_time(std::localtime(&time), "%F_%T"); // ISO 8601 without timezone information.
			auto s = ss.str();
			std::replace(s.begin(), s.end(), ':', '-');
			return s;
		}





		inline void TerrainEditmodeColorMap::mouseMoveEvent(QMouseEvent & qMouseEvent)
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

		void TerrainEditmodeColorMap::SetUseSplitMaps(bool use)
		{
			if (TerrainMaster == nullptr) //only setable if not editing
			{
				mUseSplitMaps = use;
			}
		}

		bool TerrainEditmodeColorMap::GetUseSplitMaps()
		{
			return mUseSplitMaps;
		}

		glm::vec2 TerrainEditmodeColorMap::ConvertWorldPointToRelativePoint(glm::vec2 WorldPoint)
		{
			glm::vec2 copy = WorldPoint;
			qsf::TransformComponent* TC = TerrainMaster->getEntity().getComponent<qsf::TransformComponent>();
			glm::vec3 OffsetPos = /*TC->getRotation()**/TC->getPosition();
			copy = copy - glm::vec2(OffsetPos.x, OffsetPos.z);
			copy = (copy + Offset) / TerrainMaster->getTerrainWorldSize();
			copy.y = 1.f - copy.y; //we need to mirror Y
			return copy;
		}



		void TerrainEditmodeColorMap::UpdateTerrains()
		{
			if (mUseSplitMaps)
			{
				//Replace Func
				UpdateTerrainsForSmallMaps();
				return;
			}
			//draw img
			auto ColorMapToRead = TerrainMaster->GetColorMap();
			if (ColorMapToRead.getAsset() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no color map is set in terrain component")
					PaintJobProxy.unregister();
				return;
			}
			if (image == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "image is null")
					PaintJobProxy.unregister();
				return;
			}
			int col = (int)image->columns();
			int row = (int)image->rows();
			int channels = (int)image->channels();
			//auto start = high_resolution_clock::now();
			QSF_LOG_PRINTS(INFO, "Painted Pixels " << PaintedPixels.size())
				MagickCore::Quantum *pixels = image->getPixels(0, 0, col, row);
			if (channels == 3) //rgb
			{
				for (auto a : PaintedPixels)
				{
					uint64 offset = (row* a->Pixel.y + a->Pixel.x) * channels; //4 is because we use 4 channels
					*(pixels + offset) = a->NewColor.r * 256;
					*(pixels + offset + 1) = a->NewColor.g * 256;
					*(pixels + offset + 2) = a->NewColor.b * 256;
				}
			}
			else if (channels == 4) //rgba
			{
				for (auto a : PaintedPixels)
				{
					uint64 offset = (row* a->Pixel.y + a->Pixel.x) * channels; //4 is because we use 4 channels
					*(pixels + offset) = a->NewColor.r * 256;
					*(pixels + offset + 1) = a->NewColor.g * 256;
					*(pixels + offset + 2) = a->NewColor.b * 256;
					*(pixels + offset + 3) = a->NewColor.a * 256;
				}
			}
			image->syncPixels();
			image->write(ColorMapToRead.getAbsoluteCachedAssetDataFilename());
			//auto stop = high_resolution_clock::now();
			//auto duration = std::chrono::duration_cast<milliseconds>(stop - start);
			//QSF_LOG_PRINTS(INFO,"time to save img" <<duration.count())
			//we should make sure that we have a 4 x 4 Terrain-pattern (16 tiles).
			//we assume it here by t = x and y = i
			std::sort(NeedUpdates.begin(), NeedUpdates.end());
			//NeedUpdates.erase(std::unique(NeedUpdates.begin(), NeedUpdates.end()), [](Terrains const & l, Terrains const & r) {return l.x == r.x && l.y == r.y; }, NeedUpdates.end());

			NeedUpdates.erase(
				std::unique(
					NeedUpdates.begin(),
					NeedUpdates.end(),
					[](Terrains const & l, Terrains const & r) {return l.x == r.x && l.y == r.y; }
				),
				NeedUpdates.end()
			);
			//auto start2 = high_resolution_clock::now();
			TerrainMaster->ReloadSubTerrainMaterials(0, 0);
			//auto stop2 = high_resolution_clock::now();
			 //duration = std::chrono::duration_cast<milliseconds>(stop2 - start2);
			//QSF_LOG_PRINTS(INFO, "time to update img" << duration.count())
			/*for (auto a : NeedUpdates)
			{

				TerrainMaster->ReloadSubTerrainMaterials((long)a.x, (long)a.y);
				//QSF_LOG_PRINTS(INFO,"updating stuff now "<< a.x << " "<< a.y)
				//TerrainMaster->getOgreTerrainGroup()->getTerrain(a.x, a.y)->update(true);
			}*/
			NeedUpdates.clear();
			PaintedPixels.clear();
		}

		void TerrainEditmodeColorMap::UpdateTerrainsForSmallMaps()
		{

			std::sort(NeedUpdates.begin(), NeedUpdates.end());
			//NeedUpdates.erase(std::unique(NeedUpdates.begin(), NeedUpdates.end()), [](Terrains const & l, Terrains const & r) {return l.x == r.x && l.y == r.y; }, NeedUpdates.end());

			//QSF_LOG_PRINTS(INFO, "Updated needed (unfiltered" << NeedUpdates.size())
			NeedUpdates.erase(
				std::unique(
					NeedUpdates.begin(),
					NeedUpdates.end(),
					[](Terrains const & l, Terrains const & r) {return l.x == r.x && l.y == r.y; }
				),
				NeedUpdates.end()
			);
			//QSF_LOG_PRINTS(INFO,"Updated needed"<< NeedUpdates.size())

			for (auto a : NeedUpdates)
			{
				//QSF_LOG_PRINTS(INFO, "Update" << a.x << " " << a.y)
				auto IMG = GetSmallImageByTerrainId(a.x, a.y);
				if (IMG.first == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "TerrainEditmodeColorMap : Painted Pixels casted a not good terrain " << a.x << " " << a.y)
						continue;
				}
				IMG.first->syncPixels();
				std::string FullName = qsf::AssetProxy(IMG.second).getAbsoluteCachedAssetDataFilename();
				IMG.first->write(FullName);
				TerrainMaster->ReloadSmallTerrainMaterial(a.x, a.y, qsf::AssetProxy(IMG.second).getGlobalAssetId());
			}

			auto stop2 = high_resolution_clock::now();
			auto duration2 = std::chrono::duration_cast<milliseconds>(stop2 - mJobStartTime);

			//QSF_LOG_PRINTS(INFO, "time to save img" << duration2.count())
			//change time for calls -try to reduce lag
			PaintJobProxy.changeTimeBetweenCalls(qsf::Time::fromMilliseconds(duration2.count() + 30.f));
			NeedUpdates.clear();
			PaintedPixels.clear();
		}




		void TerrainEditmodeColorMap::generateMaterial()
		{
			auto ColorMapToRead = TerrainMaster->GetColorMap();
			if (ColorMapToRead.getAsset() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no color map is set in terrain component")
					return;
			}
			image = new Magick::Image(ColorMapToRead.getAbsoluteCachedAssetDataFilename());
		}


		bool TerrainEditmodeColorMap::onStartup(EditMode * previousEditMode)
		{

			//prevent crashs if terrain is not there yet
			if (qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<qsf::TerrainComponent>() == nullptr)
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


			Offset = (float)(TerrainMaster->getTerrainWorldSize() / 2);
			percentage = 1.f / (float)TerrainMaster->getTerrainWorldSize(); /// (float)TerrainMaster->getHeightMapSize();

			if (!PaintJobProxy.isValid())
				PaintJobProxy.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&TerrainEditmodeColorMap::PaintJob, this, _1));


			Ogre::TerrainGroup::TerrainIterator it = TerrainMaster->getOgreTerrainGroup()->getTerrainIterator();
			int counter = 0; // because my ID start at 0
			while (it.hasMoreElements()) // add the layer to all terrains in the terrainGroup
			{
				Ogre::TerrainGroup::TerrainSlot* a = it.getNext();
				counter++;
			}
			//TerrainMaster->setEditing(true);
			EditMode::onStartup(previousEditMode);

			user::editor::TerrainEditColorMapToolbox* TET = static_cast<user::editor::TerrainEditColorMapToolbox*>(this->getManager().getToolWhichSelectedEditMode());
			if (TET == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "TET is a nullptr");
				return false;
			}
			TerrainEditGUI = TET;

			mTerrainComponent = qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<kc_terrain::TerrainComponent>();
			image = nullptr;
			generateMaterial();
			if (image == nullptr)
			{
				CopyOldMapWithNoAssetLoaded();
			}
			if (image == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "buffer image is a nullptr")
					return false;
			}

			mColorMapSize = (float)image->rows();
			//QSF_LOG_PRINTS(INFO, "Colormapsize " << TerrainMaster->getColorMapSize());
			QSF_LOG_PRINTS(INFO, "scale" << mColorMapSize / TerrainMaster->getTerrainWorldSize() << " units per meter");
			mScale = mColorMapSize / TerrainMaster->getTerrainWorldSize();
			partsize = mColorMapSize / sqrt(counter);
			mParts = sqrt(counter);
			QSF_LOG_PRINTS(INFO, "we have " << mParts << " x " << mParts << "  Parts with a size of " << partsize << " x " << partsize);
			QSF_LOG_PRINTS(INFO, "started  TerrainEditmodeColorMap")

				mUseSplitMaps = true; //TODO use a setter
			SplitMapInSmallMaps();


			return true;
		}

		void TerrainEditmodeColorMap::onShutdown(EditMode * nextEditMode)
		{
			EditMode::onShutdown(nextEditMode);
			PaintJobProxy.unregister();
			mDebugDrawProxy.unregister();
			QSF_LOG_PRINTS(INFO, "TerrainEditmodeColorMap Shutdown")
				OnFinishEditing();
		}

		bool TerrainEditmodeColorMap::SplitMapInSmallMaps()
		{
			m_OldAssetPackage = QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackage();
			QSF_EDITOR_APPLICATION.getAssetImportManager().setDefaultDestinationAssetPackage("asset_collector_tool_for_editor", "kc_terrrain_working_dir");
			qsf::AssetPackage* AP;
			mAssetEditHelper = std::shared_ptr<qsf::editor::AssetEditHelper>(new qsf::editor::AssetEditHelper());
			for (auto a : EM5_MOD.getMods())
			{
				if (a.second->getName() == "asset_collector_tool_for_editor")
				{
					AP = a.second->getProject().getAssetPackageByName("kc_terrrain_working_dir");
				}
			}
			if (AP == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "cant find asset_collector_tool_for_editor with AP: kc_terrrain_working_dir")
					return false;
			}
			auto AIH = new  qsf::editor::AssetPackageImportHelper(*AP);
			//clear old small images
			m_SmallImages.clear();
			//get color map
			if (TerrainMaster->GetColorMap().getAsset() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no colormap loaded - set it up it the kc_terrain::TerrainComponent")
					return false;
			}
			//get save path
			std::string path = "";
			for (auto a : QSF_PLUGIN.getPlugins())
			{
				if (a->getFilename().find("asset_collector_tool.dll") != std::string::npos)
				{

					//it was 24 but we  want to remove x64\asset_collector_tool.dll from path
					path = a->getFilename();
					path.erase(path.end() - 28, path.end());


				}
			}


			if (path == "")
				return false;
			std::string pathtoassethelper = path;
			path += "kc_terrrain_working_dir\\colormaptextures\\";
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(path, "FileSystem");
			if (image == nullptr)
				return false;
			//now split into images
			auto IAP = mAssetEditHelper->getIntermediateAssetPackage();
			pathtoassethelper += IAP->getName();
			//QSF_LOG_PRINTS(INFO, IAP->getName());
			std::string widthandheight = boost::lexical_cast<std::string>(partsize) + "x" + boost::lexical_cast<std::string>(partsize);
			for (size_t x = 0; x < mParts; x++)
			{
				for (size_t y = 0; y < mParts; y++)
				{
					//QSF_LOG_PRINTS(INFO,"write map "<<x << " "<< y)
					Magick::Image* TerrainImg = new Magick::Image();
					TerrainImg->size(widthandheight);
					TerrainImg->magick("DDS");
					TerrainImg->type(Magick::ImageType::TrueColorAlphaType);
					int xOffset = x*partsize;
					int yOffset = y * partsize;
					TerrainImg->copyPixels(*image, Magick::Geometry(partsize, partsize, xOffset, yOffset), Magick::Offset(0, 0));
					TerrainImg->syncPixels();
					std::string LocalAssetNameToCheck = "asset_collector_tool_for_editor/texture/colormaptextures/terrainchunk_" + boost::lexical_cast<std::string>(x) + "_" + boost::lexical_cast<std::string>(y) + "_tecm";
					qsf::AssetProxy AP = qsf::AssetProxy(LocalAssetNameToCheck);
					if (AP.getAsset() != nullptr)
					{
						//does it exists?
						if(!std::experimental::filesystem::exists(AP.getAbsoluteCachedAssetDataFilename()))
						{
						//if not destroy it amd create new //... this can happen when using ramdisk which i recommend
						AP.getAssetPackage()->destroyAssetByGlobalAssetId(AP.getGlobalAssetId());
						}
					}
					AP = qsf::AssetProxy(LocalAssetNameToCheck);
					if (AP.getAsset() != nullptr)
					{
						try
						{
							TerrainImg->write(AP.getAbsoluteCachedAssetDataFilename());
							mAssetEditHelper->tryEditAsset(AP.getGlobalAssetId(), AP.getAssetPackage()->getName());
							mAssetEditHelper->setAssetUploadData(AP.getGlobalAssetId(), true, true);
							m_SmallImages.push_back(std::pair<Magick::Image*, std::string>(TerrainImg, AP.getLocalAssetName()));
							//QSF_LOG_PRINTS(INFO, "Asset is valid")
						}
						catch (const std::exception& e)
						{
							QSF_LOG_PRINTS(INFO, e.what())
						}


					}
					else
					{
						//QSF_LOG_PRINTS(INFO,"Path to write "<< path << "terrainchunk_" << boost::lexical_cast<std::string>(x) << "_" << boost::lexical_cast<std::string>(y) << "_tecm")
						try
						{
							TerrainImg->write(path + "terrainchunk_" + boost::lexical_cast<std::string>(x) + "_" + boost::lexical_cast<std::string>(y) + "_tecm.dds");
						}
						catch (const std::exception& e)
						{
							QSF_LOG_PRINTS(INFO, e.what())
								return false;
						}
						std::string LocalAssetName = "asset_collector_tool_for_editor/texture/colormaptextures/terrainchunk_" + boost::lexical_cast<std::string>(x) + "_" + boost::lexical_cast<std::string>(y) + "_tecm";
						m_SmallImages.push_back(std::pair<Magick::Image*, std::string>(TerrainImg, LocalAssetName));
						//add them to explorer
						/*auto Project = QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackageProject();
							//672362*/
						if (qsf::AssetProxy(LocalAssetName).getAsset() == nullptr)
						{

							//QSF_LOG_PRINTS(INFO, "asset not found " << LocalAssetName)
							try
							{
								//a add asset
								auto Asset = mAssetEditHelper->addAsset("kc_terrrain_working_dir", qsf::QsfAssetTypes::TEXTURE, "colormaptextures", "terrainchunk_" + boost::lexical_cast<std::string>(x) + "_" + boost::lexical_cast<std::string>(y) + "_tecm");
								//b create folder structure in assethelper (to read and write)
								if (!boost::filesystem::exists((QSF_FILE.getBaseDirectory() + "/data/asset_collector_tool_for_editor/" + IAP->getName() + "/texture/colormaptextures")))
									boost::filesystem::create_directories(QSF_FILE.getBaseDirectory() + "/data/asset_collector_tool_for_editor/" + IAP->getName() + "/texture/colormaptextures");
								//c write to new folder -I think it will copy to our direction we wrote before
								TerrainImg->write(pathtoassethelper + "/texture/colormaptextures/terrainchunk_" + boost::lexical_cast<std::string>(x) + "_" + boost::lexical_cast<std::string>(y) + "_tecm.dds");

								//d learn our assets a few thing <-> this seems not needed
								if (Asset == nullptr)
									QSF_LOG_PRINTS(INFO, "error occured " << LocalAssetName)
								else
								{
									auto CachedAsset = mAssetEditHelper->getCachedAsset(Asset->getGlobalAssetId());
									if (CachedAsset == nullptr)
										CachedAsset = &qsf::CachedAsset(Asset->getGlobalAssetId());
									if (CachedAsset == nullptr)
									{
										QSF_LOG_PRINTS(INFO, "still a nullptr")
									}
									CachedAsset->setType("dds");
									//QSF_LOG_PRINTS(INFO, qsf::AssetProxy(Asset->getGlobalAssetId()).getAbsoluteCachedAssetDataFilename())

									if (mAssetEditHelper->setAssetUploadData(Asset->getGlobalAssetId(), true, true))
										QSF_LOG_PRINTS(INFO, "Caching asset was not succesfull")
								}
							}
							catch (const std::exception& e)
							{
								QSF_LOG_PRINTS(INFO, e.what())
							}

						}
						else //tell them that asset was changed :)
						{

							//QSF_LOG_PRINTS(INFO,"for some reasons it exists")
							std::string TargetAssetName = qsf::AssetProxy(LocalAssetName).getAssetPackage()->getName();
							mAssetEditHelper->tryEditAsset(qsf::AssetProxy(LocalAssetName).getGlobalAssetId(), TargetAssetName);
							auto CachedAsset = mAssetEditHelper->getCachedAsset(qsf::AssetProxy(LocalAssetName).getAsset()->getGlobalAssetId());
							/*if (CachedAsset == nullptr)
								QSF_LOG_PRINTS(INFO, "cached asset is a nullptr")
								else
								QSF_LOG_PRINTS(INFO, "cached asset isnot null?")*/
							mAssetEditHelper->setAssetUploadData(qsf::AssetProxy(LocalAssetName).getAsset()->getGlobalAssetId(), true, true);
						}
					}

				}

			}
			//only submit add end else folder will be gone
			mAssetEditHelper->submit();
			//load Materials but wait until submit is completed
			mAssetEditHelper->callWhenFinishedUploading(boost::bind(&TerrainEditmodeColorMap::SplitMap_waitForSaveTerrain, this, boost::function<void(bool)>(boost::bind(&TerrainEditmodeColorMap::onSplitMap_waitForSave_TerrainDone, this, _1))));

			return true;

		}

		void TerrainEditmodeColorMap::SplitMap_waitForSaveTerrain(boost::function<void(bool)> resultCallback)
		{
			QSF_LOG_PRINTS(INFO, "Created Splitmaps")
				ChangeMaterialToUseSmallMaps((int)0, (int)0);

		}

		void TerrainEditmodeColorMap::onSplitMap_waitForSave_TerrainDone(bool isGood)
		{
			QSF_LOG_PRINTS(INFO, "Created Splitmaps func 2")
				QSF_CHECK(nullptr != mAssetEditHelper, "qsf::editor::TerrainEditHelper::onSaveTerrainDone(): nullptr != mAssetEditHelper", QSF_REACT_THROW);
			mAssetEditHelper->reset();
			QSF_EDITOR_APPLICATION.getAssetImportManager().setDefaultDestinationAssetPackage(m_OldAssetPackage->getProject().getName(), m_OldAssetPackage->getName());
			m_OldAssetPackage = nullptr;
		}

		void TerrainEditmodeColorMap::ChangeMaterialToUseSmallMaps(int x, int y)
		{
			for (int x = 0; x < mParts; x++)
			{
				for (int y = 0; y < mParts; y++)
				{
					int y_mirror = mParts - 1 - y;
					//QSF_LOG_PRINTS(INFO, "x "<< x << " y "<<y<< " "<< GetSmallImageByTerrainId(x, y).second)
					TerrainMaster->UseMiniColorMaps(mParts, x, y_mirror, GetSmallImageByTerrainId(x, y).second);
				}
			}
		}

		std::pair<Magick::Image*, std::string> TerrainEditmodeColorMap::GetSmallImageByTerrainId(int x, int y)
		{
			int index = y + mParts * x;
			if (m_SmallImages.size() <= index)
				return std::pair<Magick::Image*, std::string>(nullptr, "");
			return m_SmallImages.at(index);

		}

		void TerrainEditmodeColorMap::OnFinishEditing()
		{
			//we combine our split maps into one map
			for (int x = 0; x < mParts; x++)
			{
				for (int y = 0; y < mParts; y++)
				{
					//QSF_LOG_PRINTS(INFO,"write map "<<x << " "<< y)
					auto img = GetSmallImageByTerrainId(x, y).first;
					if (img == nullptr)
					{
						QSF_LOG_PRINTS(INFO, "[OnFinishEditing]: error cant read img " << x << " " << y)
							return;
					}
					int xOffset = x*partsize;
					int yOffset = y * partsize;
					//    void copyPixels(const Image &source_,const Geometry &geometry_, const Offset &offset_);
					image->copyPixels(*img, Magick::Geometry(partsize, partsize, 0, 0), Magick::Offset(xOffset, yOffset));
				}
			}
			image->syncPixels();
			auto ColorMapToRead = TerrainMaster->GetColorMap();
			if (ColorMapToRead.getAsset() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no color map is set in terrain component")
					return;
			}
			image->write(ColorMapToRead.getAbsoluteCachedAssetDataFilename());
			if (!TerrainMaster.valid())
			{
				QSF_LOG_PRINTS(INFO, "Couldnt save terrain - terrain component was allready invalid")
					return;
			}
			TerrainMaster->ReloadSubTerrainMaterials(0, 0);
			Ogre::TerrainGroup::TerrainIterator it3 = TerrainMaster->getOgreTerrainGroup()->getTerrainIterator();
			while (it3.hasMoreElements()) // add the layer to all terrains in the terrainGroup
			{
				Ogre::TerrainGroup::TerrainSlot* a = it3.getNext();

				TerrainMaster->RefreshMaterial(a->instance);
			}
		}


		void TerrainEditmodeColorMap::CopyOldMap()
		{
			auto ColorMapToRead = TerrainMaster->GetColorMap();
			if (ColorMapToRead.getAsset() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no color map is set in terrain component")
					return;
			}

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


			image = new Magick::Image(ColorMapToRead.getAbsoluteCachedAssetDataFilename());

			auto image = new Magick::Image(qsf::AssetProxy(ColorMapToRead).getAbsoluteCachedAssetDataFilename());
			if (TerrainEditGUI->GetMirrorX())
			{
				image->flip();
			}
			if (TerrainEditGUI->GetMirrorY())
			{
				image->flop();
			}
			image->syncPixels();

			mAssetEditHelper = std::shared_ptr<qsf::editor::AssetEditHelper>(new qsf::editor::AssetEditHelper());
			auto IAP = mAssetEditHelper->getIntermediateAssetPackage();

				//QSF_LOG_PRINTS(INFO,"for some reasons it exists")
				//ogreImage.save(TerrainMaster->GetNewHeightMap().getAbsoluteCachedAssetDataFilename());
				image->write(TerrainMaster->GetColorMap().getAbsoluteCachedAssetDataFilename());
				std::string TargetAssetName = qsf::AssetProxy(TerrainMaster->GetNewHeightMap()).getAssetPackage()->getName();
				mAssetEditHelper->tryEditAsset(qsf::AssetProxy(TerrainMaster->GetNewHeightMap()).getGlobalAssetId(), TargetAssetName);
				auto CachedAsset = mAssetEditHelper->getCachedAsset(qsf::AssetProxy(TerrainMaster->GetNewHeightMap()).getAsset()->getGlobalAssetId());
				/*if (CachedAsset == nullptr)
				QSF_LOG_PRINTS(INFO, "cached asset is a nullptr")
				else
				QSF_LOG_PRINTS(INFO, "cached asset isnot null?")*/
				mAssetEditHelper->setAssetUploadData(qsf::AssetProxy(TerrainMaster->GetNewHeightMap()).getAsset()->getGlobalAssetId(), true, true);

			// Cleanup

			QSF_LOG_PRINTS(INFO, "Map was saved succesfully")
				mAssetEditHelper->submit();
			//mAssetEditHelper->callWhenFinishedUploading(boost::bind(&TerrainEditTool::WaitForSaveTerrain, this, boost::function<void(bool)>(boost::bind(&TerrainEditTool::onWaitForSave_TerrainDone, this, _1))));
			TerrainMaster->setAllPropertyOverrideFlags(true);
			SplitMapInSmallMaps();
			//
		}
		void TerrainEditmodeColorMap::CopyOldMapWithNoAssetLoaded()
		{
			qsf::GlobalAssetId myID;
			auto TerrainMaster = qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<kc_terrain::TerrainComponent>();
			if(TerrainMaster == nullptr)
			return;
			if (TerrainMaster->GetColorMap().getAsset() != nullptr)
			{
				QSF_LOG_PRINTS(INFO, "pls start the tool first")
				return;
			}
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
			//load qsf terrain for mirroring
			auto ColorMapToRead = CopyFromTerrain->getTerrainDefinition()->getColorMap();
			if (qsf::AssetProxy(ColorMapToRead).getAsset() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no color map is set in terrain component")
					return;
			}
			auto image = new Magick::Image(qsf::AssetProxy(ColorMapToRead).getAbsoluteCachedAssetDataFilename());
			image->magick("DDS");
			image->type(Magick::ImageType::TrueColorAlphaType);
			QSF_LOG_PRINTS(INFO,"read colormap ColorMapToRead"<< qsf::AssetProxy(ColorMapToRead).getAbsoluteCachedAssetDataFilename())
				QSF_LOG_PRINTS(INFO, "rows " << image->rows())
				if (TerrainEditGUI->GetMirrorX())
				{
					image->flip();
				}
			if (TerrainEditGUI->GetMirrorY())
			{
				image->flop();
			}
			image->syncPixels();
			//save the map
			mAssetEditHelper = std::shared_ptr<qsf::editor::AssetEditHelper>(new qsf::editor::AssetEditHelper());
			auto IAP = mAssetEditHelper->getIntermediateAssetPackage();
				try
				{
					auto Name = TerrainMaster->getEntity().getComponent<qsf::MetadataComponent>()->getName();
					auto Mapname= QSF_MAINMAP.getCoreEntity().getComponent<qsf::MapPropertiesBaseComponent>()->getMapName();
					std::transform(Mapname.begin(), Mapname.end(), Mapname.begin(),::tolower); // correct
					Name +="_"+Mapname;
					//b create folder structure in assethelper (to read and write)
					if (!boost::filesystem::exists((QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/colormap")))
						boost::filesystem::create_directories(QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/colormap");
					//c write to new folder -I think it will copy to our direction we wrote before
					//ogreImage.save(QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap/heightmap_" + Name + "_" + TimeStamp +".tif");
					auto relAssetDirectory = QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackage()->getRelativeDirectory();
					if (!boost::filesystem::exists((QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/colormap")))
						boost::filesystem::create_directories(QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/colormap");



					//MagImage->write(QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/heightmap/heightmap_" + Name  +  ".tif");
					QSF_LOG_PRINTS(INFO, "saved ogre img to " << QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/colormap/colormap" + Name + ".dds")
						//d learn our assets a few thing <-> this seems not needed
						//delete[] buffer;
						image->write(QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/colormap/colormap" + Name + ".dds");
						auto Asset = mAssetEditHelper->addAsset(QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackage()->getName(), qsf::QsfAssetTypes::TEXTURE, "colormap", "colormap" + Name);
					
					if (Asset == nullptr)
						QSF_LOG_PRINTS(INFO, "error occured " << Name << " could not create an asset")
					else
					{
						myID = Asset->getGlobalAssetId();
						auto CachedAsset = mAssetEditHelper->getCachedAsset(Asset->getGlobalAssetId());
						if (CachedAsset == nullptr)
							CachedAsset = &qsf::CachedAsset(Asset->getGlobalAssetId());
						if (CachedAsset == nullptr)
						{
							QSF_LOG_PRINTS(INFO, "still a nullptr")
						}
						CachedAsset->setType("dds");
						//QSF_LOG_PRINTS(INFO, qsf::AssetProxy(Asset->getGlobalAssetId()).getAbsoluteCachedAssetDataFilename())

						if (mAssetEditHelper->setAssetUploadData(Asset->getGlobalAssetId(), true, true))
							QSF_LOG_PRINTS(INFO, "Caching asset was not succesfull")
					}
					
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO, e.what())
				}
				mAssetEditHelper->submit();
				//update materials
				TerrainMaster->SetNewColorMap(qsf::AssetProxy(myID));
				TerrainMaster->setAllPropertyOverrideFlags(true);
				TerrainMaster->ReloadSubTerrainMaterials(0, 0);
				Ogre::TerrainGroup::TerrainIterator it3 = TerrainMaster->getOgreTerrainGroup()->getTerrainIterator();
				while (it3.hasMoreElements()) // add the layer to all terrains in the terrainGroup
				{
					Ogre::TerrainGroup::TerrainSlot* a = it3.getNext();

					TerrainMaster->RefreshMaterial(a->instance);
				}
		}
		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
