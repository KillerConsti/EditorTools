// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "qsf_editor/PrecompiledHeader.h"
#include "qsf_editor/editmode/terrain/TerrainTexturingEditMode.h"
#include "qsf_editor/operation/terrain/TerrainOperation.h"
#include "qsf_editor/operation/terrain/TerrainDataChannel.h"
#include "qsf_editor/asset/terrain/TerrainEditHelper.h"
#include "qsf_editor/EditorHelper.h"

#include <qsf_editor_base/operation/CompoundOperation.h>

#include <qsf/QsfHelper.h>
#include <qsf/math/Math.h>
#include <qsf/map/Entity.h>
#include <qsf/renderer/terrain/TerrainComponent.h>
#include <qsf/component/base/TransformComponent.h>
#include <qsf/debug/DebugDrawManager.h>
#include <qsf/debug/request/CircleDebugDrawRequest.h>
#include <qsf/debug/request/SegmentDebugDrawRequest.h>
#include <qsf/debug/request/TriangleDebugDrawRequest.h>
#include <qsf/debug/request/RectangleDebugDrawRequest.h>

#include <OGRE/Terrain/OgreTerrainGroup.h>


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace qsf
{
	namespace editor
	{


		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		const uint32 TerrainTexturingEditMode::PLUGINABLE_ID = StringHash("qsf::editor::TerrainTexturingEditMode");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainTexturingEditMode::TerrainTexturingEditMode(EditModeManager* editModeManager) :
			TerrainEditMode(editModeManager),
			mBrushTextureIndex(0),
			mChunkDrawRequestId(getUninitialized<unsigned int>()),
			mChunkColor(1.0f, 1.0f, 1.0f),
			mMultiLayerUsed(true),
			mCurrentChunkX(getUninitialized<uint32>()),
			mCurrentChunkY(getUninitialized<uint32>())
		{
			// Nothing to do in here
		}

		TerrainTexturingEditMode::~TerrainTexturingEditMode()
		{
			// Nothing to do in here
		}

		uint32 TerrainTexturingEditMode::getBrushTextureIndex() const
		{
			return mBrushTextureIndex;
		}

		void TerrainTexturingEditMode::setBrushTextureIndex(uint32 brushTextureIndex)
		{
			mBrushTextureIndex = brushTextureIndex;
		}

		void TerrainTexturingEditMode::setChunkColor(QColor color)
		{
			qsf::Color4 chunkColor(static_cast<float>(color.redF()), static_cast<float>(color.greenF()), static_cast<float>(color.blueF()));

			if (mChunkColor != chunkColor)
			{
				mChunkColor = chunkColor;

				if (isInitialized(mChunkDrawRequestId))
				{
					// New chunk visualization
					if (QSF_DEBUGDRAW.cancelRequest(mChunkDrawRequestId)) // If everything worked
					{
						mChunkDrawRequestId = getUninitialized<unsigned int>();

						if (nullptr != getTerrainComponent())
						{
							startChunkVisualization();
						}
						else
						{
							stopChunkVisualization();
						}
					}
				}
			}
		}

		void TerrainTexturingEditMode::setMultiLayerUsed(bool multiLayerUsed)
		{
			mMultiLayerUsed = multiLayerUsed;
		}

		void TerrainTexturingEditMode::setMultiLayerIndizes(const std::vector<uint32>& multiLayerIndizes)
		{
			// Copy assignment
			mMultiLayerIndizes = multiLayerIndizes;
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::EditMode methods       ]
		//[-------------------------------------------------------]
		bool TerrainTexturingEditMode::onBrushingStartup()
		{
			// Only brush on it, when the position is valid
			if (nullptr != getTerrainComponent())
			{
				// Start with a fresh workspace
				getTerrainEditHelper().clearWorkspace();

				// Done
				return true;
			}

			// Error!
			return false;
		}

		void TerrainTexturingEditMode::onBrushing()
		{
			// Only brush on it, when the position is valid
			if (nullptr != getTerrainComponent())
			{
				editTexturePainting();
			}
		}

		void TerrainTexturingEditMode::onBrushingShutdown()
		{
			TerrainEditHelper& terrainEditHelper = getTerrainEditHelper();

			// Only brush on it, when the position is valid
			TerrainDataMap* terrainDataMap = terrainEditHelper.getTerrainDataMap(DATATYPE_BLENDMAP, false);
			QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", return);
			std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

			// Commit the operation which takes control over the values buffer
			std::vector<TerrainDataMapValue>* undoValues = new std::vector<TerrainDataMapValue>();
			std::transform(workspaceValueMap.begin(), workspaceValueMap.end(),
				std::back_inserter(*undoValues),
				boost::bind(&std::map<uint64, TerrainDataMapValue>::value_type::second, _1));
			TerrainOperation* terrainOperation = new TerrainOperation(terrainEditHelper.getEntityId(), DATATYPE_BLENDMAP, *undoValues);

			// Commit the operation
			QSF_EDITOR_OPERATION.push(terrainOperation);

			// Clear workspace
			terrainEditHelper.clearWorkspace();
		}

		void TerrainTexturingEditMode::onShutdown(EditMode* nextEditMode)
		{
			// Call the base implementation
			TerrainEditMode::onShutdown(nextEditMode);

			stopChunkVisualization();
		}

		void TerrainTexturingEditMode::mouseMoveEvent(QMouseEvent& qMouseEvent)
		{
			// Call the base implementation
			TerrainEditMode::mouseMoveEvent(qMouseEvent);

			// Chunk visualization
			if (nullptr != getTerrainComponent())
			{
				startChunkVisualization();
			}
			else
			{
				// Stop the brush visualization because the mouse is currently above no valid map position
				stopChunkVisualization();
			}
		}


		//[-------------------------------------------------------]
		//[ Private virtual qsf::editor::BrushEditMode methods    ]
		//[-------------------------------------------------------]
		void TerrainTexturingEditMode::buildBrushShape()
		{
			// Start debug draw request
			DebugDrawManager& debugDrawManager = QSF_DEBUGDRAW;
			const float size = getBrushSize();
			const float arrowBase = getBrushIntensity() * 0.5f;
			const float arrowWidth = 2;
			const float arrowDirection = mSecondFunction ? -1.0f : 1.0f;

			if (BrushData::SHAPE_SQUARE == getBrushType())
			{
				// Square shape
				mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(RectangleDebugDrawRequest(glm::vec3(-size, 0.0f, -size), glm::vec3(size, 0.0f, size), Color4(0.8f, 0.8f, 0.8f, 0.35f), 1.0f)));
				mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(RectangleDebugDrawRequest(glm::vec3(-size, 0.0f, -size), glm::vec3(size, 0.0f, size), Color4(0.8f, 0.8f, 0.8f, 0.8f), 0.0f)));
			}
			else
			{
				// Circle shape
				mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(qsf::CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, size, Color4(0.8f, 0.8f, 0.8f, 0.35f), true)));
				mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(qsf::CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, size, Color4(0.8f, 0.8f, 0.8f, 0.8f), false)));
			}

			mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(SegmentDebugDrawRequest(Segment::fromTwoPoints(Math::GLM_VEC3_ZERO, glm::vec3(0.0f, arrowBase * arrowDirection, 0.0f)), Color4(1.0f, 1.0f, 1.0f, 1.0f))));
			mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(TriangleDebugDrawRequest(glm::vec3(-arrowWidth, arrowBase * arrowDirection, 0.0f), glm::vec3(arrowWidth, arrowBase * arrowDirection, 0.0f), glm::vec3(0.0f, (arrowBase + 0.2f * arrowBase) * arrowDirection, 0.0f), Color4(1.0f, 1.0f, 1.0f, 0.5f), 0.5f)));
			mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(TriangleDebugDrawRequest(glm::vec3(0.0f, arrowBase * arrowDirection, -arrowWidth), glm::vec3(0.0f, arrowBase * arrowDirection, arrowWidth), glm::vec3(0.0f, (arrowBase + 0.2f * arrowBase) * arrowDirection, 0.0f), Color4(1.0f, 1.0f, 1.0f, 0.5f), 0.5f)));
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void TerrainTexturingEditMode::startChunkVisualization()
		{
			if (isUninitialized(mChunkDrawRequestId))
			{
				// Chunk visualisation
				mChunkDrawRequestId = QSF_DEBUGDRAW.requestDraw(RectangleDebugDrawRequest(glm::vec3(-1.0f, 0.0f, -1.0f), Math::GLM_VEC3_UNIT_XYZ, mChunkColor, 0.0f));
			}
			updateChunkVisualizationTransform();
		}

		void TerrainTexturingEditMode::updateChunkVisualizationTransform()
		{
			if (isInitialized(mChunkDrawRequestId))
			{
				// Get the terrain component instance
				const TerrainComponent* terrainComponent = getTerrainComponent();
				QSF_CHECK(nullptr != terrainComponent, "There's no terrain component instance", return);

				// Get the transform of the terrain
				Transform transform;
				{
					const TransformComponent* transformComponent = terrainComponent->getEntity().getComponent<TransformComponent>();
					if (nullptr != transformComponent)
					{
						transform = transformComponent->getTransform();
					}
				}

				const float worldSize = terrainComponent->getTerrainWorldSize();
				const float worldSizeHalf = worldSize * 0.5f;
				const float size = worldSize * 0.5f / terrainComponent->getTerrainChunksPerEdge();
				const glm::vec3 brushPosition = getBrushPosition();

				glm::vec3 position = transform.getPosition();
				position.y = brushPosition.y;

				const float snapSize = worldSize / static_cast<float>(terrainComponent->getTerrainChunksPerEdge());
				const float snapSizeHalf = snapSize * 0.5f;
				position.x = glm::round((brushPosition.x - snapSizeHalf + worldSizeHalf) / snapSize) * snapSize - worldSizeHalf + snapSizeHalf;
				position.z = glm::round((brushPosition.z - snapSizeHalf - worldSizeHalf) / snapSize) * snapSize + worldSizeHalf + snapSizeHalf;

				transform.setPosition(position);
				transform.setScale(glm::vec3(size, 1.0f, size));

				{ // Tell the world about the selected chunk
					const uint32 chunkX = glm::floor(((float)position.x + worldSizeHalf) / snapSize);
					const uint32 chunkY = glm::floor((worldSizeHalf - (float)position.z) / snapSize);
					if (mCurrentChunkX != chunkX || mCurrentChunkY != chunkY)
					{
						mCurrentChunkX = chunkX;
						mCurrentChunkY = chunkY;
						Q_EMIT chunkChanged(chunkX, chunkY);
					}
				}

				// Tell the debug draw request about the transform
				QSF_DEBUGDRAW.setRequestTransform(mChunkDrawRequestId, transform);
			}
		}

		void TerrainTexturingEditMode::stopChunkVisualization()
		{
			if (isInitialized(mChunkDrawRequestId))
			{
				QSF_DEBUGDRAW.cancelRequest(mChunkDrawRequestId);
				mChunkDrawRequestId = getUninitialized<unsigned int>();
			}
			setUninitialized(mCurrentChunkX);
			setUninitialized(mCurrentChunkY);
		}

		void TerrainTexturingEditMode::editTexturePainting()
		{
			std::vector<TerrainComponent*> terrainComponentsInfluencedByBrush;
			getTerrainComponentsInfluencedByBrush(terrainComponentsInfluencedByBrush);

			for (const TerrainComponent* terrainComponent : terrainComponentsInfluencedByBrush)
			{
				QSF_CHECK(nullptr != getTerrainComponent() && terrainComponent->getEntityId() == getTerrainComponent()->getEntityId(), "QSF invalid brush action", continue);

				if (getTerrainEditHelper().isReady())
				{
					TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_BLENDMAP);
					QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", continue);

					int channelIndex = terrainDataMap->getChannelIndexByTag(mBrushTextureIndex);
					if (channelIndex < 0)
					{
						channelIndex = terrainDataMap->addChannel();
						TerrainDataChannel* terrainDataChannel = terrainDataMap->getChannel(channelIndex);
						QSF_CHECK(nullptr != terrainDataChannel, "There's no terrain data map channel " << channelIndex, continue);
						terrainDataChannel->setTag(mBrushTextureIndex);
					}

					channelPainting(terrainComponent, terrainDataMap, channelIndex, mSecondFunction ? 0.0f : 1.0f);

					if (mMultiLayerUsed && !mSecondFunction)
					{
						setBrushSize(getBrushSize() - 2);
						for (uint32 multiLayerIndizes : mMultiLayerIndizes)
						{
							const int channelIndex = terrainDataMap->getChannelIndexByTag(multiLayerIndizes);
							if (channelIndex >= 0)		// If no channel exists there is nothing we need to delete
							{
								channelPainting(terrainComponent, terrainDataMap, channelIndex, 0.0f);
							}
						}
						setBrushSize(getBrushSize() + 2);
					}

					{ // Tell the terrain edit helper about the modified OGRE terrains
						const Ogre::TerrainGroup* ogreTerrainGroup = terrainComponent->getOgreTerrainGroup();
						if (nullptr != ogreTerrainGroup)
						{
							// Get the number of segments
							const Ogre::Terrain* ogreTerrain = terrainComponent->getOgreTerrain();
							QSF_CHECK(nullptr != ogreTerrain, "There's no OGRE terrain instance", return);
							const int chunkSegments = ogreTerrain->getSize();

							// Gather modified OGRE terrains
							std::unordered_set<Ogre::Terrain*> modifiedOgreTerrains;
							for (const auto& element : terrainDataMap->getWorkspaceValueMap())
							{
								const long chunkX = element.second.x / (chunkSegments - 1);
								const long chunkY = element.second.y / (chunkSegments - 1);
								Ogre::Terrain* ogreTerrain = ogreTerrainGroup->getTerrain(chunkX, chunkY);
								if (nullptr != ogreTerrain)
								{
									modifiedOgreTerrains.insert(ogreTerrain);
								}
							}
							getTerrainEditHelper().addToModifiedOgreTerrains(modifiedOgreTerrains);
						}
					}

					getTerrainEditHelper().updateOgreMap(DATATYPE_BLENDMAP);
				}
			}
		}

		void TerrainTexturingEditMode::channelPainting(const TerrainComponent* terrainComponent, TerrainDataMap* terrainDataMap, int channelIndex, float paint)
		{
			std::vector<TerrainDataMapValue> previewValues;
			getValueList(*terrainComponent, terrainDataMap->getWidth(), previewValues);

			std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();
			for (TerrainDataMapValue& terrainDataMapValue : previewValues)
			{
				const float d = terrainDataMapValue.value * 0.01f;

				terrainDataMapValue.channel  = channelIndex;
				terrainDataMapValue.valueOld = terrainDataMap->getValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel);
				terrainDataMapValue.value	 = glm::mix(terrainDataMapValue.valueOld, paint, d);
				terrainDataMap->setValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel, terrainDataMapValue.value);
				addTerrainValue(workspaceValueMap, terrainDataMapValue);
			}
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // qsf
