// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/OrderInfoPictureCreator.h"
#include "ui_OrderInfoPictureCreator.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)


#include <qsf_editor/EditorHelper.h>
#include <qsf/QsfHelper.h>
#include <qsf/log/LogSystem.h>

//Assets 
#include <qsf/asset/helper/AssetDependencyCollector.h>
#include <qsf/asset/AssetProxy.h>
#include <qsf/asset/project/AssetPackage.h>
#include <qsf/asset/project/Project.h>
#include <qsf_editor/asset/AssetEditHelper.h>
#include <qsf_editor/asset/import/AssetImportManager.h>

//access mods
#include <em5/modding/ModSystem.h>
#include <em5\EM5Helper.h>
#include <em5/plugin/Plugin.h>

//open url
#include <qsf\platform\PlatformSystem.h>

#include <qsf\message\MessageSystem.h>
#include <qsf/log/LogSystem.h>


#include <qsf/prototype/PrototypeSystem.h>
#include <qsf/prototype/PrototypeManager.h>
#include <qsf/prototype/PrototypeHelper.h>
#include "qsf_editor/selection/layer/LayerSelectionManager.h"
#include <qsf/asset/Asset.h>
#include <qsf/prototype/helper/PrefabContent.h>
#include <qsf/prototype/BasePrototypeManager.h>

#include <qsf/renderer/mesh/MeshComponent.h>
#include <qsf/renderer/debug/DebugBoxComponent.h>
#include <qsf/prototype/helper/PrefabContent.h>

//ops

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

#include <qsf/map/map.h>
#include <qsf_editor/map/MapHelper.h>
#include <qsf/map/EntityHelper.h>
#include <qsf/prototype/Prototype.h>
#include <qsf\map\Entity.h>
#include <qsf\component\base\MetadataComponent.h>
#include <qsf\file\FileStream.h>
#include <qsf\file\FileHelper.h>

#include <qsf/asset/AssetSystem.h>
#include <qsf/asset/type/AssetType.h>
#include < qsf/asset/type/AssetTypeManager.h>
#include <boost\foreach.hpp>
#include <algorithm>
#include <QtWidgets\qmenu.h>
#include <QtGui\qclipboard.h>
#include <QtWidgets\qfiledialog.h>
#include <QtCore\qstandardpaths.h>
#include <fstream>

#include <ogre\Ogre.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <assert.h>
#include <time.h>

#include <filesystem>
#include <iostream>     // std::cout, std::endl
#include <iomanip>      // std::setfill, std::setw

#include <asset_collector_tool\extern\include\Magick++.h>
#include <string>
#include <iostream>
#include <list>
#include <algorithm>
#include <qsf/plugin/PluginSystem.h>

#include <qsf/renderer/RendererSystem.h>
#include <qsf_compositing/compositor/ScreenshotManager.h>
#include <qsf_editor/application/Application.h>
#include <qsf_editor/application/manager/CameraManager.h>
#include <qsf_compositing/compositor/CompositorSystem.h>
#include <qsf_compositing/plugin/Plugin.h>
#include <qsf/window/Window.h>
#include <qsf/window/WindowSystem.h>
#include <qsf/renderer/component/CameraComponent.h>
#include <qsf/renderer/window/RenderWindow.h>
#include <em5\component\vehicle\RoadVehicleComponent.h>
#include <qsf/math/EulerAngles.h>
#include <qsf/component/base/TransformComponent.h>
#include <qsf/map/query/ComponentMapQuery.h>
#include <OGRE/OgreRay.h>
#include <qsf/math/Convert.h>
#include <qsf/map/query/RayMapQuery.h>
#include <qsf_editor/renderer/RenderView.h>
#include <em5/map/EntityHelper.h>
#include <qsf_editor/base/GuiHelper.h>
#include <QtGui\qdesktopservices.h>
#include "qsf/renderer/material/MaterialSystem.h"
#include <qsf/renderer/material/material/MaterialVariationManager.h>
using namespace std;

using namespace Magick;


#undef ERROR
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
namespace user
{
	namespace editor
	{

