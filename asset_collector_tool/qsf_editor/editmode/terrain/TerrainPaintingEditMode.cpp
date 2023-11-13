// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "qsf_editor/PrecompiledHeader.h"
#include "qsf_editor/editmode/terrain/TerrainPaintingEditMode.h"
#include "qsf_editor/operation/terrain/TerrainOperation.h"
#include "qsf_editor/asset/terrain/TerrainEditHelper.h"
#include "qsf_editor/EditorHelper.h"

#include <qsf_editor_base/operation/CompoundOperation.h>

#include <qsf/QsfHelper.h>
#include <qsf/math/Math.h>
#include <qsf/renderer/terrain/TerrainComponent.h>
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
		const uint32 TerrainPaintingEditMode::PLUGINABLE_ID = StringHash("qsf::editor::TerrainPaintingEditMode");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		TerrainPaintingEditMode::TerrainPaintingEditMode(EditModeManager* editModeManager) :
			TerrainEditMode(editModeManager),
			mPaintMode(COLOR_PAINT),
			mBrushColor(Color3::WHITE)
		{
			// Nothing to do in here
		}

		TerrainPaintingEditMode::~TerrainPaintingEditMode()
		{
			// Nothing to do in here
		}

		TerrainPaintingEditMode::PaintMode TerrainPaintingEditMode::getPaintMode() const
		{
			return mPaintMode;
		}

		void TerrainPaintingEditMode::setPaintMode(PaintMode paintMode)
		{
			mPaintMode = paintMode;
		}

		const Color3& TerrainPaintingEditMode::getBrushColor() const
		{
			return mBrushColor;
		}

		void TerrainPaintingEditMode::setBrushColor(const Color3& brushColor)
		{
			mBrushColor = brushColor;
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::EditMode methods       ]
		//[-------------------------------------------------------]
		bool TerrainPaintingEditMode::onBrushingStartup()
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

		void TerrainPaintingEditMode::onBrushing()
		{
			// Only brush on it, when the position is valid
			if (nullptr != getTerrainComponent())
			{
				// Get the used terrain component instance
				switch (mPaintMode)
				{
					case COLOR_PAINT:
						if (!mSecondFunction)
						{
							editColorPainting();
						}
						break;

					case CAVE_PAINT:
						editCavePainting();
						break;
				}
			}
		}

		void TerrainPaintingEditMode::onBrushingShutdown()
		{
			TerrainEditHelper& terrainEditHelper = getTerrainEditHelper();

			// Only brush on it, when the position is valid
			TerrainDataMap* terrainDataMap = terrainEditHelper.getTerrainDataMap(DATATYPE_COLORMAP, false);
			QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", return);
			const std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

			// Commit the operation which takes control over the values buffer
			std::vector<TerrainDataMapValue>* undoValues = new std::vector<TerrainDataMapValue>();
			std::transform(workspaceValueMap.begin(), workspaceValueMap.end(),
				std::back_inserter(*undoValues),
				boost::bind(&std::map<uint64, TerrainDataMapValue>::value_type::second, _1));
			TerrainOperation* terrainOperation = new TerrainOperation(terrainEditHelper.getEntityId(), DATATYPE_COLORMAP, *undoValues);

			// Commit the operation
			QSF_EDITOR_OPERATION.push(terrainOperation);

			// Clear workspace
			terrainEditHelper.clearWorkspace();
		}


		//[-------------------------------------------------------]
		//[ Private virtual qsf::editor::BrushEditMode methods    ]
		//[-------------------------------------------------------]
		void TerrainPaintingEditMode::buildBrushShape()
		{
			// Start debug draw request
			DebugDrawManager& debugDrawManager = QSF_DEBUGDRAW;
			const float size = getBrushSize();
			const float arrowBase = getBrushIntensity() * 0.5f;
			const float arrowWidth = 2;
			const float arrowDirection = mSecondFunction ? -1.0f : 1.0f;

			if (mPaintMode == COLOR_PAINT)
			{
				if (BrushData::SHAPE_SQUARE == getBrushType())
				{
					// Square shape
					mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(RectangleDebugDrawRequest(glm::vec3(-size, 0.0f, -size), glm::vec3(size, 0.0f, size), Color4(mBrushColor.r, mBrushColor.g, mBrushColor.b, 0.35f), 1.0f)));
					mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(RectangleDebugDrawRequest(glm::vec3(-size, 0.0f, -size), glm::vec3(size, 0.0f, size), Color4(mBrushColor.r, mBrushColor.g, mBrushColor.b, 0.0f), 0.0f)));
				}
				else
				{
					// Circle shape
					mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, size, Color4(mBrushColor.r, mBrushColor.g, mBrushColor.b, 0.35f), true)));
					mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, size, Color4(mBrushColor.r, mBrushColor.g, mBrushColor.b, 0.8f), false)));
				}
			}
			else
			{
				const float alpha = mSecondFunction ? 0.5f : 0.15f;

				if (BrushData::SHAPE_SQUARE == getBrushType())
				{
					// Square shape
					mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(RectangleDebugDrawRequest(glm::vec3(-size, 0.0f, -size), glm::vec3(size, 0.0f, size), Color4(1.0f, 1.0f, 1.0f, alpha), 1.0f)));
					mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(RectangleDebugDrawRequest(glm::vec3(-size, 0.0f, -size), glm::vec3(size, 0.0f, size), Color4(1.0f, 1.0f, 1.0f, 0.0f), 0.0f)));
					if (!mSecondFunction)
					{
						mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(RectangleDebugDrawRequest(glm::vec3(-size, 0.0f, -size), glm::vec3(size * 0.95f, 0.0f, size * 0.95f), Color4(1.0f, 1.0f, 1.0f, 0.0f), 0.0f)));
					}
				}
				else
				{
					// Circle shape
					mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, size, Color4(1.0f, 1.0f, 1.0f, alpha), true)));
					mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, size, Color4(1.0f, 1.0f, 1.0f, 0.8f), false)));
					if (!mSecondFunction)
					{
						mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(CircleDebugDrawRequest(Math::GLM_VEC3_ZERO, Math::GLM_VEC3_UNIT_Y, size * 0.95f, Color4(1.0f, 1.0f, 1.0f, 0.8f), false)));
					}
				}
			}

			mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(SegmentDebugDrawRequest(Segment::fromTwoPoints(Math::GLM_VEC3_ZERO, glm::vec3(0.0f, arrowBase * arrowDirection, 0.0f)), Color4::WHITE)));
			mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(TriangleDebugDrawRequest(glm::vec3(-arrowWidth, arrowBase * arrowDirection, 0.0f), glm::vec3(arrowWidth, arrowBase * arrowDirection, 0.0f), glm::vec3(0.0f, (arrowBase + 0.2f * arrowBase) * arrowDirection, 0.0f), Color4(1.0f, 1.0f, 1.0f, 0.5f), 0.5f)));
			mDebugDrawRequestIds.push_back(debugDrawManager.requestDraw(TriangleDebugDrawRequest(glm::vec3(0.0f, arrowBase * arrowDirection, -arrowWidth), glm::vec3(0.0f, arrowBase * arrowDirection, arrowWidth), glm::vec3(0.0f, (arrowBase + 0.2f * arrowBase) * arrowDirection, 0.0f), Color4(1.0f, 1.0f, 1.0f, 0.5f), 0.5f)));
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void TerrainPaintingEditMode::editColorPainting()
		{
			std::vector<TerrainComponent*> terrainComponentsInfluencedByBrush;
			getTerrainComponentsInfluencedByBrush(terrainComponentsInfluencedByBrush);

			for (const TerrainComponent* terrainComponent : terrainComponentsInfluencedByBrush)
			{
				QSF_CHECK(nullptr != getTerrainComponent() && terrainComponent->getEntityId() == getTerrainComponent()->getEntityId(), "QSF invalid brush action", continue);
				TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_COLORMAP);
				QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", continue);
				std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

				std::vector<TerrainDataMapValue> previewValues;
				getValueList(*terrainComponent, terrainDataMap->getWidth(), previewValues);

				for (TerrainDataMapValue& terrainDataMapValue : previewValues)
				{
					const float d = terrainDataMapValue.value * 0.01f;

					terrainDataMapValue.channel  = 0;
					terrainDataMapValue.valueOld = terrainDataMap->getValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel);
					terrainDataMapValue.value	 = glm::mix(terrainDataMapValue.valueOld, mBrushColor.r, d);
					terrainDataMap->setValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel, terrainDataMapValue.value);
					addTerrainValue(workspaceValueMap, terrainDataMapValue);

					terrainDataMapValue.channel  = 1;
					terrainDataMapValue.valueOld = terrainDataMap->getValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel);
					terrainDataMapValue.value	 = glm::mix(terrainDataMapValue.valueOld, mBrushColor.g, d);
					terrainDataMap->setValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel, terrainDataMapValue.value);
					addTerrainValue(workspaceValueMap, terrainDataMapValue);

					terrainDataMapValue.channel  = 2;
					terrainDataMapValue.valueOld = terrainDataMap->getValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel);
					terrainDataMapValue.value	 = glm::mix(terrainDataMapValue.valueOld, mBrushColor.b, d);
					terrainDataMap->setValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel, terrainDataMapValue.value);
					addTerrainValue(workspaceValueMap, terrainDataMapValue);
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

				getTerrainEditHelper().updateOgreMap(DATATYPE_COLORMAP);
			}
		}

		void TerrainPaintingEditMode::editCavePainting()
		{
			std::vector<TerrainComponent*> terrainComponentsInfluencedByBrush;
			getTerrainComponentsInfluencedByBrush(terrainComponentsInfluencedByBrush);

			const float caveValue = mSecondFunction ? 1.0f : 0.0f;

			for (TerrainComponent* terrainComponent : terrainComponentsInfluencedByBrush)
			{
				QSF_CHECK(nullptr != getTerrainComponent() && terrainComponent->getEntityId() == getTerrainComponent()->getEntityId(), "QSF invalid brush action", continue);
				TerrainDataMap* terrainDataMap = getTerrainEditHelper().getTerrainDataMap(DATATYPE_COLORMAP);
				QSF_CHECK(nullptr != terrainDataMap, "There's no terrain data map instance", continue);
				std::map<uint64, TerrainDataMapValue>& workspaceValueMap = terrainDataMap->getWorkspaceValueMap();

				std::vector<TerrainDataMapValue> previewValues;
				getValueList(*terrainComponent, terrainDataMap->getWidth(), previewValues);

				for (TerrainDataMapValue& terrainDataMapValue : previewValues)
				{
					const float d = terrainDataMapValue.value * 0.01f;

					terrainDataMapValue.channel  = 3;
					terrainDataMapValue.valueOld = terrainDataMap->getValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel);
					terrainDataMapValue.value	 = glm::mix(terrainDataMapValue.valueOld, caveValue, d);
					terrainDataMap->setValueAtPosition(terrainDataMapValue.x, terrainDataMapValue.y, terrainDataMapValue.channel, terrainDataMapValue.value);
					addTerrainValue(workspaceValueMap, terrainDataMapValue);
				}

				getTerrainEditHelper().updateOgreMap(DATATYPE_COLORMAP);
			}
		}


//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // qsf
