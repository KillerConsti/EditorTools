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


#include <qsf/debug/DebugDrawManager.h>
#include <qsf/debug/request/RectangleDebugDrawRequest.h>
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
			EditMode(editModeManager),
			mChunkDrawRequestId(qsf::getUninitialized<uint32>()),
			mRectAngleRequest(nullptr)
		{
			timer = 0;
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
			if (yo_mousepoint == oldmouspoint)
				return;
			//QSF_LOG_PRINTS(INFO, "Start Work" << GetSelectedLayerColor())
			yo_mousepoint = oldmouspoint;
			glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(oldmouspoint.x, oldmouspoint.z));
			Mappoint = Mappoint*BlendMapSize;
			RaiseTerrain(Mappoint);
			//QSF_LOG_PRINTS(INFO, "Finish Work")

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
			int MapPointMinX = glm::clamp((int)glm::ceil(MapPoint.x - TotalRadius), 0, (int)BlendMapSize);
			int MapPointMaxX = glm::clamp((int)glm::floor(MapPoint.x + TotalRadius), 0, (int)BlendMapSize);
			int MapPointMinY = glm::clamp((int)glm::ceil(MapPoint.y - TotalRadius), 0, (int)BlendMapSize);
			int MapPointMaxY = glm::clamp((int)glm::floor(MapPoint.y + TotalRadius), 0, (int)BlendMapSize);
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
			UpdateTerrains();
			//QSF_LOG_PRINTS(INFO,"finish of function")

		}

		void TerrainTexturingTool::RaisePoint(glm::vec2 point, float Intensity)
		{
			//QSF_LOG_PRINTS(INFO,"Raise Point 1")
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

			yRemaining = partsize - yRemaining; //y is inverted
			if (yRemaining == 64)
			{
				if (yTerrain >= 1)
					yTerrain--;
				yRemaining = 0;

			}
			if (xTerrain >= mParts || xTerrain < 0)
				return;
			if (yTerrain >= mParts || yTerrain < 0)
				return;
			//QSF_LOG_PRINTS(INFO, "Raise Point Terrain:" << xTerrain << " " <<yTerrain)
			//QSF_LOG_PRINTS(INFO, xTerrain << " " << yTerrain << " " << xRemaining << " " << yRemaining)
			AffectedPoints[xTerrain][yTerrain].push_back(kc_vec2(xRemaining, yRemaining));
			//QSF_LOG_PRINTS(INFO, "Raise Point 3")

			//do remaining stuff in update
			return;
			auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(xTerrain, yTerrain);


			//QSF_LOG_PRINTS(INFO, "Raise Point 2")
			//here was sth
		//we meed to mirror x for some unknown reason. Maybe this map is also "mirrored"
			//QSF_LOG_PRINTS(INFO, "get blend map index")
		//now apply correct blendmap
			int BlendMapIndex = GetBlendMapWithTextureName(xTerrain, yTerrain);
			if (BlendMapIndex == -1)
				return;

			const uint32 maximumNumberOfLayers = TMG_getMaxLayers(Terrain);
			const uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(Terrain->getBlendTextureCount()));
			//QSF_LOG_PRINTS(INFO, "numberOfLayers " << numberOfLayers << "a123 " << BlendMapIndex << " Blendtexturecount "<< Terrain->getBlendTextureCount())
			for (uint32 layerIndex = 1; layerIndex < 6; ++layerIndex)
			{
				//	QSF_LOG_PRINTS(INFO,"layerIndex" << layerIndex)
				if (Terrain->getLayerBlendMap(layerIndex) == nullptr)
					continue;
				if (layerIndex == BlendMapIndex)
				{
					//QSF_LOG_PRINTS(INFO, "found the layer")
			//		QSF_LOG_PRINTS(INFO, "layerIndex == BlendMapIndex and now change sth"<< layerIndex)
					Terrain->getLayerBlendMap(layerIndex)->setBlendValue(xRemaining, yRemaining, 1);
					//QSF_LOG_PRINTS(INFO, "match" << layerIndex)
				}
				else
				{
					//	QSF_LOG_PRINTS(INFO, "layerIndex == BlendMapIndex and now change reset it..." << layerIndex)
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

		void TerrainTexturingTool::ReplaceLayer(int LayerId, std::string NewMaterial)
		{

			glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(oldmouspoint.x, oldmouspoint.z));
			Mappoint = Mappoint*BlendMapSize;
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
			qsf::Transform transform;
			{
				const qsf::TransformComponent* transformComponent = TerrainMaster->getEntity().getComponent<qsf::TransformComponent>();
				if (nullptr != transformComponent)
				{
					transform = transformComponent->getTransform();
				}
			}
			auto worldpos = oldmouspoint;
			const float worldSize = TerrainMaster->getTerrainWorldSize();
			const float worldSizeHalf = worldSize * 0.5f;
			const float size = worldSize * 0.5f / TerrainMaster->getTerrainChunksPerEdge();
			const glm::vec3 brushPosition = worldpos;

			glm::vec3 position = transform.getPosition();
			position.y = brushPosition.y;

			const float snapSize = worldSize / static_cast<float>(TerrainMaster->getTerrainChunksPerEdge());
			const float snapSizeHalf = snapSize * 0.5f;
			position.x = glm::round((brushPosition.x - snapSizeHalf + worldSizeHalf) / snapSize) * snapSize - worldSizeHalf + snapSizeHalf;
			position.z = glm::round((brushPosition.z - snapSizeHalf - worldSizeHalf) / snapSize) * snapSize + worldSizeHalf + snapSizeHalf;

			transform.setPosition(position);
			transform.setScale(glm::vec3(size, 1.0f, size));

			{ // Tell the world about the selected chunk
				const uint32 chunkX = glm::floor(((float)position.x + worldSizeHalf) / snapSize);
				const uint32 chunkY = glm::floor((worldSizeHalf - (float)position.z) / snapSize);
				if (xTerrain != chunkX || yTerrain != chunkY)
				{
					xTerrain = chunkX;
					yTerrain = chunkY;
					//Q_EMIT chunkChanged(chunkX, chunkY);
				}
			}
			//QSF_LOG_PRINTS(INFO,"x "<< xTerrain << " y " << yTerrain)
			auto Terrain_chunck = TerrainMaster->getOgreTerrainGroup()->getTerrain(xTerrain,yTerrain);
			if(Terrain_chunck == nullptr)
			return;
			if(qsf::AssetProxy(NewMaterial).getAsset() == nullptr)
			return;
			if (LayerId >= Terrain_chunck->getLayerCount())
			return;
			//QSF_LOG_PRINTS(INFO, "LayerId " << LayerId << " NewMaterial " << NewMaterial)
				Terrain_chunck->setLayerTextureName(LayerId,0, NewMaterial.c_str());
			TerrainMaster->RefreshMaterial(Terrain_chunck);
			WriteTerrainTextureList();

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

		void TerrainTexturingTool::CopyQSFTerrain(const qsf::MessageParameters & parameters)
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
			//check qsf terrain parts...
			auto it = CopyFromTerrain->getOgreTerrainGroup()->getTerrainIterator();
			int counter = 0; // because my ID start at 0
			while (it.hasMoreElements()) // add the layer to all terrains in the terrainGroup
			{
				Ogre::TerrainGroup::TerrainSlot* a = it.getNext();
				counter++;
			}

			int CopyFrompartsize = CopyFromTerrain->getBlendMapSize() / sqrt(counter);
			int CopyFromParts = sqrt(counter);
			if (CopyFromParts != mParts)
			{
				QSF_LOG_PRINTS(INFO, "Cant do that number of parts not matching. Source has " << CopyFromParts << " and Target has " << mParts)
					return;
			}
			if (CopyFrompartsize != partsize)
			{
				QSF_LOG_PRINTS(INFO, "Cant do that partsize does not match. Source has " << CopyFrompartsize << " and Target has " << partsize)
					return;
			}



			//overwritte layers
			auto it_source = CopyFromTerrain->getOgreTerrainGroup()->getTerrainIterator();
			auto it_target = TerrainMaster->getOgreTerrainGroup()->getTerrainIterator();
			std::string LastLayerName = "";
			while (it_source.hasMoreElements()) // add the layer to all terrains in the terrainGroup
			{
				Ogre::TerrainGroup::TerrainSlot* TS_source = it_source.getNext();
				Ogre::TerrainGroup::TerrainSlot* TS_target = it_target.getNext();

				//it seems that only layer 0-3 get used so we might kick empty data layers

				std::vector<LayerData> DataToResort;
				for (size_t t = 0; t < TS_source->instance->getLayerCount(); t++)
				{
					LayerData CurrentLayer;
					CurrentLayer.LayerName = TS_source->instance->getLayerTextureName((uint8)t, 0);
					CurrentLayer.OriginLayer = (int)t;
					if (t > 0)
					{
						for (size_t j = 0; j < partsize; j++)
						{
							std::pair<std::string, uint16> Datapair;
							for (size_t k = 0; k < partsize; k++)
							{

								auto val = TS_source->instance->getLayerBlendMap((uint8)t)->getBlendValue(j, k);
								if (val != 0)
									CurrentLayer.AffectedPoints++;
								CurrentLayer.Data.push_back(glm::vec3(j, k, val));
								//if(val > 0)
								//QSF_LOG_PRINTS(INFO,val)

							}
						}
					}
					DataToResort.push_back(CurrentLayer);
				}
				//is this a tthing
				//std::sort(DataToResort.begin(), DataToResort.end());
				for (size_t t = 0; t < DataToResort.size(); t++)
				{
					//QSF_LOG_PRINTS(INFO, "Layer " << t << " resorted is " << DataToResort.at(t).LayerName << " (" << DataToResort.at(t).OriginLayer << ") with " << DataToResort.at(t).AffectedPoints)
				}

				for (size_t t = 0; t < DataToResort.size(); t++)
				{
					//in kc terrain we use local asset name
					//in qsf terrain we use global asset id
					//so we need to translate it
					//QSF_LOG_PRINTS(INFO,"t is "<< t)
					try
					{
						//QSF_LOG_PRINTS(INFO, "getter " << t)
						qsf::AssetProxy Textureasset = qsf::AssetProxy(boost::lexical_cast<uint64>(DataToResort.at(t).LayerName));
						if (Textureasset.getAsset() != nullptr)
						{
							std::string TextureName = Textureasset.getLocalAssetName();
							//QSF_LOG_PRINTS(INFO, "setter " << TextureName)
							while (TS_target->instance->getLayerCount() <= t)
							{
								TS_target->instance->addLayer();
							}
							if (TextureName != TS_target->instance->getLayerTextureName((uint8)t, 0))
							{
								TS_target->instance->setLayerTextureName((uint8)t, 0, TextureName.c_str());
								//QSF_LOG_PRINTS(INFO, "set layer texture name")
								LastLayerName = TextureName.c_str();
							}


						}
					}
					catch (const std::exception&)
					{

					}
					//if(t == 0)
					//continue;
					//second we need to apply all values
				}
				//after all layers are created we can assign Blend values
				for (size_t t = 0; t < DataToResort.size(); t++)
				{
					auto LayerData = DataToResort.at(t).Data;
					for (auto a : LayerData)
					{
						if (t > 0)
							TS_target->instance->getLayerBlendMap((uint8)t)->setBlendValue((int)a.x, (int)a.y, a.z);

					}
					if (t > 0)
					{
						TS_target->instance->getLayerBlendMap((uint8)t)->dirty();
						TS_target->instance->getLayerBlendMap((uint8)t)->update();
					}

				}
				TerrainMaster->RefreshMaterial(TS_target->instance);


			}
			QSF_LOG_PRINTS(INFO, "Copy Data done. May need to update material ")

		}

		void TerrainTexturingTool::ClearLayer(const qsf::MessageParameters & parameters)
		{
		}

		void TerrainTexturingTool::ReplaceGroundLayer(const qsf::MessageParameters & parameters)
		{
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
						//QSF_LOG_PRINTS(INFO, x << " 1" << y)
					}
					else
					{
						TerrainMaster->getOgreTerrainGroup()->defineTerrain(x, y, Ogre::String("chunks/a" + boost::lexical_cast<std::string>(x) + "_" + boost::lexical_cast<std::string>((y)) + ""));
						//QSF_LOG_PRINTS(INFO, x << " " << y)
					}
				}
			}

			TerrainMaster->getOgreTerrainGroup()->loadAllTerrains(true);
			TerrainMaster->getOgreTerrainGroup()->freeTemporaryResources();
			//QSF_LOG_PRINTS(INFO, "Load old map done!")
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
				if ((xRemaining - partsize) > 0)
				{
					xTerrain++;
					xRemaining = xRemaining - partsize;
				}
				else
					break;
			}
			while (true)
			{
				if ((yRemaining - partsize) > 0)
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
					Mappoint = Mappoint*BlendMapSize;
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
				{
				mouseisvalid = false;
				if (qsf::isInitialized(mChunkDrawRequestId))
				{
					QSF_DEBUGDRAW.cancelRequest(mChunkDrawRequestId);
				}
				}
		}

		void TerrainTexturingTool::UpdateChunkDebugDrawg(glm::vec3 worldpos, int x, int y)
		{
			if (qsf::isUninitialized(mChunkDrawRequestId))
			{
				// Chunk visualisation
				mRectAngleRequest = new qsf::RectangleDebugDrawRequest(glm::vec3(-1.0f, 0.0f, -1.0f), qsf::Math::GLM_VEC3_UNIT_XYZ, qsf::Color4::WHITE, 0.0f);
				mChunkDrawRequestId = QSF_DEBUGDRAW.requestDraw(*mRectAngleRequest);
			}
			if (qsf::isInitialized(mChunkDrawRequestId))
			{
				// Get the transform of the terrain
				qsf::Transform transform;
				{
					const qsf::TransformComponent* transformComponent = TerrainMaster->getEntity().getComponent<qsf::TransformComponent>();
					if (nullptr != transformComponent)
					{
						transform = transformComponent->getTransform();
					}
				}

				const float worldSize = TerrainMaster->getTerrainWorldSize();
				const float worldSizeHalf = worldSize * 0.5f;
				const float size = worldSize * 0.5f / TerrainMaster->getTerrainChunksPerEdge();
				const glm::vec3 brushPosition = worldpos;

				glm::vec3 position = transform.getPosition();
				position.y = brushPosition.y;

				const float snapSize = worldSize / static_cast<float>(TerrainMaster->getTerrainChunksPerEdge());
				const float snapSizeHalf = snapSize * 0.5f;
				position.x = glm::round((brushPosition.x - snapSizeHalf + worldSizeHalf) / snapSize) * snapSize - worldSizeHalf + snapSizeHalf;
				position.z = glm::round((brushPosition.z - snapSizeHalf - worldSizeHalf) / snapSize) * snapSize + worldSizeHalf + snapSizeHalf;

				transform.setPosition(position);
				transform.setScale(glm::vec3(size, 1.0f, size));

				{ // Tell the world about the selected chunk
					const uint32 chunkX = glm::floor(((float)position.x + worldSizeHalf) / snapSize);
					const uint32 chunkY = glm::floor((worldSizeHalf - (float)position.z) / snapSize);
					if (x != chunkX || y != chunkY)
					{
						x = chunkX;
						y = chunkY;
						//Q_EMIT chunkChanged(chunkX, chunkY);
					}
				}

				// Tell the debug draw request about the transform
				QSF_DEBUGDRAW.setRequestTransform(mChunkDrawRequestId, transform);
			}
		}

		void TerrainTexturingTool::WriteTerrainTextureList()
		{
			//if (TerrainEditGUI->GetLayerColor() != "")
				//return;
			//find Terrain
			glm::vec2 Mappoint = ConvertWorldPointToRelativePoint(glm::vec2(oldmouspoint.x, oldmouspoint.z));
			Mappoint = Mappoint*BlendMapSize;
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
			if(m_NeedUpdatingTerrainList.x == xTerrain && m_NeedUpdatingTerrainList.y == yTerrain && m_NeedUpdatingTerrainList.z == 0)
			{
				UpdateChunkDebugDrawg(oldmouspoint, xTerrain, yTerrain);
				return;
			}
			else
			{

					UpdateChunkDebugDrawg(oldmouspoint, xTerrain, yTerrain);

				m_NeedUpdatingTerrainList.x = xTerrain;
				m_NeedUpdatingTerrainList.y = yTerrain;
				m_NeedUpdatingTerrainList.z = 0;
			}
			//QSF_LOG_PRINTS(INFO, "we are here" << xTerrain << " " << yTerrain )
			//QSF_LOG_PRINTS(INFO, xTerrain << " " << yTerrain << " " << xRemaining << " " << yRemaining)

			auto Terrain = TerrainMaster->getOgreTerrainGroup2()->getTerrain(xTerrain, yTerrain);
			if (Terrain == nullptr)
			{
				//QSF_LOG_PRINTS(INFO,"Terrain is a nullptr")
				return;
			}

			const uint32 maximumNumberOfLayers = TMG_getMaxLayers(Terrain);
			const uint32 numberOfLayers = std::min(maximumNumberOfLayers, (uint32)Terrain->getLayerCount());
			std::vector<std::pair<std::string, int>> TerrainNames;
			//QSF_LOG_PRINTS(INFO, "we are here"<< (int)maximumNumberOfLayers<<  " ...."<< numberOfLayers  <<"...."<<  (uint32)Terrain->getLayerCount())
			for (uint32 layerIndex = 0; layerIndex < numberOfLayers; ++layerIndex)
			{
				const std::string layerIndexAsString = std::to_string(layerIndex);
				// Inside the first texture name of the terrain layer we store the global asset ID of the QSF material the terrain layer is using, we need nothing more
				const std::string globalAssetIdAsString = Terrain->getLayerTextureName(layerIndex, 0);
				//QSF_LOG_PRINTS(INFO, "Terrainname" << xTerrain << " " << yTerrain << " " << globalAssetIdAsString)
				if (qsf::AssetProxy(globalAssetIdAsString).getAsset() == nullptr)
				{
					continue;
				}
				int count = 0;
				if(layerIndex != 0)
				{
				if (Terrain->getLayerBlendMap(layerIndex) != nullptr)
				{
					auto BM = Terrain->getLayerBlendMap(layerIndex);
					//QSF_LOG_PRINTS(INFO, "found the layer")
					//		QSF_LOG_PRINTS(INFO, "layerIndex == BlendMapIndex and now change sth"<< layerIndex)
					for (size_t x = 0; x < partsize; x++)
					{
						for (size_t y = 0; y < partsize; y++)
						{
							if (BM->getBlendValue(x,y) > 0)
								count++;

						}
						//QSF_LOG_PRINTS(INFO, "match" << layerIndex)
					}
					}
				}
				TerrainNames.push_back(std::pair<std::string, int>(qsf::AssetProxy(globalAssetIdAsString).getLocalAssetName(), count));
				//TerrainEditGUI->


			}
			//kc
			TerrainEditGUI->SetCurrentTerrainData(TerrainNames, xTerrain, yTerrain);
		}

		int TerrainTexturingTool::onAddNewTerrain(std::string BlendMapName, int x, int y)
		{
			if(BlendMapName == "")
			return -1;
			//first check if there is a layer twice
			std::vector<std::string> LayerNames;
			
			Ogre::Terrain* Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(x,y);
			bool doubleLayer = false;
			int LayerToReplace = -1;
			for (size_t t = 0; t < Terrain->getLayerCount() && t < 6; t++)
			{
				if (TerrainTextureAllreadyExists(Terrain->getLayerTextureName((uint8)t, 0), LayerNames))
				{
					doubleLayer = true;
					LayerToReplace = (int)t;
					//QSF_LOG_PRINTS(INFO,"replaced double layer"  << t)
					break;
				}

			}
			if (LayerToReplace == -1) //check layer count
			{
				if (Terrain->getLayerCount() < 6)
				{
					LayerToReplace = Terrain->getLayerCount();
					//QSF_LOG_PRINTS(INFO, "add completly new layer")
				}
			}
			if (LayerToReplace == -1)
			{
				for (size_t t = 1; t < Terrain->getLayerCount() && Terrain->getLayerCount()<6; t++)
				{
					if(TerrainLayerIsEmpty(Terrain,(uint8)t))
					{
						//QSF_LOG_PRINTS(INFO,"replace empty layer" <<t)
						LayerToReplace = (int)t;
						break;
					}

				}
				//if(LayerToReplace != -1)
					//QSF_LOG_PRINTS(INFO, "replace empty layer")
			}
			if (LayerToReplace == -1)
			{
				QSF_LOG_PRINTS(INFO,"Did not found a good layer")
				//we may search for an empty layer
				return false;
			}

			//place layer
			if (LayerToReplace != Terrain->getLayerCount())
			{
				if (qsf::AssetProxy(BlendMapName).getAsset() == nullptr)
				{
					QSF_LOG_PRINTS(INFO,"cant applay Blendmap with name "<< BlendMapName << " unknown asset ")
					return -1;
				}
				Terrain->setLayerTextureName(LayerToReplace,0,BlendMapName.c_str());
			}
			else
			{
				if(LayerToReplace < 5 && Terrain->getLayerCount() <=6)
				{
				Terrain->addLayer();
				Terrain->setLayerTextureName(LayerToReplace, 0, BlendMapName);
				}

			}

			//update material
			TerrainMaster->RefreshMaterial(Terrain);
			return LayerToReplace;

		}

		bool TerrainTexturingTool::TerrainTextureAllreadyExists(std::string CheckMe, std::vector<std::string>& ToCheck)
		{
			for (auto a : ToCheck)
			{
				if(a == CheckMe)
				return true;
			}
			ToCheck.push_back(CheckMe);
			return false;
		}

		bool TerrainTexturingTool::TerrainLayerIsEmpty(Ogre::Terrain* Terrain, int layer)
		{
			auto BlendMap = Terrain->getLayerBlendMap(layer);
			if(BlendMap == nullptr)
			return false;
			for (size_t x = 0; x < partsize; x++)
			{
				for (size_t y = 0; y < partsize; y++)
				{
					if(BlendMap->getBlendValue(x,y) > 0)
					return false;
				}
			}
			return true;
		}

		std::string TerrainTexturingTool::GetSelectedLayerColor()
		{
			return TerrainEditGUI->GetLayerColor();
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
					//QSF_LOG_PRINTS(INFO, "layer Index" << Terrain->getLayerTextureName(layerIndex, 0).c_str() << " vs " << GetSelectedLayerColor())
					if (Terrain->getLayerTextureName(layerIndex, 0).c_str() == GetSelectedLayerColor())
					{
						//QSF_LOG_PRINTS(INFO, "found layer Index " << GetSelectedLayerColor() << " " << layerIndex);
						return layerIndex;
					}
				}
				catch (const std::exception& /*e*/)
				{
					QSF_LOG_PRINTS(INFO, Terrain->getLayerTextureName(layerIndex, 0) << " and selected color is " << GetSelectedLayerColor())
						//QSF_LOG_PRINTS(INFO, e.what());
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
			copy = copy / (float)BlendMapSize;
			copy = copy * TerrainMaster->getTerrainWorldSize();
			copy = copy - Offset;
			copy.y = -1.f * copy.y;
			return copy;
		}

		void TerrainTexturingTool::UpdateTerrains()
		{
			//QSF_LOG_PRINTS(INFO,"Update Started")
				//we should make sure that we have a 4 x 4 Terrain-pattern (16 tiles).
				//we assume it here by t = x and y = i
			for (long t = 0; t <= (mParts - 1); t++)
			{
				for (long i = 0; i <= (mParts - 1); i++)
				{
					//QSF_LOG_PRINTS(INFO, " t " << t << " i " << i)
					if (TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i) != nullptr)
					{

						//Write Blend Map Data
						if (AffectedPoints[t][i].empty())
							continue;
						//QSF_LOG_PRINTS(INFO, "Update works on " << t << " " << i)
						std::sort(AffectedPoints[t][i].begin(), AffectedPoints[t][i].end());

						AffectedPoints[t][i].erase(
							std::unique(
								AffectedPoints[t][i].begin(),
								AffectedPoints[t][i].end(),
								[](kc_vec2 const & l, kc_vec2 const & r) {return l.x == r.x && l.y == r.y; }
							),
							AffectedPoints[t][i].end()
						);
						 
						//AffectedPoints[t][i].erase(std::unique(AffectedPoints[t][i].begin(), AffectedPoints[t][i].end()), AffectedPoints[t][i].end());
						int BlendMapIndex = GetBlendMapWithTextureName((long)t, (long)i);
						//QSF_LOG_PRINTS(INFO,BlendMapIndex);
						if (BlendMapIndex == -1)
						{
							onAddNewTerrain(GetSelectedLayerColor(),(int)t,(int)i);
							//int BlendMapIndex = GetBlendMapWithTextureName((long)t, (long)i);
							//if (BlendMapIndex == -1)
							continue;
						}
							
						auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i);
						const uint32 maximumNumberOfLayers = TMG_getMaxLayers(Terrain);
						const uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(Terrain->getBlendTextureCount()));


						for (uint32 layerIndex = 1; layerIndex < 6; ++layerIndex)
						{
							bool MapChanged = false;
							if(Terrain->getLayerCount() <= layerIndex)
							continue;
							//if(layerIndex > Terrain->getBlendTextureCount())
							//continue;
							auto CurrentBlendMap = Terrain->getLayerBlendMap(layerIndex);
							if (CurrentBlendMap == nullptr)
								continue;
							//if (Terrain->getLayerBlendMap(layerIndex) == nullptr)
								//continue;

							for (auto points : AffectedPoints[t][i])
							{
								//if(layerIndex == 1)
								//QSF_LOG_PRINTS(INFO,points.x << " "<< points.y << " and map is << "<< t << " " << i)
								//we need to recalc why we get 64 :)
								if (points.x >= partsize || points.y >= partsize)
									continue;
								try
								{
									//QSF_LOG_PRINTS(INFO, "eval point "<<points.x <<" " << points.y)
									if (layerIndex == BlendMapIndex)
									{
										//QSF_LOG_PRINTS(INFO, "found the layer")
										if (CurrentBlendMap->getBlendValue(points.x, points.y) == 1)
											continue;
										CurrentBlendMap->setBlendValue(points.x, points.y, 1);
										MapChanged = true;
										//QSF_LOG_PRINTS(INFO, "match" << layerIndex)
									}
									else
									{
										if (CurrentBlendMap->getBlendValue(points.x, points.y) == 0)
											continue;
										CurrentBlendMap->setBlendValue(points.x, points.y, 0);
										MapChanged = true;
									}
								}
								catch (const std::exception& /*e*/)
								{
									//QSF_LOG_PRINTS(INFO,"layer index "<< layerIndex << " " <<e.what())
									continue;
								}
							}

							if (Terrain->getLayerBlendMap(layerIndex) != nullptr)
							{
								//if(MapChanged)
								{
									CurrentBlendMap->dirty();
									CurrentBlendMap->update();
								}
							}
							//End

						}
						AffectedPoints[t][i].clear();

						//QSF_LOG_PRINTS(INFO, "Clear list and update stuff")

					}
				}
			}
			//QSF_LOG_PRINTS(INFO, "Update finished")
			m_NeedUpdatingTerrainList.z = 1;
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

			Offset = (float)(TerrainMaster->getTerrainWorldSize() / 2);
			BlendMapSize = (float)TerrainMaster->getBlendMapSize();
			//Heighmapsize = (float)TerrainMaster->getBlendMapSize();
			Ogre::TerrainGroup::TerrainIterator it = TerrainMaster->getOgreTerrainGroup()->getTerrainIterator();
			int counter = 0; // because my ID start at 0
			while (it.hasMoreElements()) // add the layer to all terrains in the terrainGroup
			{
				Ogre::TerrainGroup::TerrainSlot* a = it.getNext();
				counter++;
			}

			partsize = BlendMapSize / sqrt(counter);
			mParts = sqrt(counter);
			QSF_LOG_PRINTS(INFO, "parts " << counter << " partsize " << partsize << " mParts per direction " << mParts);
			QSF_LOG_PRINTS(INFO, "scale" << BlendMapSize / TerrainMaster->getTerrainWorldSize() << " units per meter");
			Scale = BlendMapSize / TerrainMaster->getTerrainWorldSize();
			EditMode::onStartup(previousEditMode);

			user::editor::TerrainTexturingToolbox* TET = static_cast<user::editor::TerrainTexturingToolbox*>(this->getManager().getToolWhichSelectedEditMode());
			if (TET == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "TET is a nullptr");
				return false;
			}
			TerrainEditGUI = TET;


			if (!PaintJobProxy.isValid())
				PaintJobProxy.registerAt(em5::Jobs::ANIMATION_VEHICLE, boost::bind(&TerrainTexturingTool::PaintJob, this, _1));
			mSaveMapProxy.registerAt(qsf::MessageConfiguration(qsf::MessageConfiguration("kc::save_heightmap")), boost::bind(&TerrainTexturingTool::SaveMap, this, _1));
			mCopyQSFTerrain.registerAt(qsf::MessageConfiguration(qsf::MessageConfiguration("kc::CopyFromQSFTerrain")), boost::bind(&TerrainTexturingTool::CopyQSFTerrain, this, _1));
			//LoadOldMap();
			return true;
		}

		void TerrainTexturingTool::onShutdown(EditMode * nextEditMode)
		{
			EditMode::onShutdown(nextEditMode);
			mSaveMapProxy.unregister();
			PaintJobProxy.unregister();
			mCopyQSFTerrain.unregister();
			mDebugDrawProxy.unregister();
			SaveTheFuckingMap();
			if (qsf::isInitialized(mChunkDrawRequestId))
			{
				QSF_DEBUGDRAW.cancelRequest(mChunkDrawRequestId);
			}
			QSF_LOG_PRINTS(INFO, "Shutdown")
		}




		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
