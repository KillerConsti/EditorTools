// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/indicator/TerrainEditmodeColorMap.h"
#include <asset_collector_tool\qsf_editor\tools\TerrainEditColorMapToolbox.h>
#include "ui_TerrainEditmodeColorMap.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)

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
#include <qsf/renderer/material/material/MaterialManager.h>
#include <qsf/renderer/material/MaterialSystem.h>

#include <qsf/debug/DebugDrawManager.h>

#include <chrono>
using namespace std::chrono;
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace detail
{


	//[-------------------------------------------------------]
	//[ Global functions                                      ]
	//[-------------------------------------------------------]
	void setIdentityMapTransformMaterialProperty(qsf::Material& material, qsf::MaterialPropertyId materialPropertyId)
	{
		material.setPropertyById(materialPropertyId, qsf::MaterialPropertyValue::fromFloat4(0.0f, 0.0f, 1.0f, 1.0f));
	}

	void setMapTransformMaterialProperty(qsf::Material& material, const qsf::TerrainComponent& terrainComponent, const Ogre::Terrain& ogreTerrain, qsf::MaterialPropertyId materialPropertyId, bool scale)
	{
		// TODO(co) Proper texture coordinate offset generation
		const float globalTerrainWorldSize = terrainComponent.getTerrainWorldSize();
		const int terrainChunksPerEdge = terrainComponent.getTerrainChunksPerEdge();
		float xOffset = ogreTerrain.getPosition().x - ogreTerrain.getWorldSize() * 0.5f + globalTerrainWorldSize * 0.5f;
		float yOffset = ogreTerrain.getPosition().z - ogreTerrain.getWorldSize() * 0.5f + globalTerrainWorldSize * 0.5f;
		xOffset /= globalTerrainWorldSize;
		yOffset /= globalTerrainWorldSize;
		if (scale)
		{
			material.setPropertyById(materialPropertyId, qsf::MaterialPropertyValue::fromFloat4(xOffset, yOffset, 1.0f / terrainChunksPerEdge, 1.0f / terrainChunksPerEdge));
		}
		else
		{
			material.setPropertyById(materialPropertyId, qsf::MaterialPropertyValue::fromFloat4(xOffset, yOffset, 1.0f, 1.0f));
		}
	}

	void setLayerBlendMapComponentUvMultiplier(const Ogre::Terrain& ogreTerrain, uint32 layerIndex, const std::string& layerIndexAsString, qsf::Material& terrainMaterial)
	{
		// Gather data
		const int   blendMapIndex = (layerIndex - 1) / 4;
		const int   blendMapComponent = (layerIndex - 1) % 4;
		const float uvMultiplier = ogreTerrain.getLayerUVMultiplier(layerIndex);

		// Set terrain material property value
		terrainMaterial.setPropertyById(qsf::StringHash("LayerBlendMapComponentUvMultiplier" + layerIndexAsString), qsf::MaterialPropertyValue::fromFloat3(static_cast<float>(blendMapIndex), static_cast<float>(blendMapComponent), uvMultiplier));
	}

	void setTerrainLayerMaterialProperties(const qsf::MaterialProperties& layerMaterialProperties, const std::string& layerIndexAsString, qsf::Material& terrainMaterial)
	{
		// Gather parameters for maximum height, blend falloff, visible threshold and parallax scale
		float maximumHeight = 0.0f;
		float blendFalloff = 0.1f;
		float parallaxOffset = 0.5f;
		float parallaxScale = 0.5f;
		const qsf::MaterialPropertyValue* layerMaterialPropertyValue = layerMaterialProperties.getPropertyById("MaximalHeight");	// TODO(co) Sadly, "MaximalHeight" is out there used in the material assets. Should have been "MaximumHeight".
		if (nullptr != layerMaterialPropertyValue)
		{
			maximumHeight = layerMaterialPropertyValue->getFloatValue();
		}
		layerMaterialPropertyValue = layerMaterialProperties.getPropertyById("BlendFalloff");
		if (nullptr != layerMaterialPropertyValue)
		{
			blendFalloff = layerMaterialPropertyValue->getFloatValue();
		}
		layerMaterialPropertyValue = layerMaterialProperties.getPropertyById("ParallaxOffset");
		if (nullptr != layerMaterialPropertyValue)
		{
			parallaxOffset = layerMaterialPropertyValue->getFloatValue();
		}
		layerMaterialPropertyValue = layerMaterialProperties.getPropertyById("ParallaxScale");
		if (nullptr != layerMaterialPropertyValue)
		{
			parallaxScale = layerMaterialPropertyValue->getFloatValue();
		}

		// Set terrain material property value
		terrainMaterial.setPropertyById(qsf::StringHash("LayerMaterialProperties" + layerIndexAsString), qsf::MaterialPropertyValue::fromFloat4(maximumHeight, blendFalloff, parallaxOffset, parallaxScale));
	}


	//[-------------------------------------------------------]
	//[ Anonymous detail namespace                            ]
	//[-------------------------------------------------------]
} // detail
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
			glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(oldmouspoint.x, oldmouspoint.z));
			Mappoint = Mappoint*ColorMapSize;
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
			if(image == nullptr)
			{
				//QSF_LOG_PRINTS(INFO, "image is null")
					PaintJobProxy.unregister();
				return;
			}
			float TotalRadius = Radius * Scale;
			float BrushIntensity = TerrainEditGUI->GetBrushIntensity()*0.25f;
			int MapPointMinX = glm::clamp((int)glm::ceil(MapPoint.x - TotalRadius), 0, (int)ColorMapSize);
			int MapPointMaxX = glm::clamp((int)glm::floor(MapPoint.x + TotalRadius), 0, (int)ColorMapSize);
			int MapPointMinY = glm::clamp((int)glm::ceil(MapPoint.y - TotalRadius), 0, (int)ColorMapSize);
			int MapPointMaxY = glm::clamp((int)glm::floor(MapPoint.y + TotalRadius), 0, (int)ColorMapSize);
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
			glm::vec2 ImgPoint = point;
			auto OldColor = GetOldColor(ImgPoint);
			//QSF_LOG_PRINTS(INFO, "Old Color" << "Red" << (int)(OldColor.r * 256) << " green " << (int)(OldColor.g * 256) << " Blue " << (int)(OldColor.b * 256) << " Alpha " << (int)(OldColor.a))
			//how to mix colors?
			auto NewColorASRGBA = TerrainEditGUI->GetSelectedColor();
			qsf::Color4 NewColor = qsf::Color4((float)NewColorASRGBA.red(), (float)NewColorASRGBA.blue(), (float)NewColorASRGBA.green(), (float)NewColorASRGBA.alpha());
			//QSF_LOG_PRINTS(INFO, NewColor)
			if(NewColor.a != 0)
			{
			NewColor = NewColor/256.f;
			//ignore alpha for now
			//QSF_LOG_PRINTS(INFO, "NewColor" << "Red" << (int)(NewColor.r * 256) << " green " << (int)(NewColor.g * 256) << " Blue " << (int)(NewColor.b * 256))
			OldColor.r = (OldColor.r*20.f+NewColor.r*Intensity) /(20.f+Intensity);
			//if(glm::abs(OldColor.r-NewColor.r) <= 1)
			//OldColor.r = NewColor.r;

			OldColor.g = (OldColor.g*20.f + NewColor.g*Intensity) / (20.f + Intensity);
			//if (glm::abs(OldColor.g - NewColor.g) <= 1)
				//OldColor.g = NewColor.g;

			OldColor.b = (OldColor.b*20.f + NewColor.b*Intensity) / (20.f + Intensity);
			//if (glm::abs(OldColor.b - NewColor.b) <= 1)
				//OldColor.b = NewColor.b;
			}
			else
			{
				OldColor.a = 0.f;
				OldColor.r =0.f;
				OldColor.g = 0.f;
				OldColor.b =0.f;
			}
				/*INT = 100%
				[QSF info] Old ColorRed3 green 53 Blue 2 Alpha 1
				[QSF info] NewColorRed0 green 255 Blue 0
				[QSF info] Mixed ColorRed2 green 53 Blue 2 Alpha 1

				53*100+255*100
				3700/200 = 18,5

				
				
				*/
				PaintMap* PM = new PaintMap();
				PM->Pixel = ImgPoint;
				PM->NewColor = OldColor;
				PaintedPixels.push_back(PM);
				NeedUpdates.push_back(Terrains(xTerrain,yTerrain));
			//QSF_LOG_PRINTS(INFO,"Mixed Color" << "Red" << (int)(OldColor.r * 256) << " green " << (int)(OldColor.g * 256) << " Blue " << (int)(OldColor.b * 256) << " Alpha " << (int)(OldColor.a))
			return true;


		}

		qsf::Color4 TerrainEditmodeColorMap::GetOldColor(glm::vec2& MapPoint)
		{
			//QSF_LOG_PRINTS(INFO,MapPoint)
			int col = (int)image->columns();
			int row = (int)image->rows();
			int channels = (int)image->channels();
			//editor img is flipped so we need to switch Mappoint.y
			MapPoint.y = row-(int)MapPoint.y;
			MagickCore::Quantum *pixels = image->getPixels(0, 0, col, row);
			if (channels == 3) //rgb
			{
				uint64 offset = (row* MapPoint.y + MapPoint.x) * channels; //4 is because we use 4 channels
				float Red = (float)(*(pixels + offset) / 65535);
				float Green = (float)(*(pixels + offset + 1) / 65535);
				float Blue = (float)(*(pixels + offset + 2) / 65535);
				//QSF_LOG_PRINTS(INFO,"Red"<< Red << " green "<< Green << " Blue "<< Blue)
				return qsf::Color4(Red,Green,Blue,1.f);
			}
			else if (channels == 4) //rgba
			{
				uint64 offset = (row* MapPoint.y + MapPoint.x) * channels; //4 is because we use 4 channels
				float Red = (float)(*(pixels + offset) / 65535);
				float Green = (float)(*(pixels + offset + 1) / 65535);
				float Blue = (float)(*(pixels + offset + 2) / 65535);
				float Alpha = (float)(*(pixels + offset + 3) / 65535);
				//QSF_LOG_PRINTS(INFO, "Red" << (int)(Red*256) << " green " << (int)(Green*256) << " Blue " << (int)(Blue*256)<< " Alpha " << (int)(Alpha))
				return qsf::Color4(Red, Green, Blue, Alpha);
			}
			return qsf::Color4();


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

		glm::vec2 TerrainEditmodeColorMap::ConvertWorldPointToRelativePoint(glm::vec2 WorldPoint)
		{
			glm::vec2 copy = WorldPoint;
			copy = (WorldPoint + Offset) / TerrainMaster->getTerrainWorldSize();
			copy.y = 1.f - copy.y; //we need to mirror Y
			return copy;
		}

	

		void TerrainEditmodeColorMap::UpdateTerrains()
		{
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
			MagickCore::Quantum *pixels = image->getPixels(0, 0, col, row);
			if (channels == 3) //rgb
			{
				for(auto a : PaintedPixels)
				 {
				uint64 offset = (row* a->Pixel.y + a->Pixel.x) * channels; //4 is because we use 4 channels
				*(pixels + offset) = a->NewColor.r* 65535;
				*(pixels + offset + 1) = a->NewColor.g* 65535;
				*(pixels + offset + 2) = a->NewColor.b* 65535;
				}
			}
			else if (channels == 4) //rgba
			{
				for (auto a : PaintedPixels)
				{
					uint64 offset = (row* a->Pixel.y + a->Pixel.x) * channels; //4 is because we use 4 channels
					*(pixels + offset) = a->NewColor.r* 65535;
					*(pixels + offset + 1) = a->NewColor.g* 65535;
					*(pixels + offset + 2) = a->NewColor.b* 65535;
					*(pixels + offset + 3) = a->NewColor.a * 65535;
				}
			}
			image->syncPixels();
			image->write(ColorMapToRead.getAbsoluteCachedAssetDataFilename());
			//auto stop = high_resolution_clock::now();
			//auto duration = std::chrono::duration_cast<milliseconds>(stop - start);
			//QSF_LOG_PRINTS(INFO,"time to save img" <<duration.count())
			//we should make sure that we have a 4 x 4 Terrain-pattern (16 tiles).
			//we assume it here by t = x and y = i
			std::sort(NeedUpdates.begin(),NeedUpdates.end());
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
			TerrainMaster->ReloadSubTerrainMaterials(0,0);
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
		QSF_LOG_PRINTS(INFO, "Colormapsize "<< TerrainMaster->getColorMapSize());
		QSF_LOG_PRINTS(INFO, "partsize" << TerrainMaster->getOgreTerrainGroup()->getTerrainSize());

		Offset = (float)(TerrainMaster->getTerrainWorldSize() / 2);
		percentage = 1.f / (float)TerrainMaster->getTerrainWorldSize(); /// (float)TerrainMaster->getHeightMapSize();
		ColorMapSize = (float)TerrainMaster->getColorMapSize();
		partsize = TerrainMaster->getOgreTerrainGroup()->getTerrainSize() - 1;

		if (!PaintJobProxy.isValid())
			PaintJobProxy.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&TerrainEditmodeColorMap::PaintJob, this, _1));
		QSF_LOG_PRINTS(INFO, "scale" << ColorMapSize / TerrainMaster->getTerrainWorldSize() << " units per meter");
		Scale = ColorMapSize / TerrainMaster->getTerrainWorldSize();

		Ogre::TerrainGroup::TerrainIterator it = TerrainMaster->getOgreTerrainGroup()->getTerrainIterator();
		int counter = 0; // because my ID start at 0
		while (it.hasMoreElements()) // add the layer to all terrains in the terrainGroup
		{
			Ogre::TerrainGroup::TerrainSlot* a = it.getNext();
			counter++;
		}

		partsize = ColorMapSize / sqrt(counter);
		mParts = sqrt(counter);
		QSF_LOG_PRINTS(INFO, "we have " << mParts << " x " << mParts << "  Parts");
		QSF_LOG_PRINTS(INFO, "started  TerrainEditmodeColorMap")
			for (uint8 t = 0; t <= TerrainMaster->getOgreTerrainGroup()->getTerrain(0, 0)->getLayerCount(); t++)
			{
				QSF_LOG_PRINTS(INFO, TerrainMaster->getOgreTerrainGroup()->getTerrain(0, 0)->getLayerTextureName(t, 0));

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
			QSF_LOG_PRINTS(INFO,"buffer image is a nullptr")
			return false;
		}
		return true;
	}

	void TerrainEditmodeColorMap::onShutdown(EditMode * nextEditMode)
	{
		EditMode::onShutdown(nextEditMode);
		PaintJobProxy.unregister();
		mDebugDrawProxy.unregister();
		QSF_LOG_PRINTS(INFO, "Shutdown")
	}


	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
} // editor
} // user
