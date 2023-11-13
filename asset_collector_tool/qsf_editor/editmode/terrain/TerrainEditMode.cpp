// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool\qsf_editor\editmode\terrain\TerrainEditMode.h>
#include "qsf_editor/asset/terrain/TerrainEditHelper.h"
#include "qsf_editor/asset/terrain/TerrainEditManager.h"
#include "qsf_editor/renderer/RenderView.h"
#include "qsf_editor/EditorHelper.h"

#include <qsf/QsfHelper.h>
#include <qsf/map/Entity.h>
#include <qsf/map/query/ComponentMapQuery.h>
#include <qsf/math/Math.h>
#include <qsf/math/Plane.h>
#include <qsf/math/Convert.h>
#include <qsf/renderer/window/RenderWindow.h>
#include <qsf/renderer/component/CameraComponent.h>
#include <qsf/renderer/terrain/TerrainComponent.h>
#include <qsf/component/base/TransformComponent.h>
#include "TerrainEditMode.h"


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
		const uint32 TerrainEditMode::PLUGINABLE_ID = StringHash("qsf::editor::TerrainEditMode");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainEditMode::~TerrainEditMode()
		{
			// Nothing to do in here
		}


		//[-------------------------------------------------------]
		//[ Protected methods                                     ]
		//[-------------------------------------------------------]
		TerrainEditMode::TerrainEditMode(EditModeManager* editModeManager) :
			EditMode(editModeManager),
			mTerrainComponent(nullptr)
		{
			// Nothing to do in here
		}

		TerrainEditMode::BrushData TerrainEditMode::getBrushType() const
		{
			return BrushData();
		}

		float TerrainEditMode::getBrushIntensity() const
		{
			return 0.0f;
		}

		glm::vec3 TerrainEditMode::worldToLocalSpace(const TerrainComponent& terrainComponent, const glm::vec3& position) const
		{
			const TransformComponent* transformComponent = terrainComponent.getEntity().getComponent<TransformComponent>();
			return (nullptr != transformComponent) ? transformComponent->getTransform().vec3PositionWorldToLocal(position) : position;
		}

		glm::vec3 TerrainEditMode::localToPointSpace(const TerrainComponent& terrainComponent, const glm::vec3& position) const
		{
			const float worldSize = terrainComponent.getTerrainWorldSize();
			const int segments = terrainComponent.getTerrainSegments();

			const float x = glm::clamp((position.x / worldSize + 0.5f) * segments, 0.0f, segments - 1.0f);
			const float y = glm::clamp((1.0f - (position.z / worldSize + 0.5f)) * segments, 0.0f, segments - 1.0f);

			return glm::vec3(x, y, 0.0f);
		}

		glm::vec3 TerrainEditMode::worldToPointSpace(const TerrainComponent& terrainComponent, const glm::vec3& position) const
		{
			return localToPointSpace(terrainComponent, worldToLocalSpace(terrainComponent, position));
		}

		glm::vec3 TerrainEditMode::localToWorldSpace(const TerrainComponent& terrainComponent, const glm::vec3& position) const
		{
			const TransformComponent* transformComponent = terrainComponent.getEntity().getComponent<TransformComponent>();
			return (nullptr != transformComponent) ? transformComponent->getTransform().vec3PositionLocalToWorld(position) : position;
		}

		/*void TerrainEditMode::addTerrainValue(std::map<uint64, TerrainDataMapValue>& values, const TerrainDataMapValue& addValue) const
		{
			// Get the key for this value
			const uint64 key = (((static_cast<uint64>(addValue.x) << 16) + static_cast<uint64>(addValue.y)) << 16) + static_cast<uint64>(addValue.channel);

			// Check whether or not the value is already inside our map
			std::map<uint64, TerrainDataMapValue>::iterator iterator = values.find(key);

			// Replace or add new value
			if (values.end() != iterator)
			{
				// Replace value
				iterator->second.value = addValue.value;
			}
			else
			{
				// Add new value
				values[key] = addValue;
			}
		}*/

		glm::vec3 TerrainEditMode::getBrushPosition() const
		{
			return glm::vec3();
		}

		float TerrainEditMode::getBrushSize() const
		{
			return 0.0f;
		}

		void TerrainEditMode::getEditArea(const TerrainComponent& terrainComponent, const glm::vec3& position, int width, int& pointXMin, int& pointXMax, int& pointZMin, int& pointZMax) const
		{
			const float worldSize = terrainComponent.getTerrainWorldSize();
			const int w = width - 1;
			pointXMin = glm::clamp(int(floor((( position.x - getBrushSize()) / worldSize + 0.5f) * w)), 0, w);
			pointZMin = glm::clamp(int(floor(((-position.z - getBrushSize()) / worldSize + 0.5f) * w)), 0, w);
			pointXMax = glm::clamp(int(ceil(((  position.x + getBrushSize()) / worldSize + 0.5f) * w)), 0, w);
			pointZMax = glm::clamp(int(ceil((( -position.z + getBrushSize()) / worldSize + 0.5f) * w)), 0, w);
		}

		float TerrainEditMode::intensityByBrush(const TerrainComponent& terrainComponent, const glm::vec3& position, float u, float v) const
		{
			const float worldSize = terrainComponent.getTerrainWorldSize();

			glm::vec3 positionWithOffset = glm::vec3(
				worldSize * u - worldSize * 0.5f - position.x,
				0.0f,
				-worldSize * v + worldSize * 0.5f - position.z);

			float result = 0.0f;

			{ // Evaluate the brush shape
				if (BrushData::SHAPE_SQUARE != getBrushType())
				{
					result = glm::length(positionWithOffset);
					result /= getBrushSize();
				}
				else
				{
					result = std::abs(positionWithOffset.x / getBrushSize()) * std::abs(positionWithOffset.z / getBrushSize());
				}

				result = glm::clamp(result, 0.0f, 1.0f);

				if (BrushData::SHAPE_DOME == getBrushType())
				{
					result = glm::cos(result * glm::pi<float>()) * 0.5f + 0.5f;
				}
				else if (BrushData::SHAPE_CONE == getBrushType())
				{
					result = 1.0f - result;
				}
				else if (BrushData::SHAPE_CIRCLE == getBrushType() || BrushData::SHAPE_SQUARE == getBrushType())
				{
					result = 1.0f - result;
					if (result > 0.001f)
					{
						result = 1.0f;
					}
				}
			}

			// Apply brush intensity
			result *= getBrushIntensity();

			// Add some random noise the brush intensity
			result = glm::mix(result, result * (rand() % 1001) / 1000.0f, getBrushRandomness());

			// Done
			return result;
		}

		void TerrainEditMode::getTerrainComponentsInfluencedByBrush(std::vector<TerrainComponent*>& terrainComponentsInfluencedByBrush) const
		{
			// Get the axis aligned bounding box of the brush
			const Ogre::AxisAlignedBox box = Ogre::AxisAlignedBox(Ogre::Vector3(getBrushPosition().x - getBrushSize(), -1000.0f, getBrushPosition().z - getBrushSize()),
																  Ogre::Vector3(getBrushPosition().x + getBrushSize(),  1000.0f, getBrushPosition().z + getBrushSize()));

			// Check which terrain components are influenced by the brush
			for (TerrainComponent* terrainComponent : ComponentMapQuery(QSF_MAINMAP).getAllInstances<TerrainComponent>())
			{
				const float worldSize = terrainComponent->getTerrainWorldSize() / 2.0f;
				const glm::vec3 terrainPosition = localToWorldSpace(*terrainComponent, Math::GLM_VEC3_ZERO);

				// Get the axis aligned bounding box of the current terrain component
				const Ogre::AxisAlignedBox boxTerrain = Ogre::AxisAlignedBox(Ogre::Vector3(terrainPosition.x - worldSize, -1000.0f, terrainPosition.z - worldSize), Ogre::Vector3(terrainPosition.x + worldSize, 1000.0f, terrainPosition.z + worldSize));

				// Do the axis aligned bounding boxes of the brush and current terrain component intersect?
				if (box.intersects(boxTerrain))
				{
					terrainComponentsInfluencedByBrush.push_back(terrainComponent);
				}
			}
		}

		void TerrainEditMode::getValueList(const TerrainComponent& terrainComponent, int width, std::vector<TerrainDataMapValue>& valueList) const
		{
			const glm::vec3 localPosition = worldToLocalSpace(terrainComponent, getBrushPosition());

			// Get edit area
			int pointXMin = 0;
			int pointZMin = 0;
			int pointXMax = 0;
			int pointZMax = 0;
			getEditArea(terrainComponent, localPosition, width, pointXMin, pointXMax, pointZMin, pointZMax);

			// Iterate over all points, that could be affected by the brush
			for (uint16 y = pointZMin; y <= pointZMax; ++y)
			{
				for (uint16 x = pointXMin; x <= pointXMax; ++x)
				{
					const float d = intensityByBrush(terrainComponent, localPosition, (x + 0.5f) / static_cast<float>(width), (y + 0.5f) / static_cast<float>(width));
					if (std::abs(d) > 0.0f)
					{
						// Collect new and previous value
						TerrainDataMapValue value;
						value.x		   = x;
						value.y		   = y;
						value.channel  = 0;
						value.value	   = d;
						value.valueOld = d;

						valueList.push_back(value);
					}
				}
			}
		}

		TerrainComponent* TerrainEditMode::editFirstFoundTerrainComponent()
		{
			if (nullptr == mTerrainComponent)
			{
				mTerrainComponent = ComponentMapQuery(QSF_MAINMAP).getFirstInstance<TerrainComponent>();
				mTerrainEditHelper = QSF_EDITOR_TERRAINEDIT.findTerrainEditHelper(mTerrainComponent->getEntityId());
				if (nullptr == mTerrainEditHelper || !mTerrainEditHelper->isGood() || !mTerrainEditHelper->isReady())
				{
					mTerrainComponent = nullptr;
				}
			}
			return mTerrainComponent;
		}

		TerrainComponent* TerrainEditMode::getTerrainComponent() const
		{
			return mTerrainComponent;
		}

		TerrainEditHelper& TerrainEditMode::getTerrainEditHelper() const
		{
			QSF_CHECK(nullptr != mTerrainEditHelper, "QSF terrain edit mode is trying to access a terrain that didn't exist on tool startup", QSF_REACT_THROW);
			return *mTerrainEditHelper;
		}



		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::BrushEditMode methods  ]
		//[-------------------------------------------------------]
		bool TerrainEditMode::evaluateBrushPosition(const QPoint& mousePosition, glm::vec3& position)
		{
			float closestDistance = -1.0f;
			mTerrainComponent = nullptr;

			// Get the camera component the render window is using
			const RenderWindow& renderWindow = getRenderView().getRenderWindow();
			const CameraComponent* cameraComponent = renderWindow.getCameraComponent();
			if (nullptr != cameraComponent)
			{
				// Get the normalized mouse position
				const glm::vec2 normalizedPosition = renderWindow.getNormalizedWindowSpaceCoords(mousePosition.x(), mousePosition.y());

				// Get a ray at the given viewport position
				const Ray ray = cameraComponent->getRayAtViewportPosition(normalizedPosition.x, normalizedPosition.y);
				Ogre::Ray ogreRay = Convert::getOgreRay(ray);

				for (TerrainComponent* terrainComponent : ComponentMapQuery(QSF_MAINMAP).getAllInstances<TerrainComponent>())
				{
					if (terrainComponent->getTerrainHitBoundingBoxByRay(ogreRay))
					{
						// Get terrain intersection
						glm::vec3 hitPosition;
						if (terrainComponent->getTerrainHitByRay(ogreRay, &hitPosition))
						{
							// Get distance to intersection
							const float distance = glm::length(hitPosition - Convert::getGlmVec3(ogreRay.getOrigin()));
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
					const Plane plane = Plane(position, Math::GLM_VEC3_UNIT_Y);
					Math::intersectRayWithPlane(ray, plane, &position, nullptr);
					closestDistance = 0.0f;
				}
			}

			if (closestDistance < 0.0f)
			{
				mTerrainComponent = nullptr;
			}

			if (nullptr != mTerrainComponent)
			{
				mTerrainEditHelper = QSF_EDITOR_TERRAINEDIT.findTerrainEditHelper(mTerrainComponent->getEntityId());
				if (nullptr == mTerrainEditHelper || !mTerrainEditHelper->isGood() || !mTerrainEditHelper->isReady())
				{
					mTerrainComponent = nullptr;
				}
			}
			else
			{
				// Do not invalidate the terrain edit helper instance in here, we might be in the middle of feeding an operation
				// mTerrainEditHelper = nullptr;
			}

			return (nullptr != mTerrainComponent);
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::EditMode methods       ]
		//[-------------------------------------------------------]
		void TerrainEditMode::onShutdown(EditMode* nextEditMode)
		{
			EditMode::onShutdown(nextEditMode);

			mTerrainComponent = nullptr;
			mTerrainEditHelper = nullptr;
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // qsf
