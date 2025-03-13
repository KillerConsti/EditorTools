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

#include <qsf/file/FileSystem.h>
#include <experimental/filesystem> 
#include <boost\filesystem.hpp>
#include <qsf/plugin/QsfAssetTypes.h>
#include <qsf/asset/project/AssetPackage.h>
#include <qsf_editor/asset/import/AssetImportManager.h>
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
			if (TerrainTexturingToolbox::GetInstance() == nullptr) //allready called
			{
				return;
			}
			if(!mSaveMapProxy.isValid())
			return;
			mSaveMapProxy.unregister();
			PaintJobProxy.unregister();
			mCopyQSFTerrain.unregister();
			mDebugDrawProxy.unregister();
			SaveTheFuckingMap();
			if (qsf::isInitialized(mChunkDrawRequestId))
			{
				QSF_DEBUGDRAW.cancelRequest(mChunkDrawRequestId);
			}
			QSF_LOG_PRINTS(INFO, "TerrainTexturingTool Shutdown")
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
			if (TerrainTexturingToolbox::GetInstance() == nullptr || !TerrainMaster.valid()) //call on shutdown if our gui was shutdowned
			{
				QSF_LOG_PRINTS(INFO,"Tool shutdown")
				onShutdown(nullptr);
				return;
			}
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
			bool InEraseMode = TerrainEditGUI->IsInEraseMode();
			//QSF_LOG_PRINTS(INFO,"aha 1")
			float TotalRadius = Radius * Scale;
			float BrushIntensity = TerrainEditGUI->GetBrushIntensity()*0.01;

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
								if (!InEraseMode)
									RaisePoint(glm::vec2(t, j), BrushIntensity);
								else
									RaisePoint(glm::vec2(t, j), 1);
								counter++;
								//break;
							}
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Cone)
							{
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = 1.0f - (Distance / TotalRadius);
								if (!InEraseMode)
									RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod);
								else
									RaisePoint(glm::vec2(t, j), 1);
								counter++;
								//break;
							}
							else if (TerrainEditGUI->GetBrushShape() == TerrainEditGUI->Dome)
							{
								//intensity formula here
								float Distance = glm::distance(glm::vec2(t, j), MapPoint);
								//intensity formula here
								float IntensityMod = glm::cos((Distance / TotalRadius) * glm::pi<float>()) * 0.5f + 0.5f;
								if (!InEraseMode)
									RaisePoint(glm::vec2(t, j), BrushIntensity*IntensityMod);
								else
									RaisePoint(glm::vec2(t, j), 1);
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
						if (!InEraseMode)
							RaisePoint(glm::vec2(t, j), BrushIntensity);
						else
							RaisePoint(glm::vec2(t, j), 1);
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
			//QSF_LOG_PRINTS(INFO, "Raise Point" << Intensity)
			AffectedPoints[xTerrain][yTerrain].push_back(kc_vec3(xRemaining, yRemaining, Intensity));
			//QSF_LOG_PRINTS(INFO, "Raise Point 3")

			//do remaining stuff in update
			return;

		}

		void TerrainTexturingTool::MixIntensitiesTerrain(TerrainTexturingTool::NewMixedIntensities * NMI, int LayerId, float NewIntensity2)
		{
		//tickzcahl ist wahrscheinlich sonst zu hoch
			float NewIntensity = NewIntensity2 * 0.25f;
			//QSF_LOG_PRINTS(INFO, "input i1 " <<  NMI->IntensityLayer1  << " i2 " <<  NMI->IntensityLayer2  <<" i3 "<< NMI->IntensityLayer3 << " i4 " << NMI->IntensityLayer4)
			float AllIntensitiesTogether = NMI->IntensityLayer1 + NMI->IntensityLayer2 + NMI->IntensityLayer3 + NMI->IntensityLayer4 + NMI->IntensityLayer5;
			if (AllIntensitiesTogether <= 0.1) //cant divide by 0
				AllIntensitiesTogether = 1;
			float AllNewIntensíty = 1.f - NewIntensity;
			
			NMI->IntensityLayer1 = (NMI->IntensityLayer1 / AllIntensitiesTogether) * AllNewIntensíty; //first normalize to 1 - then apply removed intensity
			NMI->IntensityLayer2 = (NMI->IntensityLayer2 / AllIntensitiesTogether) * AllNewIntensíty;
			NMI->IntensityLayer3 = (NMI->IntensityLayer3 / AllIntensitiesTogether) * AllNewIntensíty;
			NMI->IntensityLayer4 = (NMI->IntensityLayer4 / AllIntensitiesTogether) * AllNewIntensíty;
			NMI->IntensityLayer5 = (NMI->IntensityLayer5 / AllIntensitiesTogether) * AllNewIntensíty;
			
			switch (LayerId)
			{
			case 0:
			{
				//duno?
				break;
			}
			case 1:
			{
				NMI->IntensityLayer1 = NMI->IntensityLayer1 + NewIntensity;

				break;
			}
			case 2:
			{
				NMI->IntensityLayer2 = NMI->IntensityLayer2 + NewIntensity;
				break;
			}
			case 3:
			{
				NMI->IntensityLayer3 = NMI->IntensityLayer3 + NewIntensity;
				break;
			}
			case 4:
			{
				NMI->IntensityLayer4 = NMI->IntensityLayer4 + NewIntensity;
				break;
			}
			case 5:
			{
				NMI->IntensityLayer5 = NMI->IntensityLayer5 + NewIntensity;
				break;
			}
			default:
				break;
			}
			//QSF_LOG_PRINTS(INFO, "layer " << LayerId << " int "<< NewIntensity << " i1 "<<  NMI->IntensityLayer1 << " NMI->IntensityLayer2 " << NMI->IntensityLayer2 << " NMI->IntensityLayer3 " << NMI->IntensityLayer3 << " NMI->IntensityLayer4 " << NMI->IntensityLayer4)
		}

		void TerrainTexturingTool::WriteTerrainTextureJson(qsf::editor::AssetEditHelper* IAP)
		{
			//QSF_LOG_PRINTS(INFO,"Write Terrain Textures 1")
			boost::property_tree::ptree rootPTree;
			auto it = TerrainMaster->getOgreTerrainGroup()->getTerrainIterator();
			int counter =0;
			while (it.hasMoreElements()) // add the layer to all terrains in the terrainGroup
			{
				Ogre::TerrainGroup::TerrainSlot* a = it.getNext();
				boost::property_tree::ptree layers;
				for (size_t x = 0; x < a->instance->getLayerCount() && x < 6; x++)
				{
					layers.put("Layer"+boost::lexical_cast<std::string>(x),a->instance->getLayerTextureName((uint8)x,0).c_str());
					//QSF_LOG_PRINTS(INFO, a->instance->getLayerTextureName((uint8)x, 0).c_str())
				}
				//QSF_LOG_PRINTS(INFO, "Write Terrain Textures 2 "<< counter)
				counter++;
				rootPTree.add_child(boost::lexical_cast<std::string>(a->x)+"_"+ boost::lexical_cast<std::string>(a->y), layers);
			}
			//QSF_LOG_PRINTS(INFO, "Write Terrain Textures 3 " << counter)
			//now save
			auto AP = qsf::AssetProxy(TerrainMaster->GetTerrainLayerList());
			std::string LocalAssetName = "";
			if (AP.getAsset() != nullptr)
			{
				LocalAssetName = AP.getLocalAssetName();
				if (!std::experimental::filesystem::exists(AP.getAbsoluteCachedAssetDataFilename()))
				{
					//if not destroy it amd create new //... this can happen when using ramdisk which i recommend
					AP.getAssetPackage()->destroyAssetByGlobalAssetId(AP.getGlobalAssetId());
				}
			}
			AP = qsf::AssetProxy(LocalAssetName);
			if(AP.getAsset() != nullptr) //we need to overwrite it
			//write to disk
			{
			boost::nowide::ofstream ofs(AP.getAbsoluteCachedAssetDataFilename());
			qsf::FileHelper::writeJson(ofs, rootPTree);
			IAP->tryEditAsset(AP.getGlobalAssetId(), AP.getAssetPackage()->getName());
			IAP->setAssetUploadData(AP.getGlobalAssetId(), true, true);
			}
			else //Asset does not exists (yet)
			{

				auto TimeStamp = GetCurrentTimeForFileName();
				auto Name = TerrainMaster->getEntity().getComponent<qsf::MetadataComponent>()->getName();


				//b create folder structure in assethelper (to read and write)
				if (!boost::filesystem::exists((QSF_FILE.getBaseDirectory() + "/" + IAP->getIntermediateAssetPackage()->getRelativeDirectory() + "/texture/heightmap")))
					boost::filesystem::create_directories(QSF_FILE.getBaseDirectory() + "/" + IAP->getIntermediateAssetPackage()->getRelativeDirectory() + "/texture/heightmap");
				//c write to new folder -I think it will copy to our direction we wrote before
				//ogreImage.save(QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap/heightmap_" + Name + "_" + TimeStamp +".tif");
				auto relAssetDirectory = QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackage()->getRelativeDirectory();
				if (!boost::filesystem::exists((QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/heightmap")))
					boost::filesystem::create_directories(QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/heightmap");

				auto fileName = "terrain_layerdescription_" + Name;

				//MagImage->write(QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/heightmap/heightmap_" + Name  +  ".tif");
				QSF_LOG_PRINTS(INFO, "saved terrainlayerdescription to " << QSF_FILE.getBaseDirectory() + "/" + IAP->getIntermediateAssetPackage()->getRelativeDirectory() + "/texture/heightmap/" + fileName + ".json")
					//d learn our assets a few thing <-> this seems not needed
					//delete[] buffer;

					auto Asset = IAP->addAsset(QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackage()->getName(), qsf::QsfAssetTypes::TEXTURE, "heightmap", fileName);
				boost::nowide::ofstream ofs(QSF_FILE.getBaseDirectory() + "/" + IAP->getIntermediateAssetPackage()->getRelativeDirectory() + "/texture/heightmap/" + fileName + ".json");
				qsf::FileHelper::writeJson(ofs, rootPTree);
				if (Asset == nullptr)
					QSF_LOG_PRINTS(INFO, "error occured " << Name << " could not create an asset")
				else
				{
					auto CachedAsset = IAP->getCachedAsset(Asset->getGlobalAssetId());
					if (CachedAsset == nullptr)
						CachedAsset = &qsf::CachedAsset(Asset->getGlobalAssetId());
					if (CachedAsset == nullptr)
					{
						QSF_LOG_PRINTS(INFO, "still a nullptr")
					}
					CachedAsset->setType("json");
					//QSF_LOG_PRINTS(INFO, qsf::AssetProxy(Asset->getGlobalAssetId()).getAbsoluteCachedAssetDataFilename())

					if (IAP->setAssetUploadData(Asset->getGlobalAssetId(), true, true))
						QSF_LOG_PRINTS(INFO, "Caching asset was not succesfull")
				}
				TerrainMaster->SetTerrainLayerList(qsf::AssetProxy(Asset->getGlobalAssetId()));
			}
		}


		void TerrainTexturingTool::ReplaceLayer(int LayerId, std::string NewMaterial)
		{


			//they are known from WriteTerrainTextureList
			auto Terrain_chunck = TerrainMaster->getOgreTerrainGroup()->getTerrain(SelectedChunk_x, SelectedChunk_y);
			if (Terrain_chunck == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "cant find terrain chunk " << SelectedChunk_x << " " << SelectedChunk_y)
					return;
			}
			if (qsf::AssetProxy(NewMaterial).getAsset() == nullptr)
				return;
			if (LayerId >= Terrain_chunck->getLayerCount())
				return;
			//QSF_LOG_PRINTS(INFO, "LayerId " << LayerId << " NewMaterial " << NewMaterial)
			Terrain_chunck->setLayerTextureName(LayerId, 0, NewMaterial.c_str());
			TerrainMaster->RefreshMaterial(Terrain_chunck);
			WriteTerrainTextureList(true);

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
			if(TerrainEditGUI->MirrorX() || TerrainEditGUI->MirrorY())
			{
				//notice that in modelling mode it may be just partsize without -1
				long OffsetX = TerrainEditGUI->MirrorX() ? partsize-1 : 0;
				long OffsetY = TerrainEditGUI->MirrorY() ? partsize-1 : 0;
				long PageOffsetX = TerrainEditGUI->MirrorX() ? mParts - 1 : 0;
				long PageOffsetY = TerrainEditGUI->MirrorY() ? mParts - 1 : 0;
				for (long XPage = 0; XPage < mParts; XPage++)
				{
					for (long YPage = 0; YPage < mParts; YPage++)
					{
						auto TargetTerrain  = TerrainMaster->getOgreTerrainGroup()->getTerrain(XPage,YPage);
						auto SourceTerrain = CopyFromTerrain->getOgreTerrainGroup()->getTerrain(glm::abs(PageOffsetX -XPage), glm::abs(PageOffsetY-YPage));
						if (TargetTerrain == nullptr || SourceTerrain == nullptr)
						{
							QSF_LOG_PRINTS(INFO, "couldnt find terrain with index "<< XPage <<" / " << YPage << " or " <<glm::abs(PageOffsetX - XPage) << " / " << glm::abs(PageOffsetY - YPage))
							continue;
						}
						//QSF_LOG_PRINTS(INFO, "acees terrain " << XPage << " / " << YPage << " or " << glm::abs(PageOffsetX - XPage) << " / " << glm::abs(PageOffsetY - YPage))
						//Copy and paste stuff
						std::string LastLayerName = "";
						std::vector<LayerData> DataToResort;
						for (size_t t = 0; t < SourceTerrain->getLayerCount(); t++)
						{
							LayerData CurrentLayer;
							CurrentLayer.LayerName = SourceTerrain->getLayerTextureName((uint8)t, 0);
							CurrentLayer.OriginLayer = (int)t;
							if (t > 0)
							{
								for (long j = 0; j < partsize; j++)
								{
									std::pair<std::string, uint16> Datapair;
									for (long k = 0; k < partsize; k++)
									{

										auto val = SourceTerrain->getLayerBlendMap((uint8)t)->getBlendValue(j, k);
										if (val != 0)
											CurrentLayer.AffectedPoints++;
										int j_mod = (int)glm::abs(OffsetX- j);
										int k_mod = (int)glm::abs(OffsetY - k);
										CurrentLayer.Data.push_back(glm::vec3(j_mod, k_mod, val));
										//if(val > 0)
										//QSF_LOG_PRINTS(INFO,val)

									}
								}
							}
							DataToResort.push_back(CurrentLayer);

						}
						//Layer loop ends here

						//now apply data to target layer
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
									while (TargetTerrain->getLayerCount() <= t)
									{
										TargetTerrain->addLayer();
									}
									if (TextureName != TargetTerrain->getLayerTextureName((uint8)t, 0))
									{
										TargetTerrain->setLayerTextureName((uint8)t, 0, TextureName.c_str());
										//QSF_LOG_PRINTS(INFO, "set layer texture name")
										LastLayerName = TextureName.c_str();
									}


								}
							}
							catch (const std::exception&)
							{

							}
						}
						//looop ended
						//now apply blend maps
						//after all layers are created we can assign Blend values
						for (size_t t = 0; t < DataToResort.size(); t++)
						{
							auto LayerData = DataToResort.at(t).Data;
							for (auto a : LayerData)
							{
								if (t > 0)
								{
									if (TargetTerrain->getLayerBlendMap((uint8)t) == nullptr)
									{
										QSF_LOG_PRINTS(INFO,"Blendmap is a nullptr")
										continue;
										
									}
									TargetTerrain->getLayerBlendMap((uint8)t)->setBlendValue(a.x,a.y,a.z);
								}

							}
							if (t > 0)
							{
								if (TargetTerrain->getLayerBlendMap((uint8)t) == nullptr)
								{
									QSF_LOG_PRINTS(INFO, "Blendmap cant be updated... is a nullptr")
										continue;

								}
								TargetTerrain->getLayerBlendMap((uint8)t)->dirty();
								TargetTerrain->getLayerBlendMap((uint8)t)->update();
							}

						}
						TerrainMaster->RefreshMaterial(TargetTerrain);

					}
				}

			}
			else //orig function
			{
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
			if(!TerrainMaster.valid() )
			{
				QSF_LOG_PRINTS(INFO,"Couldnt save terrain - terrain component was allready invalid")
				return;
			}
			//take a look how it's done in terrain edit tool (modelling)
			//we need to save 5 layers and the layerlist
			//layer 0 is always 100% and cant be read anyway
			std::string widthandheight = boost::lexical_cast<std::string>(BlendMapSize) + "x" + boost::lexical_cast<std::string>(BlendMapSize);
			Magick::Image* MagImage_1_4 = new Magick::Image();
			MagImage_1_4->size(widthandheight);
			MagImage_1_4->magick("TIF");
			MagImage_1_4->type(Magick::ImageType::TrueColorAlphaType);

			//for now we support only #5 so make it grayscale
			Magick::Image* MagImage_5_8 = new Magick::Image();
			MagImage_5_8->size(widthandheight);
			MagImage_5_8->magick("TIF");
			MagImage_5_8->type(Magick::ImageType::GrayscaleType);

			auto Quant1_4 = MagImage_1_4->getPixels(0, 0, BlendMapSize, BlendMapSize);
			auto Quant5_8 = MagImage_5_8->getPixels(0, 0, BlendMapSize, BlendMapSize);
			for (size_t x = 0; x < mParts; x++)
			{
				for (size_t y = 0; y < mParts; y++)
				{

					auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain((long)x, (long)y);
					if (Terrain == nullptr)
					{
						QSF_LOG_PRINTS(INFO, "Terrain x" << x << " Terrain y " << y << " is a nullptr")
					}
					const uint32 maximumNumberOfLayers = TMG_getMaxLayers(Terrain);
					const uint32 numberOfLayers = std::min(maximumNumberOfLayers, (uint32)Terrain->getLayerCount());
					std::vector<std::pair<std::string, int>> TerrainNames;
					//QSF_LOG_PRINTS(INFO, "we are here"<< (int)maximumNumberOfLayers<<  " ...."<< numberOfLayers  <<"...."<<  (uint32)Terrain->getLayerCount())
					int Offsetx = x*partsize;
					int Offsety = y* partsize;
					for (uint32 layerIndex = 0; layerIndex < numberOfLayers; ++layerIndex)
					{
						if (layerIndex == 0)
						{
							//just json data
							continue;
						}
						if (Terrain->getLayerBlendMap(layerIndex) == nullptr)
							continue;
						//json data + pixeldata
						//json data

						//pixel data

						auto BM = Terrain->getLayerBlendMap(layerIndex);
						for (size_t intern_x = 0; intern_x < partsize; intern_x++)
						{
							for (size_t intern_y = 0; intern_y < partsize; intern_y++)
							{
								//Read and write data
								//map 1
								if (layerIndex < 5) //1 - r 2-g  3-b 4-a
								{
									float value = BM->getBlendValue(intern_x, intern_y);
									//notice that we mirror intern_y here
									uint64 offset = ((Offsety + partsize - 1 - intern_y)* BlendMapSize + Offsetx + intern_x) * 4 + layerIndex - 1; //4 is because we use 4 channels
									//layerindex -1 is the channel selection 0 is red, 1 is green, 2 is blue and 3 is alpha. As we start at layerindex 1 we have to substract -1
									*(Quant1_4 + offset) = value * 65535.f;
								}
								else
								{
									float value = BM->getBlendValue(intern_x, intern_y);
									uint64 offset = ((Offsety + partsize - 1 - intern_y)* BlendMapSize + Offsetx + intern_x); //just one greyscale channel <-> no offset
									*(Quant5_8 + offset) = value * 65535.f;
								}
							}
						}
					}
				}
			}
			//flip?
			MagImage_1_4->flip();
			MagImage_5_8->flip();
			MagImage_1_4->syncPixels();
			MagImage_5_8->syncPixels();

			//finaaly we need to tell our asset system
			mAssetEditHelper = std::shared_ptr<qsf::editor::AssetEditHelper>(new qsf::editor::AssetEditHelper());
			auto IAP = mAssetEditHelper->getIntermediateAssetPackage();
			if (TerrainMaster->GetNewTextureMap1_4().getAsset() == nullptr)
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

					auto fileName = "terraintexture_c1_to_c4_" + Name;

					//MagImage->write(QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/heightmap/heightmap_" + Name  +  ".tif");
					QSF_LOG_PRINTS(INFO, "saved ogre img to " << QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap/" + fileName + ".tif")
						//d learn our assets a few thing <-> this seems not needed
						//delete[] buffer;

						auto Asset = mAssetEditHelper->addAsset(QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackage()->getName(), qsf::QsfAssetTypes::TEXTURE, "heightmap", fileName);
					MagImage_1_4->write(QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap/" + fileName + ".tif");
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
					TerrainMaster->SetNewTextureMap1_4(qsf::AssetProxy(Asset->getGlobalAssetId()));
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
				MagImage_1_4->write(TerrainMaster->GetNewTextureMap1_4().getAbsoluteCachedAssetDataFilename());
				std::string TargetAssetName = qsf::AssetProxy(TerrainMaster->GetNewTextureMap1_4()).getAssetPackage()->getName();
				mAssetEditHelper->tryEditAsset(qsf::AssetProxy(TerrainMaster->GetNewTextureMap1_4()).getGlobalAssetId(), TargetAssetName);
				auto CachedAsset = mAssetEditHelper->getCachedAsset(qsf::AssetProxy(TerrainMaster->GetNewTextureMap1_4()).getAsset()->getGlobalAssetId());
				/*if (CachedAsset == nullptr)
				QSF_LOG_PRINTS(INFO, "cached asset is a nullptr")
				else
				QSF_LOG_PRINTS(INFO, "cached asset isnot null?")*/
				mAssetEditHelper->setAssetUploadData(qsf::AssetProxy(TerrainMaster->GetNewTextureMap1_4()).getAsset()->getGlobalAssetId(), true, true);
				//delete[] buffer;
			}

			if (TerrainMaster->GetNewTextureMap5_8().getAsset() == nullptr)
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

					auto fileName = "terraintexture_c5_" + Name;

					//MagImage->write(QSF_FILE.getBaseDirectory() + "/" + relAssetDirectory + "/texture/heightmap/heightmap_" + Name  +  ".tif");
					QSF_LOG_PRINTS(INFO, "saved ogre img to " << QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap/" + fileName + ".tif")
						//d learn our assets a few thing <-> this seems not needed
						//delete[] buffer;

						auto Asset = mAssetEditHelper->addAsset(QSF_EDITOR_APPLICATION.getAssetImportManager().getDefaultDestinationAssetPackage()->getName(), qsf::QsfAssetTypes::TEXTURE, "heightmap", fileName);
					MagImage_5_8->write(QSF_FILE.getBaseDirectory() + "/" + IAP->getRelativeDirectory() + "/texture/heightmap/" + fileName + ".tif");
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
					TerrainMaster->SetNewTextureMap5_8(qsf::AssetProxy(Asset->getGlobalAssetId()));
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
				MagImage_5_8->write(TerrainMaster->GetNewTextureMap5_8().getAbsoluteCachedAssetDataFilename());
				std::string TargetAssetName = qsf::AssetProxy(TerrainMaster->GetNewTextureMap5_8()).getAssetPackage()->getName();
				mAssetEditHelper->tryEditAsset(qsf::AssetProxy(TerrainMaster->GetNewTextureMap5_8()).getGlobalAssetId(), TargetAssetName);
				auto CachedAsset = mAssetEditHelper->getCachedAsset(qsf::AssetProxy(TerrainMaster->GetNewTextureMap5_8()).getAsset()->getGlobalAssetId());
				/*if (CachedAsset == nullptr)
				QSF_LOG_PRINTS(INFO, "cached asset is a nullptr")
				else
				QSF_LOG_PRINTS(INFO, "cached asset isnot null?")*/
				mAssetEditHelper->setAssetUploadData(qsf::AssetProxy(TerrainMaster->GetNewTextureMap5_8()).getAsset()->getGlobalAssetId(), true, true);
				//delete[] buffer;
			}

			// Cleanup
			WriteTerrainTextureJson(mAssetEditHelper.get());
			QSF_LOG_PRINTS(INFO, "Map was saved succesfully")
				mAssetEditHelper->submit();
			//mAssetEditHelper->callWhenFinishedUploading(boost::bind(&TerrainEditTool::WaitForSaveTerrain, this, boost::function<void(bool)>(boost::bind(&TerrainEditTool::onWaitForSave_TerrainDone, this, _1))));
			TerrainMaster->setAllPropertyOverrideFlags(true);

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


		inline void TerrainTexturingTool::mouseMoveEvent(QMouseEvent & qMouseEvent)
		{
			glm::vec3 mousepos2;
			if (TerrainEditGUI != nullptr)
				Radius = TerrainEditGUI->GetBrushRadius();
			if (evaluateBrushPosition(qMouseEvent.pos(), mousepos2))
			{
				//QSF_LOG_PRINTS(INFO, mousepos2);
				oldmouspoint = mousepos2;

				//if mouse was invalid we enforce an update of our debug draw "grid"
				WriteTerrainTextureList(mouseisvalid);
				mouseisvalid = true;

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
					mChunkDrawRequestId = qsf::getUninitialized<unsigned int>();
				}
			}
		}

		void TerrainTexturingTool::UpdateChunkDebugDrawg(glm::vec3 worldpos, int x_in, int y_in)
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
				const float size = worldSize * 0.5f / TerrainMaster->kc_getTerrainChunksPerEdge();
				const glm::vec3 brushPosition = worldpos;

				glm::vec3 position = transform.getPosition();
				position.y = brushPosition.y;

				const float snapSize = worldSize / static_cast<float>(TerrainMaster->kc_getTerrainChunksPerEdge());
				const float snapSizeHalf = snapSize * 0.5f;

				position.x = snapSize*x_in + snapSizeHalf + position.x - worldSizeHalf;
				position.z = worldSizeHalf - snapSize*y_in - snapSizeHalf + position.z;
				//position.z = 1.f*-(snapSize*y_in + snapSizeHalf) + position.x + worldSizeHalf;

				transform.setPosition(position);
				transform.setScale(glm::vec3(size, 1.0f, size));
				// Tell the debug draw request about the transform
				QSF_DEBUGDRAW.setRequestTransform(mChunkDrawRequestId, transform);
			}
		}

		void TerrainTexturingTool::WriteTerrainTextureList(bool MouseWasvalid)
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
			if (m_NeedUpdatingTerrainList.x == xTerrain && m_NeedUpdatingTerrainList.y == yTerrain && m_NeedUpdatingTerrainList.z == 0)
			{
				if (!MouseWasvalid)
				{
					UpdateChunkDebugDrawg(oldmouspoint, xTerrain, yTerrain);
				}
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
				if (layerIndex != 0)
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
								if (BM->getBlendValue(x, y) > 0)
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
			SelectedChunk_x = xTerrain;
			SelectedChunk_y = yTerrain;
		}

		int TerrainTexturingTool::onAddNewTerrain(std::string BlendMapName, int x, int y)
		{
			if (BlendMapName == "")
				return -1;
			//first check if there is a layer twice
			std::vector<std::string> LayerNames;

			Ogre::Terrain* Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(x, y);
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
					QSF_LOG_PRINTS(INFO, "add completly new layer" << LayerToReplace)
				}
				else
				{
					//QSF_LOG_PRINTS(INFO, "add completly new layer failed " << Terrain->getLayerCount())
				}
			}
			if (LayerToReplace == -1)
			{
				for (size_t t = 1; t < Terrain->getLayerCount() && Terrain->getLayerCount() < 6; t++)
				{
					if (TerrainLayerIsEmpty(Terrain, (uint8)t))
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
				//QSF_LOG_PRINTS(INFO, "Did not found a good layer")
					//we may search for an empty layer
					return false;
			}

			//place layer
			if (LayerToReplace != Terrain->getLayerCount())
			{
				if (qsf::AssetProxy(BlendMapName).getAsset() == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "cant applay Blendmap with name " << BlendMapName << " unknown asset ")
						return -1;
				}
				Terrain->setLayerTextureName(LayerToReplace, 0, BlendMapName.c_str());
			}
			else
			{
				if (LayerToReplace <= 5 && Terrain->getLayerCount() <= 6)
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
				if (a == CheckMe)
					return true;
			}
			ToCheck.push_back(CheckMe);
			return false;
		}

		bool TerrainTexturingTool::TerrainLayerIsEmpty(Ogre::Terrain* Terrain, int layer)
		{
			auto BlendMap = Terrain->getLayerBlendMap(layer);
			if (BlendMap == nullptr)
				return false;
			for (size_t x = 0; x < partsize; x++)
			{
				for (size_t y = 0; y < partsize; y++)
				{
					if (BlendMap->getBlendValue(x, y) > 0)
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
			qsf::TransformComponent* TC = TerrainMaster->getEntity().getComponent<qsf::TransformComponent>();
			glm::vec3 OffsetPos = /*TC->getRotation()**/TC->getPosition();
			copy = copy - glm::vec2(OffsetPos.x, OffsetPos.z);
			copy = (copy + Offset) / TerrainMaster->getTerrainWorldSize();
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
								[](kc_vec3 const & l, kc_vec3 const & r) {return l.x == r.x && l.y == r.y; }
							),
							AffectedPoints[t][i].end()
						);

						//AffectedPoints[t][i].erase(std::unique(AffectedPoints[t][i].begin(), AffectedPoints[t][i].end()), AffectedPoints[t][i].end());
						int BlendMapIndex = GetBlendMapWithTextureName((long)t, (long)i);
						//QSF_LOG_PRINTS(INFO,BlendMapIndex);
						if (BlendMapIndex == -1)
						{
							onAddNewTerrain(GetSelectedLayerColor(), (int)t, (int)i);
							//int BlendMapIndex = GetBlendMapWithTextureName((long)t, (long)i);
							//if (BlendMapIndex == -1)
							continue;
						}

						auto Terrain = TerrainMaster->getOgreTerrainGroup()->getTerrain(t, i);
						const uint32 maximumNumberOfLayers = TMG_getMaxLayers(Terrain);
						const uint32 numberOfLayers = std::min(maximumNumberOfLayers, static_cast<uint32>(Terrain->getBlendTextureCount()));

						if (TerrainEditGUI->IsInEraseMode())
						{
							for (uint32 layerIndex = 1; layerIndex < 6; ++layerIndex)
							{
								bool MapChanged = false;
								if (Terrain->getLayerCount() <= layerIndex)
									continue;
								auto CurrentBlendMap = Terrain->getLayerBlendMap(layerIndex);
								if (CurrentBlendMap == nullptr)
									continue;
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
						}
						else //mixed intensities
						{
							for (auto points : AffectedPoints[t][i])
							{
								//if(layerIndex == 1)
								//QSF_LOG_PRINTS(INFO,points.x << " "<< points.y << " and map is << "<< t << " " << i)
								//we need to recalc why we get 64 :)
								if (points.x >= partsize || points.y >= partsize)
									continue;

								NewMixedIntensities* Mixer = new NewMixedIntensities;
								for (uint32 layerIndex = 1; layerIndex < 6; ++layerIndex)
								{
									if (Terrain->getLayerCount() <= layerIndex)
										continue;
									if (Terrain->getLayerBlendMap(layerIndex) == nullptr)
										continue;
									if (layerIndex == 1)
									{
										Mixer->IntensityLayer1 = Terrain->getLayerBlendMap(layerIndex)->getBlendValue(points.x, points.y);
									}
									else if (layerIndex == 2)
									{
										Mixer->IntensityLayer2 = Terrain->getLayerBlendMap(layerIndex)->getBlendValue(points.x, points.y);
									}
									else if (layerIndex == 3)
									{
										Mixer->IntensityLayer3 = Terrain->getLayerBlendMap(layerIndex)->getBlendValue(points.x, points.y);
									}
									else if (layerIndex == 4)
									{
										Mixer->IntensityLayer4 = Terrain->getLayerBlendMap(layerIndex)->getBlendValue(points.x, points.y);
									}
									else if (layerIndex == 5)
									{
										Mixer->IntensityLayer5 = Terrain->getLayerBlendMap(layerIndex)->getBlendValue(points.x, points.y);
									}
								}
								//put new values
								//QSF_LOG_PRINTS(INFO, "New Intensity value" << points.Intensity)
									MixIntensitiesTerrain(Mixer, BlendMapIndex, points.Intensity);
								for (uint32 layerIndex = 1; layerIndex < 6; ++layerIndex)
								{
									if (Terrain->getLayerCount() <= layerIndex)
										continue;
									if (Terrain->getLayerBlendMap(layerIndex) == nullptr)
										continue;
									if (layerIndex == 1)
									{
										Terrain->getLayerBlendMap(layerIndex)->setBlendValue(points.x, points.y, Mixer->IntensityLayer1);
									}
									else if (layerIndex == 2)
									{
										Terrain->getLayerBlendMap(layerIndex)->setBlendValue(points.x, points.y, Mixer->IntensityLayer2);
									}
									else if (layerIndex == 3)
									{
										Terrain->getLayerBlendMap(layerIndex)->setBlendValue(points.x, points.y, Mixer->IntensityLayer3);
									}
									else if (layerIndex == 4)
									{
										Terrain->getLayerBlendMap(layerIndex)->setBlendValue(points.x, points.y, Mixer->IntensityLayer4);
									}
									else if (layerIndex == 5)
									{
										Terrain->getLayerBlendMap(layerIndex)->setBlendValue(points.x, points.y, Mixer->IntensityLayer5);
									}
								}
							}
							//all points done now update blend maps
							for (uint32 layerIndex = 1; layerIndex < 6; ++layerIndex)
							{
								if (Terrain->getLayerCount() <= layerIndex)
									continue;
								auto CurrentBlendMap = Terrain->getLayerBlendMap(layerIndex);
								if (CurrentBlendMap == nullptr)
									continue;
								CurrentBlendMap->dirty();
								CurrentBlendMap->update();

							}
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
			QSF_LOG_PRINTS(INFO, "Blendmapsize" << TerrainMaster->GetBlendtMapSize());

			Offset = (float)(TerrainMaster->getTerrainWorldSize() / 2);
			BlendMapSize = (float)TerrainMaster->GetBlendtMapSize();
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