		using boost::lexical_cast;
		using boost::bad_lexical_cast;
		//[-------------------------------------------------------]
		//[ Public definitions                                    ]
		//[-------------------------------------------------------]
		const uint32 OrderInfoPictureCreator::PLUGINABLE_ID = qsf::StringHash("user::editor::OrderInfoPictureCreator");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		OrderInfoPictureCreator::OrderInfoPictureCreator(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
			qsf::editor::View(viewManager, qWidgetParent),
			//View(viewManager, qWidgetParent),
			mUiOrderInfoPictureCreator(nullptr),
			mSavepath(""),
			cam(0),
			EntName("none")
		{
			// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
			addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);

		}

		OrderInfoPictureCreator::~OrderInfoPictureCreator()
		{

			// Destroy the UI view instance
			if (nullptr != mUiOrderInfoPictureCreator)
			{
				delete mUiOrderInfoPictureCreator;
			}
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		void OrderInfoPictureCreator::retranslateUi()
		{
			// Retranslate the content of the UI, this automatically clears the previous content
			mUiOrderInfoPictureCreator->retranslateUi(this);
		}

		void OrderInfoPictureCreator::changeVisibility(bool visible)
		{
			// Lazy evaluation: If the view is shown the first time, create its content
			if (visible && nullptr == mUiOrderInfoPictureCreator)
			{
				// Setup the view content
				QWidget* contentWidget = new QWidget(this);
				{

					// Load content to widget
					mUiOrderInfoPictureCreator = new Ui::OrderInfoPictureCreator();
					mUiOrderInfoPictureCreator->setupUi(contentWidget);
				}

				// Set content to view
				setWidget(contentWidget);
				// Connect Qt signals/slots
				connect(mUiOrderInfoPictureCreator->searchbutton, SIGNAL(clicked(bool)), this, SLOT(OnPushLoadMaterial_or_texture(bool)));
				connect(mUiOrderInfoPictureCreator->decompilebutton, SIGNAL(clicked(bool)), this, SLOT(onPushDecompileButton(bool)));
				connect(mUiOrderInfoPictureCreator->SetSaveLocationButton, SIGNAL(clicked(bool)), this, SLOT(onSetSaveDirectory(bool)));
				connect(mUiOrderInfoPictureCreator->SetSelectionGreenscreen, SIGNAL(clicked(bool)), this, SLOT(onSetSelectionGreenscreen(bool)));

				connect(mUiOrderInfoPictureCreator->cam1, SIGNAL(clicked(bool)), this, SLOT(onCam1(bool)));
				connect(mUiOrderInfoPictureCreator->cam2, SIGNAL(clicked(bool)), this, SLOT(onCam2(bool)));
				connect(mUiOrderInfoPictureCreator->origcam, SIGNAL(clicked(bool)), this, SLOT(onOrigCam(bool)));
				connect(mUiOrderInfoPictureCreator->removeEntity, SIGNAL(clicked(bool)), this, SLOT(onremoveEntity(bool)));
				connect(mUiOrderInfoPictureCreator->OpenSaveLocation, SIGNAL(clicked(bool)), this, SLOT(onOpenSaveLocation(bool)));
				InitSavePath();
			}
			else if (!visible && nullptr == mUiOrderInfoPictureCreator)
			{
			}

		}

		void OrderInfoPictureCreator::showEvent(QShowEvent * qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);

			// Perform a GUI rebuild to ensure the GUI content is up-to-date
			rebuildGui();
		}

