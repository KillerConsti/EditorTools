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
#include <sstream>
#include <qsf_editor\asset\terrain\TerrainEditHelper.h>
#include <qsf/renderer/material/material/MaterialManager.h>
#include <qsf/renderer/material/MaterialSystem.h>

#include "qsf/renderer/RendererSystem.h"
#include "qsf/settings/CompositingSettingsGroup.h"
#include <qsf/renderer/terrain/TerrainDefinition.h>

#include "qsf/asset/Asset.h"
#include "qsf/asset/AssetSystem.h"
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
			QSF_LOG_PRINTS(INFO, "start Edit Mode Color Map")
		}


		TerrainEditmodeColorMap::~TerrainEditmodeColorMap()
		{
			QSF_LOG_PRINTS(INFO, "left Edit Mode Color Map")
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
				for (qsf::TerrainComponent* terrainComponent : qsf::ComponentMapQuery(QSF_MAINMAP).getAllInstances<qsf::TerrainComponent>())
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
			mDebugDrawProxy.registerAt(QSF_MAINMAP.getDebugDrawManager());
			mDebugDrawProxy.addRequest(qsf::CircleDebugDrawRequest(oldmouspoint, qsf::CoordinateSystem::getUp(), Radius, qsf::Color4::GREEN));
			if (!mouseisvalid)
				return;

			if (!QSF_INPUT.getMouse().Left.isPressed())
				return;
			glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(oldmouspoint.x, oldmouspoint.z));
			Mappoint = Mappoint*Heighmapsize;
			/*switch (TerrainEditGUI->GetEditMode()) //Special Handlings
			{
			case 0: //Set
			{*/
			if (yo_mousepoint == oldmouspoint)
				return;
			yo_mousepoint = oldmouspoint;
			SetHeight(Mappoint);
			return;
			/*}
			case 1: //Raise
			{
				RaiseTerrain(Mappoint);
			}
			case 2: //Smooth
			{
				SmoothMap(Mappoint);
			}
			}*/

		}

		float TerrainEditmodeColorMap::GetCustomIntensity(float distancetoMidpoint, TerrainEditColorMapToolbox::TerrainEditMode2 Mode)
		{
			return 0.0f;
		}

		void TerrainEditmodeColorMap::RaiseTerrain(glm::vec2 MapPoint)
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

		void TerrainEditmodeColorMap::RaisePoint(glm::vec2 point, float Intensity)
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
			float newIntensity = glm::clamp(Intensity* 0.5f, 0.f, 5.f);
			Terrain->setHeightAtPoint(xRemaining, yRemaining, Terrain->getHeightAtPoint(xRemaining, yRemaining) + Intensity);
		}

		void TerrainEditmodeColorMap::IncreaseHeight(glm::vec2 point, float NewHeight)
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

			//Terrain->setHeightAtPoint(xRemaining, yRemaining, NewHeight);//TerrainEditGUI->GetHeight());
			for (uint8 t = 0; t <= Terrain->getLayerCount(); t++)
			{
				QSF_LOG_PRINTS(INFO, Terrain->getLayerTextureName(t, 0));
			}
		}


		void TerrainEditmodeColorMap::SetHeight(glm::vec2 MapPoint)
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
						return;
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
						return;
					}
				}
			}
			UpdateTerrains();
		}


		glm::vec3 TerrainEditmodeColorMap::ApplySmooth(glm::vec2 Point, float Intensity)
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





		void TerrainEditmodeColorMap::SaveMap(const qsf::MessageParameters & parameters)
		{
			SaveTheFuckingMap();
		}

		void TerrainEditmodeColorMap::SaveTheFuckingMap()
		{
			if (TerrainEditGUI == nullptr)
				return;
			if (TerrainMaster.get() == nullptr)
				return;
			auto path = TerrainEditGUI->GetSavePath();
			//scalemap
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
			highestpoint = highestpoint - lowestpoint;

			uint8* buffer = new uint8[Heighmapsize * Heighmapsize * 8];
			Ogre::Image ogreImage;
			ogreImage.loadDynamicImage(buffer, Heighmapsize, Heighmapsize, Ogre::PixelFormat::PF_FLOAT32_GR);
			for (size_t t = 0; t < Heighmapsize - 1; t++)
			{
				for (size_t j = 0; j < Heighmapsize - 1; j++)
				{
					float value = ReadValue(glm::vec2(t, j) - lowestpoint) / highestpoint;
					const Ogre::ColourValue ogreColorValue = Ogre::ColourValue(value, value, value);
					ogreImage.setColourAt(ogreColorValue, t, j, 0);
				}
				//ogreImage.setColourAt()
			}
			ogreImage.flipAroundX();
			uint64 globalMapAssetId = QSF_MAINMAP.getGlobalAssetId();
			if (qsf::AssetProxy(globalMapAssetId).getGlobalAssetId() == qsf::getUninitialized<qsf::GlobalAssetId>())
			{
				ogreImage.save(path + "\\heightmap___min__" + low + "__max__" + height + "__date__" + GetCurrentTimeForFileName() + ".tif");
			}
			else
			{
				auto mapname = qsf::AssetProxy(globalMapAssetId).getLocalAssetName();
				std::vector<std::string> splittedString;
				boost::split(splittedString, mapname, boost::is_any_of("/"), boost::token_compress_on);
				if (!splittedString.empty())
				{
					QSF_LOG_PRINTS(INFO, "Mapname " << splittedString.at(splittedString.size() - 1));
					ogreImage.save(path + "\\heightmap__" + splittedString.at(splittedString.size() - 1) + "__min__" + low + "__max__" + height + "__date__" + GetCurrentTimeForFileName() + ".tif");
				}
				else
					ogreImage.save(path + "\\heightmap___min__" + low + "__max__" + height + "__date__" + GetCurrentTimeForFileName() + ".tif");
			}


			// Cleanup
			delete[] buffer;
			QSF_LOG_PRINTS(INFO, "Map was saved succesfully")
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

		float TerrainEditmodeColorMap::ReadValue(glm::vec2 point)
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

		inline void TerrainEditmodeColorMap::mousePressEvent(QMouseEvent & qMouseEvent)
		{
			if (Qt::RightButton == qMouseEvent.button() && TerrainEditGUI != nullptr && TerrainEditGUI->GetEditMode() == TerrainEditGUI->Set)
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

		glm::vec2 TerrainEditmodeColorMap::ConvertMappointToWorldPoint(glm::vec2 Mappoint)
		{
			glm::vec2 copy = Mappoint;
			copy = copy / (float)Heighmapsize;
			copy = copy * TerrainMaster->getTerrainWorldSize();
			copy = copy - Offset;
			copy.y = -1.f * copy.y;
			return copy;
		}

		void TerrainEditmodeColorMap::UpdateTerrains()
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




		void TerrainEditmodeColorMap::generateMaterial()
		{
			
			int LayerCount = TerrainMaster->getOgreTerrain()->getMaxLayers();
			QSF_LOG_PRINTS(INFO, LayerCount)
			for (size_t t = 0; t < LayerCount + 2; t++)
			{
				int Samplerindex = 0;
				for(auto a : TerrainMaster->getOgreTerrain()->getLayerDeclaration().samplers)
				{
					
					QSF_LOG_PRINTS(INFO,"layer" << t << " sampler" << Samplerindex)
					QSF_LOG_PRINTS(INFO, TerrainMaster->getOgreTerrain()->getLayerTextureName((uint8)t,Samplerindex).c_str() << " " << a.alias.c_str())
						Samplerindex++;
					}
			}
			if(TerrainMaster->getOgreTerrain()->getMaterial().isNull())
			QSF_LOG_PRINTS(INFO,"is null")
			//TerrainMaster->getOgreTerrain()->getLayerTextureName(0,0)
			return;
			int _materialNum =1;
			Ogre::String _materialName = "materialOgreLine";
			_materialName.append(Ogre::StringConverter::toString(_materialNum));
			Ogre::MaterialPtr matTest = Ogre::MaterialManager::getSingleton().getByName(_materialName);
			while (!matTest.isNull())
			{
				_materialName.append(Ogre::StringConverter::toString(_materialNum));
				matTest = Ogre::MaterialManager::getSingleton().getByName(_materialName);
			}

			Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(_materialName.c_str(),
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			int r =50;
			int g = 100;
			int b = 150;
			float rf = r / 255;
			float gf = g / 255;
			float bf = b / 255;
			mat->setAmbient(rf, gf, bf);
			mat->setDiffuse(rf, gf, bf, 1);
			mat->setSpecular(0, 0, 0, 1);

			/*auto Mat = TerrainMaster->getOgreTerrainGroup()->getTerrain(0,0)->;
			if(Mat == nullptr)
			QSF_LOG_PRINTS(INFO,"null")
			else
				QSF_LOG_PRINTS(INFO, "not null")*/
				}
		/*	Ogre::TerrainMaterialGeneratorPtr terrainMaterialGenerator(OGRE_NEW Ogre::TerrainMaterialGenerator);
			terrainMaterialGenerator->
			//TerrainMaster-> 
			terrainMaterialGenerator.bind(OGRE_NEW Ogre::TerrainMaterialGenerator,)
			// Set Ogre Material  with the name "TerrainMaterial" in constructor
			TerrainMaterial *terrainMaterial = OGRE_NEW Ogre::TerrainMaterial("TerrainMaterial");
			terrainMaterialGenerator.bind(terrainMaterial);

			TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i)->mat*/
		//}
			/*TerrainMaster->getOgreTerrainGroup()->mat
			//const Ogre::String& matName, const Ogre::Terrain* ogreTerrain)

			const Ogre::String& matName = "New Terrain";
			auto ogreTerrain = TerrainMaster->getOgreTerrain();
			static qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
			QSF_ASSERT(matName == ogreTerrain->getMaterialName(), "qsf::TerrainMaterialGenerator::Profile::createMaterial(): OGRE terrain material name mismatch", QSF_REACT_NONE);

			// In case the terrain material instance is already there, just update it
			qsf::Material* terrainMaterial = materialManager.findElement(qsf::StringHash(matName));
			if (nullptr == terrainMaterial)
			{
				// Create terrain material instance
				terrainMaterial = materialManager.createMaterial(matName, qsf::StringHash("qsf_terrain"));
				QSF_CHECK(nullptr != terrainMaterial, "QSF failed to create QSF terrain material " << matName << " instance", return);
			}

			// Apply terrain quality setting
			qsf::compositing::CompositingSettingsGroup::getInstanceSafe().applyTerrainQualityToMaterial(*terrainMaterial);

			// Asset relevant information
			// TODO(co) Don't use QSF_MAIMAP in here. Support multiple terrains per map.
			const qsf::Map& map = QSF_MAINMAP;
			const qsf::TerrainComponent* terrainComponent = qsf::ComponentMapQuery(map).getFirstInstance<qsf::TerrainComponent>();
			QSF_CHECK(nullptr != terrainComponent, "There are no terrain component instances inside the map", return);

			// Global color map
			if (ogreTerrain->getGlobalColourMapEnabled() && !ogreTerrain->getGlobalColourMap().isNull())
			{
				// Global color map transform
				::detail::setIdentityMapTransformMaterialProperty(*terrainMaterial, "GlobalColorMapTransform");

				// Set color map
				terrainMaterial->setPropertyById("UseGlobalColorMap", qsf::MaterialPropertyValue::fromBoolean(true));
				terrainMaterial->setPropertyById("GlobalColorMap", qsf::MaterialPropertyValue::fromResourceName(ogreTerrain->getGlobalColourMap()->getName()));
			}
			else
			{
				// Global color map transform
				::detail::setMapTransformMaterialProperty(*terrainMaterial, *terrainComponent, *ogreTerrain, "GlobalColorMapTransform", true);

				// A global color map which spans all terrain chunks, usually only used during runtime for efficiency
				static const qsf::GlobalAssetId missingTextureGlobalAssetId = qsf::AssetProxy("qsf/texture/default/missing").getGlobalAssetId();
				const qsf::TerrainDefinition* terrainDefinition = terrainComponent->getTerrainDefinition();
				qsf::GlobalAssetId globalAssetId = (nullptr != terrainDefinition && terrainDefinition->isValid()) ? terrainDefinition->getColorMap() : qsf::getUninitialized<qsf::GlobalAssetId>();
				if (qsf::isUninitialized(globalAssetId) || nullptr == QSF_ASSET.getAssetByGlobalAssetId(globalAssetId))
				{
					globalAssetId = missingTextureGlobalAssetId;
				}
				terrainMaterial->setPropertyById("UseGlobalColorMap", qsf::MaterialPropertyValue::fromBoolean(qsf::isInitialized(globalAssetId)));
				terrainMaterial->setPropertyById("GlobalColorMap", qsf::MaterialPropertyValue::fromGlobalAssetId(globalAssetId));
			}

			// Global normal map
			if (ogreTerrain->getTerrainNormalMap().isNull())
			{
				// Global normal map transform
				::detail::setMapTransformMaterialProperty(*terrainMaterial, *terrainComponent, *ogreTerrain, "GlobalNormalMapTransform", true);

				// A global normal map which spans all terrain chunks, usually only used during runtime for efficiency
				const qsf::TerrainDefinition* terrainDefinition = terrainComponent->getTerrainDefinition();
				if (nullptr != terrainDefinition && terrainDefinition->isValid())
				{
					// Just use the global asset ID and let our resource management do the rest...
					qsf::GlobalAssetId globalAssetId = terrainDefinition->getNormalMap();
					if (nullptr == QSF_ASSET.getAssetByGlobalAssetId(globalAssetId))
					{
						qsf::setUninitialized(globalAssetId);
					}
					terrainMaterial->setPropertyById("UseGlobalNormalMap", qsf::MaterialPropertyValue::fromBoolean(qsf::isInitialized(globalAssetId)));
					terrainMaterial->setPropertyById("GlobalNormalMap", qsf::MaterialPropertyValue::fromGlobalAssetId(globalAssetId));
				}
				else
				{
					terrainMaterial->setPropertyById("UseGlobalNormalMap", qsf::MaterialPropertyValue::fromBoolean(false));
					terrainMaterial->setPropertyById("GlobalNormalMap", qsf::MaterialPropertyValue::fromGlobalAssetId(qsf::getUninitialized<qsf::GlobalAssetId>()));
				}
			}
			else
			{
				// Global normal map transform
				::detail::setIdentityMapTransformMaterialProperty(*terrainMaterial, "GlobalNormalMapTransform");

				// Set normal map
				terrainMaterial->setPropertyById("UseGlobalNormalMap", qsf::MaterialPropertyValue::fromBoolean(true));
				terrainMaterial->setPropertyById("GlobalNormalMap", qsf::MaterialPropertyValue::fromResourceName(ogreTerrain->getTerrainNormalMap()->getName()));
			}

			// Layers: UV layer offset and terrain size
			::detail::setMapTransformMaterialProperty(*terrainMaterial, *terrainComponent, *ogreTerrain, "UvLayerTransform", false);
			terrainMaterial->setPropertyById("TerrainWorldSize", qsf::MaterialPropertyValue::fromFloat(ogreTerrain->getWorldSize()));

			// Number of layers and number of blend maps
			const uint32 maximumNumberOfLayers = 16;//getMaxLayers(ogreTerrain); //kc
			const uint32 numberOfBlendMaps = std::min(ogreTerrain->getBlendTextureCount(maximumNumberOfLayers), ogreTerrain->getBlendTextureCount());
			uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(ogreTerrain->getLayerCount()));
			terrainMaterial->setPropertyById("NumberOfLayers", qsf::MaterialPropertyValue::fromInteger(numberOfLayers));
			terrainMaterial->setPropertyById("NumberOfBlendMaps", qsf::MaterialPropertyValue::fromInteger(numberOfBlendMaps));

			// Blend maps
			for (uint32 i = 0; i < numberOfBlendMaps; ++i)
			{
				terrainMaterial->setPropertyById(qsf::StringHash("BlendMap" + std::to_string(i)), qsf::MaterialPropertyValue::fromResourceName(ogreTerrain->getBlendTextureName(i)));
			}

			// Texture layers
			for (uint32 layerIndex = 0; layerIndex < numberOfLayers; ++layerIndex)
			{
				const std::string layerIndexAsString = std::to_string(layerIndex);
				bool layerCreated = false;

				// Inside the first texture name of the terrain layer we store the global asset ID of the QSF material the terrain layer is using, we need nothing more
				const std::string globalAssetIdAsString = ogreTerrain->getLayerTextureName(layerIndex, 0);
				static const qsf::AssetSystem& assetSystem = QSF_ASSET;
				const qsf::GlobalAssetId globalAssetId = assetSystem.globalAssetIdAsStringToGlobalAssetId(globalAssetIdAsString);
				if (nullptr != assetSystem.getAssetByGlobalAssetId(globalAssetId))
				{
					const qsf::Material* layerMaterial = materialManager.findElement(qsf::StringHash(globalAssetIdAsString));
					if (nullptr != layerMaterial)
					{
						// Diffuse height
						const qsf::MaterialPropertyValue* layerMaterialPropertyValue = layerMaterial->getPropertyById("_crgb_ha");
						if (nullptr != layerMaterialPropertyValue)
						{
							terrainMaterial->setPropertyById(qsf::StringHash("_crgb_ha_" + layerIndexAsString), *layerMaterialPropertyValue);
						}
						else
						{
							QSF_ERROR("Broken material without _crgb_ha texture", QSF_REACT_NONE);
						}

						// Normal specular gloss
						layerMaterialPropertyValue = layerMaterial->getPropertyById("_nag_sr_gb");
						if (nullptr != layerMaterialPropertyValue)
						{
							terrainMaterial->setPropertyById(qsf::StringHash("_nag_sr_gb_" + layerIndexAsString), *layerMaterialPropertyValue);
						}
						else
						{
							QSF_ERROR("Broken material without _nag_sr_gb texture", QSF_REACT_NONE);
						}

						{ // World size
							float worldSize = 1.0f;
							layerMaterialPropertyValue = layerMaterial->getPropertyById("WorldSize");
							if (nullptr != layerMaterialPropertyValue)
							{
								worldSize = layerMaterialPropertyValue->getFloatValue();
							}
							// TODO(co) We might want to optimize this by e.g. directly setting the shader parameter instead of going over the OGRE terrain instance
							// TODO(tl) We should check how we can calculate UVMultiplier ourself from world size when we have time
							const_cast<Ogre::Terrain*>(ogreTerrain)->setLayerWorldSize(layerIndex, worldSize);
						}

						// Set terrain layer material properties
						::detail::setLayerBlendMapComponentUvMultiplier(*ogreTerrain, layerIndex, layerIndexAsString, *terrainMaterial);
						::detail::setTerrainLayerMaterialProperties(layerMaterial->getMaterialProperties(), layerIndexAsString, *terrainMaterial);

						// Layer has been created successfully
						layerCreated = true;
					}
					else
					{
						QSF_ERROR("Terrain layer material " << globalAssetIdAsString << " not found, restart editor, ignore this error and use terrain tool to reevaluate all layer", QSF_REACT_NONE);
					}
				}
				else
				{
					QSF_ERROR("Terrain layer asset " << globalAssetId << " not found, restart editor, ignore this error and use terrain tool to reevaluate all layer", QSF_REACT_NONE);
				}

				// Error handling: The show must go on
				if (!layerCreated)
				{
					// If one layer is defective, we can't use the following layers or we end up in a material-shader chaos
					numberOfLayers = layerIndex;
					terrainMaterial->setPropertyById("NumberOfLayers", qsf::MaterialPropertyValue::fromInteger(numberOfLayers));
					layerIndex = static_cast<int>(numberOfLayers);
				}
			}

			//TerrainMaster->getOgreEntity()->setMaterial(terrainMaterial->)
			//now load
			//auto mTerrainGlobals = new Ogre::TerrainGlobalOptions();
			/*
			{ // Terrain material generator
				Ogre::TerrainMaterialGeneratorPtr terrainMaterialGeneratorPtr;
				terrainMaterialGeneratorPtr.bind(OGRE_NEW TerrainMaterialGenerator());
				mTerrainGlobals->setDefaultMaterialGenerator(terrainMaterialGeneratorPtr);
				mTerrainGlobals->setQueryFlags(0);	// Don't allow it to e.g. pick the terrain
			}*/
			
			// Create the texture

/*const std::string materialName = "qsf_unlit_mesh";
{
	// Check and (if necessary) create OGRE material
	static qsf::MaterialManager& materialManager = QSF_MATERIAL.getMaterialManager();
	qsf::Material* material = materialManager.findElement(qsf::StringHash(materialName));
	if (nullptr == material)
	{
		material = materialManager.createMaterial(materialName, qsf::StringHash("qsf_unlit_mesh"));
	}
	if (nullptr != material)
	{
		material->setPropertyById("DrawInClipSpace", qsf::MaterialPropertyValue::fromBoolean(true));
		material->setPropertyById("UseColorMap", qsf::MaterialPropertyValue::fromBoolean(true));
		material->setPropertyById("ColorMap", qsf::MaterialPropertyValue::fromResourceName("em5/gui/cursor/interaction/.dds"));
	}
}

			QSF_LOG_PRINTS(INFO,"a")
			Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual(
				"kc_DynamicTexture", // name
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				Ogre::TEX_TYPE_2D,      // type
				256, 256,         // width & height
				0,                // number of mipmaps
				Ogre::PF_BYTE_BGRA,     // pixel format
				Ogre::TU_DEFAULT);      // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
								  // textures updated very often (e.g. each frame)

								  // Get the pixel buffer
			QSF_LOG_PRINTS(INFO, "b")
			Ogre::v1::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();

			// Lock the pixel buffer and get a pixel box
			pixelBuffer->lock(Ogre::v1::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
			const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();
			QSF_LOG_PRINTS(INFO, "c")
			uint8* pDest = static_cast<uint8*>(pixelBox.data);
			QSF_LOG_PRINTS(INFO, "d")
			// Fill in some pixel data. This will give a semi-transparent blue,
			// but this is of course dependent on the chosen pixel format.
			for (size_t j = 0; j < 256; j++)
			{
				for (size_t i = 0; i < 256; i++)
				{
					*pDest++ = 255; // B
					*pDest++ = 255; // G
					*pDest++ = 0; // R
					*pDest++ = 0; // A
				}

				pDest += pixelBox.getRowSkip() * Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
			}
			QSF_LOG_PRINTS(INFO, "e")
			// Unlock the pixel buffer
			pixelBuffer->unlock();
			QSF_LOG_PRINTS(INFO, "f")
			// Create a material using the texture
			Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create(
				"DynamicTextureMaterial", // name
				Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			QSF_LOG_PRINTS(INFO, "g")
			texture->load(false);


			Ogre::ManualObject* ogreManualObject = nullptr;
				// Sorry, we're out of OGRE manual objects, wait a short moment while we're creating a new instance for you

				// Create a new OGRE manual object (that we can fill with generated mesh data)
				ogreManualObject = TerrainMaster->getOgreSceneManager()->createManualObject();
				//ogreManualObject->setQueryFlags(0);	// Don't allow it to e.g. pick the command target marker

													// Setup OGRE manual object
				ogreManualObject->setCastShadows(false);

				// Handle the OGRE manual object as overlay by setting the proper render queue group
				//ogreManualObject->setRenderQueueGroup(-1);	// Don't use "qsf::RENDER_QUEUE_OVERLAY"
																			// Set bounding box to infinite
				ogreManualObject->setLocalAabb(Ogre::Aabb::BOX_INFINITE);

				// Assign manual OGRE object to OGRE scene node
				
				ogreManualObject->setVisible(true);

				// To save memory reallocation say the estimate number of vertices and indices to the OGRE manual object
				// -> See http://www.ogre3d.org/docs/api/html/classOgre_1_1ManualObject.html as reference
				const int vertexCount = 4;
				ogreManualObject->estimateVertexCount(vertexCount);
				ogreManualObject->estimateIndexCount(6);

				// Build icon geometry
				ogreManualObject->begin(materialName, Ogre::v1::RenderOperation::OT_TRIANGLE_LIST);
				if (nullptr != ogreManualObject)
				{
					const glm::vec2 clipSpaceMin(-10.0f, -10.f);
					const glm::vec2 clipSpaceMax(10.f, 10.0f);
					const Ogre::ColourValue& ogreColour = Ogre::ColourValue::Blue;

					ogreManualObject->position(Ogre::Vector3(clipSpaceMin.x, clipSpaceMax.y, 0.0f));
					ogreManualObject->textureCoord(Ogre::Vector2(0.0f, 0.0f));
					ogreManualObject->colour(ogreColour);

					ogreManualObject->position(Ogre::Vector3(clipSpaceMax.x, clipSpaceMax.y, 0.0f));
					ogreManualObject->textureCoord(Ogre::Vector2(1.0f, 0.0f));
					ogreManualObject->colour(ogreColour);

					ogreManualObject->position(Ogre::Vector3(clipSpaceMin.x, clipSpaceMin.y, 0.0f));
					ogreManualObject->textureCoord(Ogre::Vector2(0.0f, 1.0f));
					ogreManualObject->colour(ogreColour);

					ogreManualObject->position(Ogre::Vector3(clipSpaceMax.x, clipSpaceMin.y, 0.0f));
					ogreManualObject->textureCoord(Ogre::Vector2(1.0f, 1.0f));
					ogreManualObject->colour(ogreColour);

					ogreManualObject->index(0);
					ogreManualObject->index(1);
					ogreManualObject->index(2);
					ogreManualObject->index(2);
					ogreManualObject->index(1);
					ogreManualObject->index(3);

					

					// We need to reset the AABB after each manual object update
					ogreManualObject->setLocalAabb(Ogre::Aabb::BOX_INFINITE);
					QSF_LOG_PRINTS(INFO, "g2")
			}
				TerrainMaster->getOgreSceneManager()->getRootSceneNode()->attachObject(ogreManualObject);
				ogreManualObject->setVisible(true);
				QSF_LOG_PRINTS(INFO, "g31")
				QSF_LOG_PRINTS(INFO, "g32")
				QSF_LOG_PRINTS(INFO, "g4")
					glm::vec2 clipSpacePosition;
				glm::vec2 clipSpaceSize;
					ogreManualObject->position(10.f, 10.f, 0.0f);
					ogreManualObject->end();
					//ogreManualObject->end();
			return;
			/*
			
			
			
				QSF_LOG_PRINTS(INFO, "g2")
				auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(0, 0);
				TerrainMaster->getOgreEntity()->getSubEntity(0)->setMaterial(material);
			//Terrain->mesh->setMaterial(material);
			//TerrainMaster->getOgreEntity()->upd
			QSF_LOG_PRINTS(INFO, "h")
			material->getTechnique(0)->getPass(0)->createTextureUnitState("DynamicTexture");
			QSF_LOG_PRINTS(INFO, "i")
			//material->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		}*/


	bool TerrainEditmodeColorMap::onStartup(EditMode * previousEditMode)
	{

		//prevent crashs if terrain is not there yet
		if (qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<qsf::TerrainComponent>() == nullptr)
			return false;
		TerrainMaster = qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<qsf::TerrainComponent>();
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
			PaintJobProxy.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&TerrainEditmodeColorMap::PaintJob, this, _1));
		QSF_LOG_PRINTS(INFO, "scale" << Heighmapsize / TerrainMaster->getTerrainWorldSize() << " units per meter");
		Scale = Heighmapsize / TerrainMaster->getTerrainWorldSize();
		mParts = floor(Heighmapsize / (partsize - 1.f));
		QSF_LOG_PRINTS(INFO, "we have " << mParts << " x " << mParts << "  Parts");
		QSF_LOG_PRINTS(INFO, "started  TerrainEditmodeColorMap")
			for (uint8 t = 0; t <= TerrainMaster->getOgreTerrainGroup()->getTerrain(0, 0)->getLayerCount(); t++)
			{
				QSF_LOG_PRINTS(INFO, TerrainMaster->getOgreTerrainGroup()->getTerrain(0, 0)->getLayerTextureName(t, 0));

			}
		TerrainMaster->setEditing(true);
		EditMode::onStartup(previousEditMode);

		user::editor::TerrainEditColorMapToolbox* TET = static_cast<user::editor::TerrainEditColorMapToolbox*>(this->getManager().getToolWhichSelectedEditMode());
		if (TET == nullptr)
		{
			QSF_LOG_PRINTS(INFO, "TET is a nullptr");
			return false;
		}
		TerrainEditGUI = TET;
		mSaveMapProxy.registerAt(qsf::MessageConfiguration(qsf::MessageConfiguration("kc::save_heightmap")), boost::bind(&TerrainEditmodeColorMap::SaveMap, this, _1));

		mTerrainComponent = qsf::ComponentMapQuery(QSF_MAINMAP).getFirstInstance<qsf::TerrainComponent>();

		generateMaterial();
		return true;
	}

	void TerrainEditmodeColorMap::onShutdown(EditMode * nextEditMode)
	{
		EditMode::onShutdown(nextEditMode);
		mSaveMapProxy.unregister();
		PaintJobProxy.unregister();
		mDebugDrawProxy.unregister();
		SaveTheFuckingMap();
		QSF_LOG_PRINTS(INFO, "Shutdown")
	}


	bool TerrainEditmodeColorMap::projectToScreen(const glm::vec3& worldSpacePosition, const glm::vec2& screenSpaceSize, glm::vec2& outClipSpacePosition, glm::vec2& outClipSpaceSize) const
	{
				const int width = 10;
				const int height = 10;
				QSF_CHECK(width > 0, "OGRE viewport width is " << width, return false);
				QSF_CHECK(height > 0, "OGRE viewport height is " << height, return false);

				outClipSpaceSize = glm::vec2(screenSpaceSize.x * 2.0f / width, screenSpaceSize.y * 2.0f / height);

				// Distance check: Don't show icons too far away
					// Project position into clip space; it's not visible if somewhere behind camera
						const glm::vec2 clipSpaceMin = outClipSpacePosition + glm::vec2(0.0f, -outClipSpaceSize.y);
						const glm::vec2 clipSpaceMax = outClipSpacePosition + glm::vec2(outClipSpaceSize.x, 0.0f);

							return true;
		}

	//[-------------------------------------------------------]
	//[ Namespace                                             ]
	//[-------------------------------------------------------]
} // editor
} // user
