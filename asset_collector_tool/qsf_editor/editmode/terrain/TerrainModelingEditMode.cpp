// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "qsf_editor/PrecompiledHeader.h"
#include "qsf_editor/editmode/terrain/TerrainModelingEditMode.h"
#include "qsf_editor/operation/terrain/TerrainOperation.h"
#include "qsf_editor/asset/terrain/TerrainEditHelper.h"
#include "qsf_editor/selection/entity/EntitySelectionManager.h"
#include "qsf_editor/settings/EditorSettingsGroup.h"
#include "qsf_editor/EditorHelper.h"

#include <qsf_editor_base/operation/CompoundOperation.h>

#include <qsf/QsfHelper.h>
#include <qsf/map/Map.h>
#include <qsf/map/Entity.h>
#include <qsf/map/query/ComponentMapQuery.h>
#include <qsf/math/Math.h>
#include <qsf/math/Convert.h>
#include <qsf/renderer/mesh/AnalysedMesh.h>
#include <qsf/renderer/mesh/MeshComponent.h>
#include <qsf/renderer/terrain/TerrainComponent.h>
#include <qsf/component/base/TransformComponent.h>
#include <qsf/debug/DebugDrawManager.h>
#include <qsf/debug/request/CircleDebugDrawRequest.h>
#include <qsf/debug/request/SegmentDebugDrawRequest.h>
#include <qsf/debug/request/TriangleDebugDrawRequest.h>
#include <qsf/debug/request/RectangleDebugDrawRequest.h>