		void OrderInfoPictureCreator::hideEvent(QHideEvent * qHideEvent)
		{
			// Call the base implementation
			View::hideEvent(qHideEvent);
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void OrderInfoPictureCreator::rebuildGui()
		{
		}

		void OrderInfoPictureCreator::onPushDecompileButton(const bool pressed)
		{
			if (!mUiOrderInfoPictureCreator->Raytracing->isChecked())
			{
				DoWithOutRaytracing();
				return;
			}
			CheckForRaysHitting();

			auto win = QSF_EDITOR_APPLICATION.getCameraManager().getCameraComponent()->getRenderWindows();
			for (auto a : win)
			{
				if (a == nullptr)
					return;
				auto RenderWindow = a->getOgreRenderWindow();
				if (RenderWindow == nullptr)
					return;
				RenderWindow->suggestPixelFormat();
				const auto memory_category = Ogre::MemoryCategory::MEMCATEGORY_RENDERSYS;
				auto const pixel_format = RenderWindow->suggestPixelFormat();

				auto const width = RenderWindow->getWidth();
				auto const height = RenderWindow->getHeight();
				auto const bytesPerPixel = Ogre::PixelUtil::getNumElemBytes(pixel_format);
				auto const byteCount = width * height * bytesPerPixel;
				auto* data = OGRE_ALLOC_T(std::uint8_t, byteCount, memory_category);

				Ogre::PixelBox pixelBox(width, height, 1, pixel_format, data);
				RenderWindow->copyContentsToMemory(pixelBox, Ogre::RenderTarget::FB_AUTO);
				Ogre::Image ColorMap;
				
				ColorMap.loadDynamicImage(static_cast<unsigned char*>(pixelBox.data), width, height, Ogre::PixelFormat::PF_BYTE_RGB);
				std::string Path = GetSavePath() + EntName + boost::lexical_cast<std::string>(cam) + ".png";
				std::string Path_Mini = GetSavePath() + EntName + boost::lexical_cast<std::string>(cam) + "_mini.png";
				std::string widthandheight = boost::lexical_cast<std::string>(width) + "x" + boost::lexical_cast<std::string>(height);
				Image IOL;
				IOL.size(widthandheight);
				IOL.magick("PNG");
				IOL.type(Magick::ImageType::TrueColorAlphaType);

				MagickCore::Quantum *pixels = IOL.getPixels(0, 0, width, height);
				if (IOL.channels() == 4) //rgb
				{
					for (size_t x = 0; x < width; x++)
					{
						for (size_t y = 0; y < height; y++)
						{
							auto OldColor = ColorMap.getColourAt(x, y, 0);
							uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
							*(pixels + offset) = 65535.f*OldColor.r;
							*(pixels + offset + 1) = 65535.f*OldColor.g;
							*(pixels + offset + 2) = 65535.f*OldColor.b;
						}
					}
				}
				//make them transparent
				if (IOL.channels() == 4) //rgb
				{

					for (auto a : HitByRay)
					{
						if (a.z == 0)
						{
							uint64 offset = (width* a.y + a.x) * 4; //4 is because we use 4 channels
							(*(pixels + offset + 3) = 0);
						}
					}

				}
				//crop image
				int CropTop = 0;
				bool Done = false;
				for (size_t y = 0; y < height; y++)
				{
				for (size_t x = 0; x < width; x++)
				{

						uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
						if (*(pixels + offset + 3) != 0)
						{
							Done = true;
							break;
						}
					}
					if (!Done)
					{
						CropTop++;
					}
					else
						break;

				}

				int CropBottom = 0;
				Done = false;
				for (size_t y = height - 1; y > 0; y--)
				{
				for (size_t x = 0; x < width; x++)
				{

						uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
						if (*(pixels + offset + 3) != 0)
						{
							Done = true;
							break;
						}
					}
					if (!Done)
					{
						CropBottom++;
					}
					else
						break;

				}

				int CropLeft = 0;
				Done = false;


						for (size_t x = 0; x < width; x++)
						{
							for (size_t y = 0; y < height; y++)
							{
						uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
						if (*(pixels + offset + 3) != 0)
						{
							Done = true;
							break;
						}
					}
					if (!Done)
					{
						CropLeft++;
					}
					else
						break;

				}

					int CropRight = 0;
					Done = false;

					for (size_t x = width-1; 0 < x; x--)
					{
						for (size_t y = 0; y < height; y++)
						{
							uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
							if (*(pixels + offset + 3) != 0)
							{
								Done = true;
								break;
							}
						}
						if (!Done)
						{
							CropRight++;
						}
						else
							break;

					}

					// Crop the image to specified size (width, height, xOffset, yOffset)
					//image.crop(Geometry(100, 100, 100, 100));
				IOL.syncPixels();
				QSF_LOG_PRINTS(INFO,"crop left "<< CropLeft << " crop right "<< CropRight << " crop top "<< CropTop << " crop bottom "<< CropBottom)
				IOL.crop(Geometry(width- CropLeft -CropRight,height-CropTop-CropBottom,CropLeft,CropTop));
				try
				{
					IOL.write(Path.c_str());
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO,e.what())
					return;
				}
				
				auto newwidth = width - CropLeft - CropRight;
				auto newheight = height - CropTop - CropBottom;
				//our target is width of 103 or hight of 60
				newheight = newheight *(float)( 103.f/(float)newwidth);
				IOL.scale(Geometry(newwidth,newheight,0,0));
				try
				{
					IOL.write(Path_Mini);
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO, e.what())
						return;
				}
				//ColorMap.save(Path.c_str());
				auto pm = QPixmap(Path.c_str());
				mUiOrderInfoPictureCreator->Materialinfos->setPixmap(pm);
				//mUiOrderInfoPictureCreator->Materialinfos->setScaledContents(true);
				//we are in a for loop
				return;
			}
		}
		void OrderInfoPictureCreator::DoWithOutRaytracing()
		{
			auto QSF_renderWindow = &QSF_EDITOR_APPLICATION.getMainWindow()->getRenderView().getRenderWindow();
			if (QSF_renderWindow == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no render window")
					return;
			}
				auto RenderWindow = QSF_renderWindow->getOgreRenderWindow();
				if (RenderWindow == nullptr)
					return;
				RenderWindow->suggestPixelFormat();
				const auto memory_category = Ogre::MemoryCategory::MEMCATEGORY_RENDERSYS;
				auto const pixel_format = RenderWindow->suggestPixelFormat();

				auto const width = RenderWindow->getWidth();
				auto const height = RenderWindow->getHeight();
				auto const bytesPerPixel = Ogre::PixelUtil::getNumElemBytes(pixel_format);
				auto const byteCount = width * height * bytesPerPixel;
				auto* data = OGRE_ALLOC_T(std::uint8_t, byteCount, memory_category);
				auto GS = GetGreenscreen();
				if (GS == nullptr)
					return;
				for (auto childs : GS->getComponent<qsf::LinkComponent>()->getChildLinks())
				{
					//if (childs->getEntity().getComponent<qsf::MetadataComponent>()->getName().find("greenscreen") != std::string::npos)
						//QSF_MATERIAL.getMaterialVariationManager().setEntityMaterialPropertyValue(childs->getEntity(), "DiffuseColor", qsf::MaterialPropertyValue::fromFloat4(0.f, 0.f, 0.f, 0.f), 106948281);


				}
				QSF_RENDERER.renderFrame(*QSF_renderWindow, *RenderWindow, false);
				Ogre::PixelBox pixelBox(width, height, 1, pixel_format, data);
				RenderWindow->copyContentsToMemory(pixelBox, Ogre::RenderTarget::FB_AUTO);
				Ogre::Image ColorMap;

				ColorMap.loadDynamicImage(static_cast<unsigned char*>(pixelBox.data), width, height, Ogre::PixelFormat::PF_BYTE_RGB);
				std::string Path = GetSavePath() + EntName + boost::lexical_cast<std::string>(cam) + ".png";

				QSF_RENDERER.renderFrame(*QSF_renderWindow, *RenderWindow, false);
				for(auto childs : GS->getComponent<qsf::LinkComponent>()->getChildLinks())
				{
					if(childs->getEntity().getComponent<qsf::MetadataComponent>()->getName().find("greenscreen") != std::string::npos)
					//QSF_MATERIAL.getMaterialVariationManager().setEntityMaterialPropertyValue(childs->getEntity(), "DiffuseColor", qsf::MaterialPropertyValue::fromFloat4(0.f, 255.f, 0.f,0.f), 106948281);
					QSF_MATERIAL.getMaterialVariationManager().setEntityMaterialPropertyValue(childs->getEntity(), "UseEmissiveMap", qsf::MaterialPropertyValue::fromBoolean(true), 106948281);
				}
				QSF_RENDERER.renderFrame(*QSF_renderWindow, *RenderWindow, false);
				auto* data_green = OGRE_ALLOC_T(std::uint8_t, byteCount, memory_category);
				Ogre::PixelBox pixelBox_green(width, height, 1, pixel_format, data_green);
				RenderWindow->copyContentsToMemory(pixelBox_green, Ogre::RenderTarget::FB_AUTO);
				Ogre::Image ColorMap_green;
				ColorMap_green.loadDynamicImage(static_cast<unsigned char*>(pixelBox_green.data), width, height, Ogre::PixelFormat::PF_BYTE_RGB);
				QSF_RENDERER.renderFrame(*QSF_renderWindow, *RenderWindow, false);
				//ColorMap.save(Path);
				//ColorMap_green.save(Path2);
				for (auto childs : GS->getComponent<qsf::LinkComponent>()->getChildLinks())
				{
					if (childs->getEntity().getComponent<qsf::MetadataComponent>()->getName().find("greenscreen") != std::string::npos)
					QSF_MATERIAL.getMaterialVariationManager().resetEntityMaterialPropertyValues(childs->getEntity(),true);
					//QSF_MATERIAL.getMaterialVariationManager().setEntityMaterialPropertyValue(getLightMaterialHolder(), "UseEmissiveMap", qsf::MaterialPropertyValue::fromBoolean(false), 106948281);

				}
				std::string Path_Mini = GetSavePath() + EntName + boost::lexical_cast<std::string>(cam) + "_mini.png";
				std::string widthandheight = boost::lexical_cast<std::string>(width) + "x" + boost::lexical_cast<std::string>(height);
				Image IOL;
				IOL.size(widthandheight);
				IOL.magick("PNG");
				IOL.type(Magick::ImageType::TrueColorAlphaType);

				MagickCore::Quantum *pixels = IOL.getPixels(0, 0, width, height);
				if (IOL.channels() == 4) //rgb
				{
					for (size_t x = 0; x < width; x++)
					{
						for (size_t y = 0; y < height; y++)
						{
							uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
							auto OldColor = ColorMap.getColourAt(x, y, 0);
							auto OldColorGreen = ColorMap_green.getColourAt(x, y, 0);
							if (glm::abs(OldColor.g - OldColorGreen.g) > 0.25) //Alpha
							{
								*(pixels + offset + 3) = 0;
							}
							else
							{
								*(pixels + offset) = 65535.f*OldColor.r;
								*(pixels + offset + 1) = 65535.f*OldColor.g;
								*(pixels + offset + 2) = 65535.f*OldColor.b;
							}

						}
					}
				}
				//crop image
				int CropTop = 0;
				bool Done = false;
				for (size_t y = 0; y < height; y++)
				{
					for (size_t x = 0; x < width; x++)
					{

						uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
						if (*(pixels + offset + 3) != 0)
						{
							Done = true;
							break;
						}
					}
					if (!Done)
					{
						CropTop++;
					}
					else
						break;

				}

				int CropBottom = 0;
				Done = false;
				for (size_t y = height - 1; y > 0; y--)
				{
					for (size_t x = 0; x < width; x++)
					{

						uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
						if (*(pixels + offset + 3) != 0)
						{
							Done = true;
							break;
						}
					}
					if (!Done)
					{
						CropBottom++;
					}
					else
						break;

				}

				int CropLeft = 0;
				Done = false;


				for (size_t x = 0; x < width; x++)
				{
					for (size_t y = 0; y < height; y++)
					{
						uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
						if (*(pixels + offset + 3) != 0)
						{
							Done = true;
							break;
						}
					}
					if (!Done)
					{
						CropLeft++;
					}
					else
						break;

				}

				int CropRight = 0;
				Done = false;

				for (size_t x = width - 1; 0 < x; x--)
				{
					for (size_t y = 0; y < height; y++)
					{
						uint64 offset = (width* y + x) * 4; //4 is because we use 4 channels
						if (*(pixels + offset + 3) != 0)
						{
							Done = true;
							break;
						}
					}
					if (!Done)
					{
						CropRight++;
					}
					else
						break;

				}

				// Crop the image to specified size (width, height, xOffset, yOffset)
				//image.crop(Geometry(100, 100, 100, 100));
				IOL.syncPixels();
				QSF_LOG_PRINTS(INFO, "crop left " << CropLeft << " crop right " << CropRight << " crop top " << CropTop << " crop bottom " << CropBottom)
					IOL.crop(Geometry(width - CropLeft - CropRight, height - CropTop - CropBottom, CropLeft, CropTop));

				try
				{
					IOL.write(Path.c_str());
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO, e.what())
						return;
				}
				auto newwidth = width - CropLeft - CropRight;
				auto newheight = height - CropTop - CropBottom;
				//our target is width of 103 or hight of 60
				newheight = newheight *(float)(103.f / (float)newwidth);
				IOL.scale(Geometry(newwidth, newheight, 0, 0));
				try
				{
					IOL.write(Path_Mini.c_str());
				}
				catch (const std::exception& e)
				{
					QSF_LOG_PRINTS(INFO, e.what())
						return;
				}
				//mUiOrderInfoPictureCreator->Materialinfos->setScaledContents(true);
				//we are in a for loop
				auto pm = QPixmap(Path.c_str());
				mUiOrderInfoPictureCreator->Materialinfos->setPixmap(pm);
				return;
		}

		void OrderInfoPictureCreator::onSetSelectionGreenscreen(const bool pressed)
		{
			qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
			if (entitySelectionManager.getSelectionSize() != 1)
				return;
			std::string Greenscreen = boost::lexical_cast<std::string>(entitySelectionManager.getSelectedId());
			mUiOrderInfoPictureCreator->label->setText(Greenscreen.c_str());

		}

		void OrderInfoPictureCreator::onCam1(const bool pressed)
		{

			qsf::Entity* GS = GetGreenscreen();
			if (GS == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Pls set a greenscreen first")
			}
			qsf::Entity* cam2 = nullptr;
			for (auto a : GS->getOrCreateComponent<qsf::LinkComponent>()->getChildLinks())
			{
				auto MC = a->getEntity().getComponent<qsf::MetadataComponent>();
				if (MC == nullptr || MC->getName().find("cam1") == std::string::npos)
					continue;
				cam2 = &a->getEntity();
				break;
			}
			if (cam2 == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Greenscrenn has no entity called \"cam1\" attached to it")
					return;
			}
			auto CC = cam2->getComponent<qsf::CameraComponent>();
			if (CC == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Greenscrenn->\"cam1\" has no camera component")
					return;
			}
			QSF_EDITOR_APPLICATION.getCameraManager().setCameraComponent(CC);
			cam = 1;
		}

		void OrderInfoPictureCreator::onCam2(const bool pressed)
		{
			
			qsf::Entity* GS = GetGreenscreen();
			if (GS == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Pls set a greenscreen first")
			}
			qsf::Entity* cam2 = nullptr;
			for (auto a : GS->getOrCreateComponent<qsf::LinkComponent>()->getChildLinks())
			{
				auto MC = a->getEntity().getComponent<qsf::MetadataComponent>();
				if (MC == nullptr || MC->getName().find("cam2") == std::string::npos)
					continue;
				cam2 = &a->getEntity();
				break;
			}
			if (cam2 == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Greenscrenn has no entity called \"cam2\" attached to it")
					return;
			}
			auto CC = cam2->getComponent<qsf::CameraComponent>();
			if (CC == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Greenscrenn->\"cam2\" has no camera component")
					return;
			}
			QSF_EDITOR_APPLICATION.getCameraManager().setCameraComponent(CC);
			cam = 2;
		}

		void OrderInfoPictureCreator::onOrigCam(const bool pressed)
		{
			QSF_EDITOR_APPLICATION.getCameraManager().setDefaultCamera();
			cam = 0;
		}

		void OrderInfoPictureCreator::OnPushLoadMaterial_or_texture(const bool pressed)
		{
			qsf::editor::EntitySelectionManager& entitySelectionManager = QSF_EDITOR_SELECTION_SYSTEM.getSafe<qsf::editor::EntitySelectionManager>();
			if (entitySelectionManager.getSelectionSize() != 1)
				return;
			//disable Shadows
			std::vector<qsf::Entity*> entList;
			qsf::Entity* Entity = QSF_MAINMAP.getEntityById(entitySelectionManager.getSelectedId());
			if (Entity == nullptr)
				return;
			auto LC = Entity->getOrCreateComponent<qsf::LinkComponent>();
			//QSF_LOG_PRINTS(INFO,"Scanned Child size "<<LC->getChildLinks().size())
			for (auto a : LC->getChildLinks())
			{
				if (a != nullptr && &a->getEntity() != nullptr)
					entList.push_back(&a->getEntity());
			}
			std::vector<qsf::Entity*> ChildList;

			ChildList.insert(ChildList.begin(), entList.begin(), entList.end());
			entList.clear();
			while (!ChildList.empty()) //iterate through all children
			{
				entList.push_back(ChildList.at(0)); //push it to other list
				qsf::Entity* ent = ChildList.at(0);
				if (ent == nullptr)
				{
					ChildList.erase(ChildList.begin());
					continue;
				}
				auto LC = ent->getOrCreateComponent<qsf::LinkComponent>();
				auto NewChildren = LC->getChildLinks();
				for (auto a : NewChildren)
				{
					ChildList.push_back(&a->getEntity());
				}
				ChildList.erase(ChildList.begin());
			}
			entList.push_back(Entity);
			for (auto a : entList)
			{
				if (a->getComponent<qsf::MeshComponent>() != nullptr)
				{
					a->getComponent<qsf::MeshComponent>()->setCastShadows(false);
				}
			}
			//disable shadows end
			//disable bluelight
			if (Entity->getComponent<em5::RoadVehicleComponent>())
			{
				Entity->getComponent<em5::RoadVehicleComponent>()->setBlueLightState(false);
			}
			//position object
			qsf::Entity* GS = GetGreenscreen();
			if (GS == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Pls set a greenscreen first")
				return;
			}
			if (Entity->getId() == GS->getId())
			{
				QSF_LOG_PRINTS(INFO, "no entity but the greenscreen was selected")
					return;
			}
			Entity->getOrCreateComponent<qsf::LinkComponent>()->setParentId(GS->getId());
			Entity->getOrCreateComponent<qsf::LinkComponent>()->setLocalPosition(glm::vec3(-24.411270141601563, 2.5086259841918945, -15.40203857421875));
			Entity->getOrCreateComponent<qsf::LinkComponent>()->setLocalRotation(qsf::EulerAngles::eulerToQuaternion(glm::vec3(glm::pi<float>(), 0.f, 0.f)));
			EntName = Entity->getOrCreateComponent<qsf::MetadataComponent>()->getName();

		}



		void OrderInfoPictureCreator::onSetSaveDirectory(const bool pressed)
		{
			QString Path = GetSavePath().c_str();
			QWidget* qtw = new QWidget();
			auto fileName = QFileDialog::getExistingDirectory(qtw,
				tr("Set Save Directory"), Path);
			if (fileName == "")
				return;
			QSF_LOG_PRINTS(INFO, fileName.toStdString() + "/")
				mSavepath = fileName.toStdString() + "//";

			std::ofstream ofs(path + "image_decompiler.txt", std::ofstream::trunc);

			ofs << mSavepath;

			ofs.close();

		}

		void OrderInfoPictureCreator::onremoveEntity(const bool pressed)
		{
			qsf::Entity* GS = GetGreenscreen();
			if (GS == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Pls set a greenscreen first")
			}

			qsf::Entity* remover = nullptr;
			for (auto a : GS->getOrCreateComponent<qsf::LinkComponent>()->getChildLinks())
			{
				auto MC = a->getEntity().getComponent<qsf::MetadataComponent>();
				if (MC == nullptr || MC->getName() != EntName)
					continue;
				remover = &a->getEntity();
				break;
			}
			if (remover == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "found no ent to remove")
					EntName = "";
				return;
			}
			remover->getComponent<qsf::LinkComponent>()->unlinkFromParent();
			auto TC = remover->getComponent<qsf::TransformComponent>();
			TC->setPosition(TC->getPosition() + glm::vec3(100.f, 0.f, 0.f));
			EntName = "";
		}

		void OrderInfoPictureCreator::onOpenSaveLocation(const bool pressed)
		{
			//qsf::editor::GuiHelper::openExplorer(GetSavePath().c_str());
			QDesktopServices::openUrl(QUrl::fromLocalFile(GetSavePath().c_str()));
		}

		qsf::Entity * OrderInfoPictureCreator::GetGreenscreen()
		{
			uint64 GreenScreenId = qsf::getUninitialized<uint64>();
			try
			{
				GreenScreenId = boost::lexical_cast<uint64>(mUiOrderInfoPictureCreator->label->text().toStdString());
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO, "Pls set a greenscreen first " << e.what())
					return nullptr;
			}
			return QSF_MAINMAP.getEntityById(GreenScreenId);
		}




		std::string OrderInfoPictureCreator::GetSavePath()
		{
			return mSavepath;
		}

		std::string OrderInfoPictureCreator::InitSavePath()
		{
			for (auto a : QSF_PLUGIN.getPlugins())
			{
				if (a->getFilename().find("asset_collector_tool.dll") != std::string::npos)
				{
					path = a->getFilename();
					path.erase(path.end() - 24, path.end());


				}
			}
			std::ifstream myfile(path + "image_decompiler.txt");
			std::string line;
			if (myfile.is_open())
			{
				while (std::getline(myfile, line))
				{
					mSavepath = line;
					QSF_LOG_PRINTS(INFO, "Savepath is" << mSavepath)
						break;

				}
				myfile.close();
			}
			else
			{
				std::ofstream myfile(path + "image_decompiler.txt");
				if (myfile.is_open())
				{
					myfile << path;
					myfile.close();
				}
			}
			return std::string();
		}
		void OrderInfoPictureCreator::CheckForRaysHitting()
		{
			if (QSF_EDITOR_APPLICATION.getMainWindow() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no main window")
					return;
			}
			if (&QSF_EDITOR_APPLICATION.getMainWindow()->getRenderView() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no render view")
					return;
			}
			auto renderWindow = &QSF_EDITOR_APPLICATION.getMainWindow()->getRenderView().getRenderWindow();
			if (renderWindow == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "no render window")
					return;
			}
			auto Height = renderWindow->getHeight();
			auto Width = renderWindow->getWidth();

			//now entity stuff
			uint64 GreenScreenId = qsf::getUninitialized<uint64>();
			try
			{
				GreenScreenId = boost::lexical_cast<uint64>(mUiOrderInfoPictureCreator->label->text().toStdString());
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO, "Pls set a greenscreen first " << e.what())
					return;
			}
			qsf::Entity* GS = QSF_MAINMAP.getEntityById(GreenScreenId);
			if (GS == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Pls set a greenscreen first")
			}
			qsf::Entity* TakePicEnt = nullptr;
			for (auto a : GS->getOrCreateComponent<qsf::LinkComponent>()->getChildLinks())
			{
				auto MC = a->getEntity().getComponent<qsf::MetadataComponent>();
				if (MC == nullptr || MC->getName() != EntName)
					continue;
				TakePicEnt = &a->getEntity();
				break;
			}
			if (TakePicEnt == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "No entity attached to take a photo")
					return;
			}
			//unlink so its easier to find if it is caught
			if(mUiOrderInfoPictureCreator->Raytracing->isChecked())
			{
			TakePicEnt->getComponent<qsf::LinkComponent>()->unlinkFromParent();
			HitByRay.clear();
			auto cameraComponent = renderWindow->getCameraComponent();
			if (nullptr != cameraComponent)
			{
				for (size_t x = 0; x < Width; x++)
				{
					for (size_t y = 0; y < Height; y++)
					{
						// Get the normalized mouse position
						bool hit = RayDidHit(glm::vec2(x, y), renderWindow, TakePicEnt);
						if (!hit)
							HitByRay.push_back(glm::vec3((int)x, (int)y, 0));
					}
				}
			}

			//relink
			TakePicEnt->getComponent<qsf::LinkComponent>()->setParentId(GS->getId());
			}
		}
		bool OrderInfoPictureCreator::RayDidHit(glm::vec2 Pos, qsf::RenderWindow* RW, qsf::Entity* Target)
		{

			const glm::vec2 normalizedPosition = RW->getNormalizedWindowSpaceCoords(Pos.x, Pos.y);
			qsf::RayMapQueryResponse rayMapQueryResponse(qsf::RayMapQueryResponse::POSITION_RESPONSE);
			qsf::RayMapQuery(QSF_MAINMAP).getFirstHitByRenderWindowNormalizedPosition(*RW, normalizedPosition.x, normalizedPosition.y, rayMapQueryResponse);
			if (rayMapQueryResponse.component == nullptr)
				return false;
			uint64 Id = rayMapQueryResponse.component->getEntityId();
			auto ent = &QSF_MAINMAP.getEntityById(Id)->getOrCreateComponent<qsf::LinkComponent>()->getTopmostAncestorLink(qsf::LinkComponent::SELECT_PARENT).getEntity();
			return (ent->getId() == Target->getId());
		}
		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