#include <OGRE/OgreEntity.h>
#include <OGRE/Terrain/OgreTerrain.h>


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
		const uint32 TerrainModelingEditMode::PLUGINABLE_ID = StringHash("qsf::editor::TerrainModelingEditMode");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainModelingEditMode::TerrainModelingEditMode(EditModeManager* editModeManager) :
			TerrainEditMode(editModeManager),
			mTransformMode(RAISE_LOWER_TRANSFORM),
			mHeight(0.5f)
		{
			// Nothing to do in here
		}

		TerrainModelingEditMode::~TerrainModelingEditMode()
		{
			// Nothing to do in here
		}

		TerrainModelingEditMode::TransformMode TerrainModelingEditMode::getTransformMode() const
		{
			return mTransformMode;
		}

		void TerrainModelingEditMode::setTransformMode(TransformMode transformMode)
		{
			mTransformMode = transformMode;
		}

		float TerrainModelingEditMode::getHeight() const
		{
			return mHeight;
		}

		void TerrainModelingEditMode::setHeight(float height)
		{
			mHeight = height;

			// Don't emit the "heightPicked"-signal in here
		}

		void TerrainModelingEditMode::setHeightValuesToSelection()
		{
			TerrainComponent* terrainComponent = editFirstFoundTerrainComponent();
			if (nullptr != terrainComponent)
			{
				// Get the currently selected entities
				const EntitySelectionManager::IdSet& idSet = QSF_EDITOR_SELECTION_SYSTEM.getSafe<editor::EntitySelectionManager>().getSelectedIdSet();

				// If no entities are selected, we exit this method
				if (idSet.empty())
				{
					// TODO(co) Disable GUI button so the user can't press it in the first place
					QSF_ERROR("Select some entities for this operation", QSF_REACT_NONE);
				}
				else
				{
					// Create compound operation to collect multiple operations and operation data
					base::CompoundOperation* compoundOperation = new base::CompoundOperation();

					// Flatten per entity
					const Map& map = terrainComponent->getEntity().getMap();
					unsigned int numberOfEntities = 0;
					for (uint64 entityId : idSet)
					{
						const Entity* entity = map.getEntityById(entityId);
						if (nullptr != entity)
						{
							// Start with a fresh workspace
							getTerrainEditHelper().clearWorkspace();

							// Manipulate height map values
							setHeightValuesToEntity(*entity);

							TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_HEIGHTMAP, false);
							QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", continue);
							const std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

							// Commit the operation which takes control over the values buffer
							std::vector<TerrainDataMapValue>* undoValues = new std::vector<TerrainDataMapValue>();
							std::transform(workspaceValueMap.begin(), workspaceValueMap.end(),
								std::back_inserter(*undoValues),
								boost::bind(&std::map<uint64, TerrainDataMapValue>::value_type::second, _1));
							compoundOperation->pushBackOperation(new TerrainOperation(terrainComponent->getEntityId(), DATATYPE_HEIGHTMAP, *undoValues));

							++numberOfEntities;

							// Cleanup with a fresh workspace
							getTerrainEditHelper().clearWorkspace();
						}
					}

					// Set text for compound operation
					if (numberOfEntities > 1)
					{
						compoundOperation->setText("Flatten Terrain using " + std::to_string(numberOfEntities) + " Entities");
					}
					else
					{
						compoundOperation->setText("Flatten Terrain using 1 Entity");
					}

					// Commit the operation
					QSF_EDITOR_OPERATION.push(compoundOperation);
				}
			}
		}

		void TerrainModelingEditMode::applyGlobalHeightSmooth()
		{
			TerrainComponent* terrainComponent = editFirstFoundTerrainComponent();
			if (nullptr != terrainComponent)
			{
				// Start with a fresh workspace
				getTerrainEditHelper().clearWorkspace();

				applyGlobalHeightSmoothInternal();

				TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_HEIGHTMAP, false);
				QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", return);

				// Commit the operation which takes control over the values buffer
				std::vector<TerrainDataMapValue>* undoValues = new std::vector<TerrainDataMapValue>();
				for (const TerrainDataMapValue& terrainDataMapValue : terrainDataMap->getWorkspaceValueVector())
				{
					undoValues->push_back(terrainDataMapValue);
				}

				// Create compound operation to collect multiple operations and operation data
				base::CompoundOperation* compoundOperation = new base::CompoundOperation();

				compoundOperation->pushBackOperation(new TerrainOperation(terrainComponent->getEntityId(), DATATYPE_HEIGHTMAP, *undoValues));

				compoundOperation->setText("Height Map Global Smooth");

				// Commit the operation
				QSF_EDITOR_OPERATION.push(compoundOperation);

				// Cleanup with a fresh workspace
				getTerrainEditHelper().clearWorkspace();
			}
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::BrushEditMode methods  ]
		//[-------------------------------------------------------]
		bool TerrainModelingEditMode::onBrushingStartup()
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

		void TerrainModelingEditMode::onBrushing()
		{
			// Only brush on it, when the position is valid
			if (nullptr != getTerrainComponent())
			{
				// Get the used terrain component instance
				switch (mTransformMode)
				{
					case RAISE_LOWER_TRANSFORM:
						editHeightRaise();
						break;

					case SMOOTH_TRANSFORM:
						editHeightSmooth();
						break;

					case SET_PICK_TRANSFORM:
						if (mSecondFunction)
						{
							mHeight = getHeightAt();

							// Tell the world about the wonderful news
							Q_EMIT heightPicked();
						}
						else
						{
							editHeightSet();
						}
						break;
				}
			}
		}

		void TerrainModelingEditMode::onBrushingShutdown()
		{
			TerrainEditHelper& terrainEditHelper = getTerrainEditHelper();

			// Only brush on it, when the position is valid
			if (!(mTransformMode == SET_PICK_TRANSFORM && mSecondFunction))
			{
				TerrainDataMap* terrainDataMap = terrainEditHelper.getTerrainDataMap(DATATYPE_HEIGHTMAP, false);
				QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", return);
				std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

				// Commit the operation which takes control over the values buffer
				std::vector<TerrainDataMapValue>* undoValues = new std::vector<TerrainDataMapValue>();
				std::transform(workspaceValueMap.begin(), workspaceValueMap.end(),
					std::back_inserter(*undoValues),
					boost::bind(&std::map<uint64, TerrainDataMapValue>::value_type::second, _1));
				TerrainOperation* terrainOperation = new TerrainOperation(terrainEditHelper.getEntityId(), DATATYPE_HEIGHTMAP, *undoValues);

				// Commit the operation
				QSF_EDITOR_OPERATION.push(terrainOperation);
			}

			// Cleanup with a fresh workspace
			terrainEditHelper.clearWorkspace();
		}


		//[-------------------------------------------------------]
		//[ Private virtual qsf::editor::BrushEditMode methods    ]
		//[-------------------------------------------------------]
		void TerrainModelingEditMode::buildBrushShape()
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
				mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(RectangleDebugDrawRequest(glm::vec3(-arrowWidth, 0.0f, -arrowWidth), glm::vec3(arrowWidth, 0.0f, arrowWidth), Color4(1.0f, 1.0f, 1.0f, 0.5f), 1.0f)));
			}
			else
			{
				// Circle shape
				mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, size, Color4(0.8f, 0.8f, 0.8f, 0.35f), true)));
				mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, size, Color4(0.8f, 0.8f, 0.8f, 0.8f), false)));
				mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, arrowWidth, Color4(1.0f, 1.0f, 1.0f, 0.5f), true)));
			}

			mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(SegmentDebugDrawRequest(Segment::fromTwoPoints(Math::GLM_VEC3_ZERO, glm::vec3(0.0f, arrowBase * arrowDirection, 0.0f)), Color4(1.0f, 1.0f, 1.0f, 1.0f))));
			mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(TriangleDebugDrawRequest(glm::vec3(-arrowWidth, arrowBase * arrowDirection, 0.0f), glm::vec3(arrowWidth, arrowBase * arrowDirection, 0.0f), glm::vec3(0.0f, (arrowBase + 0.2f * arrowBase) * arrowDirection, 0.0f), Color4(1.0f, 1.0f, 1.0f, 0.5f), 0.5f)));
			mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(TriangleDebugDrawRequest(glm::vec3(0.0f, arrowBase * arrowDirection, -arrowWidth), glm::vec3(0.0f, arrowBase * arrowDirection, arrowWidth), glm::vec3(0.0f, (arrowBase + 0.2f * arrowBase) * arrowDirection, 0.0f), Color4(1.0f, 1.0f, 1.0f, 0.5f), 0.5f)));
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void TerrainModelingEditMode::updateOgreTerrainHeightMapDirectly(const TerrainComponent& terrainComponent, const std::vector<TerrainDataMapValue>& newValues) const
		{
			const uint64 entityId = terrainComponent.getEntityId();
			QSF_CHECK(nullptr != getTerrainComponent() && entityId == getTerrainComponent()->getEntityId(), "QSF invalid brush action", QSF_REACT_THROW);

			Ogre::TerrainGroup* ogreTerrainGroup = terrainComponent.getOgreTerrainGroup();
			if (nullptr != ogreTerrainGroup)
			{
				// Get the number of segments
				const Ogre::Terrain* ogreTerrain = terrainComponent.getOgreTerrain();
				QSF_CHECK(nullptr != ogreTerrain, "There's no OGRE terrain instance", return);
				const int chunkSegments = ogreTerrain->getSize();

				// Tell the OGRE terrain instance about the new height values
				std::unordered_set<Ogre::Terrain*> modifiedOgreTerrains;
				TerrainEditHelper& terrainEditHelper = getTerrainEditHelper();
				for (const TerrainDataMapValue& value : newValues)
				{
					terrainEditHelper.setOgreTerrainHeight(value.x, value.y, value.value, chunkSegments, *ogreTerrainGroup, modifiedOgreTerrains);
				}

				// Ensure the OGRE terrain geometry is up-to-date
				for (Ogre::Terrain* ogreTerrain : modifiedOgreTerrains)
				{
					ogreTerrain->updateGeometryWithoutNotifyNeighbours();
				}
			}
		}

		void TerrainModelingEditMode::editHeightRaise()
		{
			std::vector<TerrainComponent*> terrainComponentsInfluencedByBrush;
			getTerrainComponentsInfluencedByBrush(terrainComponentsInfluencedByBrush);

			for (const TerrainComponent* terrainComponent : terrainComponentsInfluencedByBrush)
			{
				QSF_CHECK(nullptr != getTerrainComponent() && terrainComponent->getEntityId() == getTerrainComponent()->getEntityId(), "QSF invalid brush action", continue);
				TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_HEIGHTMAP);
				QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", continue);
				std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

				std::vector<TerrainDataMapValue> previewValues;
				getValueList(*terrainComponent, terrainDataMap->getWidth(), previewValues);

				for (TerrainDataMapValue& terrainDataMapValue : previewValues)
				{
					// Because Marcel Gonzales wishes more detail in small intensity areas, we power the intensity
					terrainDataMapValue.value = glm::pow(terrainDataMapValue.value * 0.01f, 1.5f) * 100.0f;

					const float intensity = mSecondFunction ? -terrainDataMapValue.value : terrainDataMapValue.value;
					const float heightAtPoint = terrainDataMap->getValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, 0);

					// Collect new and previous value
					terrainDataMapValue.valueOld = heightAtPoint;
					terrainDataMapValue.value = heightAtPoint + intensity * 0.1f;

					addTerrainValue(workspaceValueMap, terrainDataMapValue);

					terrainDataMap->setValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, 0, terrainDataMapValue.value);
				}

				updateOgreTerrainHeightMapDirectly(*terrainComponent, previewValues);
			}
		}

		void TerrainModelingEditMode::editHeightSmooth()
		{
			// Smoothing size
			const int smoothWindow = 5;

			std::vector<TerrainComponent*> terrainComponentsInfluencedByBrush;
			getTerrainComponentsInfluencedByBrush(terrainComponentsInfluencedByBrush);

			for (const TerrainComponent* terrainComponent : terrainComponentsInfluencedByBrush)
			{
				QSF_CHECK(nullptr != getTerrainComponent() && terrainComponent->getEntityId() == getTerrainComponent()->getEntityId(), "QSF invalid brush action", continue);
				TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_HEIGHTMAP);
				QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", continue);
				std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

				std::vector<TerrainDataMapValue> previewValues;
				getValueList(*terrainComponent, terrainDataMap->getWidth(), previewValues);

				for (TerrainDataMapValue& terrainDataMapValue : previewValues)
				{
					// Accumulate all values we need and calculate the average
					float valueSmooth = 0.0f;
					int smoothCount = 0;
					for (int yy = terrainDataMapValue.y - smoothWindow; yy <= terrainDataMapValue.y + smoothWindow; ++yy)
					{
						for (int xx = terrainDataMapValue.x - smoothWindow; xx <= terrainDataMapValue.x + smoothWindow; ++xx)
						{
							if (terrainDataMap->isValid(xx, yy))
							{
								valueSmooth += terrainDataMap->getValueAtPosition(xx, yy, 0);
								++smoothCount;
							}
						}
					}

					if (smoothCount > 1)
					{
						valueSmooth /= smoothCount;

						// Collect new and previous value
						const float heightAtPoint = terrainDataMap->getValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, 0);
						terrainDataMapValue.valueOld = heightAtPoint;
						terrainDataMapValue.value = glm::mix(heightAtPoint, valueSmooth, glm::clamp(terrainDataMapValue.value * 0.01f, 0.0f, 1.0f));
					}
				}

				for (const TerrainDataMapValue& terrainDataMapValue : previewValues)
				{
					addTerrainValue(workspaceValueMap, terrainDataMapValue);
					terrainDataMap->setValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, 0, terrainDataMapValue.value);
				}

				updateOgreTerrainHeightMapDirectly(*terrainComponent, previewValues);
			}
		}

		float TerrainModelingEditMode::getHeightAt() const
		{
			float result = 0.0f;

			std::vector<TerrainComponent*> terrainComponentsInfluencedByBrush;
			getTerrainComponentsInfluencedByBrush(terrainComponentsInfluencedByBrush);

			for (const TerrainComponent* terrainComponent : terrainComponentsInfluencedByBrush)
			{
				QSF_CHECK(nullptr != getTerrainComponent() && terrainComponent->getEntityId() == getTerrainComponent()->getEntityId(), "QSF invalid brush action", continue);
				const TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_HEIGHTMAP);
				QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", continue);

				const glm::vec3 point = worldToPointSpace(*terrainComponent, getBrushPosition());

				const int x = glm::round(point.x);
				const int y = glm::round(point.y);

				if (terrainDataMap->isValid(x, y))
				{
					result = terrainDataMap->getValueAtPosition(x, y, 0);
					const glm::vec3 resultVec = localToWorldSpace(*terrainComponent, glm::vec3(0.0f, result, 0.0f));
					result = resultVec.y;
				}
			}

			return result;
		}

		void TerrainModelingEditMode::editHeightSet()
		{
			std::vector<TerrainComponent*> terrainComponentsInfluencedByBrush;
			getTerrainComponentsInfluencedByBrush(terrainComponentsInfluencedByBrush);

			for (const TerrainComponent* terrainComponent : terrainComponentsInfluencedByBrush)
			{
				QSF_CHECK(nullptr != getTerrainComponent() && terrainComponent->getEntityId() == getTerrainComponent()->getEntityId(), "QSF invalid brush action", continue);
				TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_HEIGHTMAP);
				QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", continue);
				std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

				const glm::vec3 heightVec = worldToLocalSpace(*terrainComponent, glm::vec3(0.0f, mHeight, 0.0f));
				const float height = heightVec.y;

				std::vector<TerrainDataMapValue> previewValues;
				getValueList(*terrainComponent, terrainDataMap->getWidth(), previewValues);

				for (TerrainDataMapValue& terrainDataMapValue : previewValues)
				{
					const float heightAtPoint = terrainDataMap->getValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, 0);

					// Collect new and previous value
					terrainDataMapValue.valueOld = heightAtPoint;
					terrainDataMapValue.value = glm::mix(heightAtPoint, height, glm::clamp(terrainDataMapValue.value * 0.1f, 0.0f, 1.0f));

					addTerrainValue(workspaceValueMap, terrainDataMapValue);

					terrainDataMap->setValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, 0, terrainDataMapValue.value);
				}

				updateOgreTerrainHeightMapDirectly(*terrainComponent, previewValues);
			}
		}

		void TerrainModelingEditMode::setHeightValuesToEntity(const Entity& entity)
		{
			// If there is no mesh in entity, we can exit this method
			const MeshComponent* meshComponent = entity.getComponent<MeshComponent>();
			if (nullptr == meshComponent)
			{
				return;
			}

			// TODO(co) I assume that later on we have a shared used mesh analysing manager instance
			AnalysedMesh meshAnalyser;
			meshAnalyser.analyseBaseMesh(*meshComponent->getOgreEntity()->getMesh().getPointer());

			{ // Transform mesh vertices from local space to world space
				const TransformComponent* transformComponent = entity.getComponent<TransformComponent>();
				if (nullptr != transformComponent)
				{
					meshAnalyser.transformVertices(transformComponent->getTransform());
				}
			}

			glm::vec3 minBound, maxBound;
			meshAnalyser.getBoxAabb(minBound, maxBound);

			// Backup the current brush size
			const float oldBrushSize = getBrushSize();

			// Set new brush position and size
			setBrushPosition(glm::mix(minBound, maxBound, 0.5f));
			setBrushSize(glm::max(getBrushPosition().x - minBound.x, getBrushPosition().z - minBound.z));

			std::vector<TerrainComponent*> terrainComponentsInfluencedByBrush;
			getTerrainComponentsInfluencedByBrush(terrainComponentsInfluencedByBrush);

			const float flattenOffset = EditorSettingsGroup::getInstanceSafe().getTerrainFlattenOffset();

			for (const TerrainComponent* terrainComponent : terrainComponentsInfluencedByBrush)
			{
				QSF_CHECK(nullptr != getTerrainComponent() && terrainComponent->getEntityId() == getTerrainComponent()->getEntityId(), "QSF invalid brush action", continue);
				TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_HEIGHTMAP);
				QSF_CHECK(nullptr != terrainDataMap, "QSF editor terrain modeling edit mode is using an invalid terrain data map", continue);
				std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

				const float worldSize = terrainComponent->getTerrainWorldSize();
				const int segments = terrainComponent->getTerrainSegments() - 1;

				const glm::vec3 minBoundTerrain = worldToLocalSpace(*terrainComponent, minBound);
				const glm::vec3 maxBoundTerrain = worldToLocalSpace(*terrainComponent, maxBound);

				const int pointXMin = glm::clamp(int(floor((minBoundTerrain.x / worldSize + 0.5f)  * segments)), 0, segments);
				const int pointZMin = glm::clamp(int(ceil((-maxBoundTerrain.z / worldSize + 0.5f) * segments)), 0, segments);
				int pointXMax = glm::clamp(int(ceil((maxBoundTerrain.x / worldSize + 0.5f) * segments)), 0, segments);
				int pointZMax = glm::clamp(int(floor((-minBoundTerrain.z / worldSize + 0.5f) * segments)), 0, segments);

				// TODO(co) Please review this, had to make a hotfix for bug H476
				// Ensure we never leave the data map boundaries
				const int terrainDataMapWidth = terrainDataMap->getWidth();
				const int terrainDataMapHeight = terrainDataMap->getHeight();
				if (pointXMax >= terrainDataMapWidth)
				{
					pointXMax = terrainDataMapWidth - 1;
				}
				if (pointZMax >= terrainDataMapHeight)
				{
					pointZMax = terrainDataMapHeight - 1;
				}

				std::vector<TerrainDataMapValue> flattening;

				// Iterate over all points, that could be affected by the brush
				for (int y = pointZMin; y <= pointZMax; ++y)
				{
					for (int x = pointXMin; x <= pointXMax; ++x)
					{
						const glm::vec3 worldSpacePoint = localToWorldSpace(*terrainComponent, glm::vec3(
							worldSize / segments * x - worldSize * 0.5f,
							-1000.0f,
							-worldSize / segments * y + worldSize * 0.5f));

						glm::vec3 intersection;
						if (meshAnalyser.intersectUp(worldSpacePoint, intersection))
						{
							const glm::vec3 height = worldToLocalSpace(*terrainComponent, glm::vec3(0.0f, intersection.y - flattenOffset, 0.0f));

							// Collect new and previous value
							TerrainDataMapValue value;
							value.x = x;
							value.y = y;
							value.channel = 0;
							value.value = height.y;
							flattening.push_back(value);
						}
					}
				}

				std::vector<TerrainDataMapValue>* previewValues = new std::vector<TerrainDataMapValue>;

				{ // Smoothing path
					for (const TerrainDataMapValue& flattenValue : flattening)
					{
						for (int yy = -1; yy <= 1; ++yy)
						{
							for (int xx = -1; xx <= 1; ++xx)
							{
								if (0 != xx || 0 != yy)
								{
									if (terrainDataMap->isValid(flattenValue.x + xx, flattenValue.y + yy))
									{
										TerrainDataMapValue value;
										value.x		   = flattenValue.x + xx;
										value.y		   = flattenValue.y + yy;
										value.channel  = 0;
										value.valueOld = terrainDataMap->getValueAtPosition(value.x, value.y, 0);
										value.value	   = flattenValue.value;

										addTerrainValue(workspaceValueMap, value);

										previewValues->push_back(value);
									}
								}
							}
						}
					}

					for (const TerrainDataMapValue& flattenValue : flattening)
					{
						TerrainDataMapValue value;
						value.x		   = flattenValue.x;
						value.y		   = flattenValue.y;
						value.channel  = 0;
						value.valueOld = terrainDataMap->getValueAtPosition(value.x, value.y, 0);
						value.value    = flattenValue.value;

						addTerrainValue(workspaceValueMap, value);

						previewValues->push_back(value);
					}
				}

				updateOgreTerrainHeightMapDirectly(*terrainComponent, *previewValues);

				delete previewValues;
			}

			setBrushSize(oldBrushSize);
		}

		void TerrainModelingEditMode::applyGlobalHeightSmoothInternal()
		{
			for (TerrainComponent* terrainComponent : ComponentMapQuery(QSF_MAINMAP).getAllInstances<TerrainComponent>())
			{
				// Smoothing size
				const int smoothWindow = 2;

				QSF_CHECK(nullptr != getTerrainComponent() && terrainComponent->getEntityId() == getTerrainComponent()->getEntityId(), "QSF invalid brush action", continue);
				TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_HEIGHTMAP);
				QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", continue);
				std::vector<TerrainDataMapValue>& workspaceValueVector = terrainDataMap->getWorkspaceValueVector();

				// Get the size of terrain
				const int terrainSize = terrainDataMap->getWidth();

				// Iterate over all points, that could be affected by the brush
				for (int y = 0; y < terrainSize; ++y)
				{
					for (int x = 0; x < terrainSize; ++x)
					{
						int smoothFactor = 0;

						// Accumulate all values we need and calculate the average
						float valueSmooth = 0.0f;
						for (int yy = y - smoothWindow; yy <= y + smoothWindow; ++yy)
						{
							for (int xx = x - smoothWindow; xx <= x + smoothWindow; ++xx)
							{
								if (terrainDataMap->isValid(xx, yy))
								{
									valueSmooth += terrainDataMap->getValueAtPosition(xx, yy, 0);
									++smoothFactor;
								}
							}
						}

						if (smoothFactor > 0)
						{
							valueSmooth /= smoothFactor;

							const float heightAtPoint = terrainDataMap->getValueAtPosition(x, y, 0);

							// Collect new and previous value
							TerrainDataMapValue value;
							value.x		   = x;
							value.y		   = y;
							value.valueOld = heightAtPoint;
							value.value	   = valueSmooth;
							workspaceValueVector.push_back(value);
						}
					}
				}

				updateOgreTerrainHeightMapDirectly(*terrainComponent, workspaceValueVector);
			}
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // qsf
