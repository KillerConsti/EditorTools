// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/ImageDecompiler.h"
#include "ui_ImageDecompiler.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)


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
		const uint32 ImageDecompiler::PLUGINABLE_ID = qsf::StringHash("user::editor::ImageDecompiler");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		ImageDecompiler::ImageDecompiler(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
			KC_AbstractView(viewManager, qWidgetParent),
			//View(viewManager, qWidgetParent),
			mUiImageDecompiler(nullptr),
			mSavepath("")
		{
			// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
			addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);

		}

		ImageDecompiler::~ImageDecompiler()
		{

			// Destroy the UI view instance
			if (nullptr != mUiImageDecompiler)
			{
				delete mUiImageDecompiler;
			}
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		void ImageDecompiler::retranslateUi()
		{
			// Retranslate the content of the UI, this automatically clears the previous content
			mUiImageDecompiler->retranslateUi(this);
		}

		void ImageDecompiler::changeVisibility(bool visible)
		{
			// Lazy evaluation: If the view is shown the first time, create its content
			if (visible && nullptr == mUiImageDecompiler)
			{
				// Setup the view content
				QWidget* contentWidget = new QWidget(this);
				{

					// Load content to widget
					mUiImageDecompiler = new Ui::ImageDecompiler();
					mUiImageDecompiler->setupUi(contentWidget);
				}

				// Set content to view
				setWidget(contentWidget);
				// Connect Qt signals/slots
				connect(mUiImageDecompiler->searchbutton, SIGNAL(clicked(bool)), this, SLOT(OnPushLoadMaterial_or_texture(bool)));
				connect(mUiImageDecompiler->decompilebutton, SIGNAL(clicked(bool)), this, SLOT(onPushDecompileButton(bool)));
				connect(mUiImageDecompiler->SetSaveLocationButton, SIGNAL(clicked(bool)), this, SLOT(onSetSaveDirectory(bool)));
				InitSavePath();
			}
			else if (!visible && nullptr == mUiImageDecompiler)
			{
			}

		}

		void ImageDecompiler::showEvent(QShowEvent * qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);

			// Perform a GUI rebuild to ensure the GUI content is up-to-date
			rebuildGui();
		}

		void ImageDecompiler::hideEvent(QHideEvent * qHideEvent)
		{
			// Call the base implementation
			View::hideEvent(qHideEvent);
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void ImageDecompiler::rebuildGui()
		{
		}

		void ImageDecompiler::onPushDecompileButton(const bool pressed)
		{
			uint64 Id = qsf::getUninitialized<uint64>();
			auto Text = mUiImageDecompiler->line_edit_searchprototype->text().toStdString();
			try
			{
				Id = boost::lexical_cast<uint64>(Text);
			}
			catch (const std::exception& e)
			{
				mUiImageDecompiler->textBrowser->setText(e.what());
				QSF_LOG_PRINTS(ERROR,e.what())
				return;
			}
			auto proxy = qsf::AssetProxy(Id);
			if (proxy.getAsset() == nullptr)
			{
				QSF_LOG_PRINTS(INFO, "Global Asset id is not valid")
				mUiImageDecompiler->textBrowser->setText("Global Asset id is not valid... maybe it's not mounted");
					return;
			}
			if(proxy.getAsset()->getTypeName() == "texture")
			{
				std::string prefix = "test";
				std::string NewPrefix = mUiImageDecompiler->lineedit_prefix->text().toStdString();
				if(NewPrefix != "")
				prefix = NewPrefix;
				DecompileImage(proxy.getLocalAssetName(),prefix);
			}
			else if (proxy.getAsset()->getTypeName() == "material")
			{
				QSF_LOG_PRINTS(INFO,"parsing a material")
					auto a = qsf::AssetDependencyCollector(proxy.getGlobalAssetId());
				std::vector<uint64> CompleteList;
				a.collectAllDerivedAssets(CompleteList);
				std::string TextbrowserText ="Load Material...\n\n";
				for (auto b : CompleteList)
				{
					if(qsf::AssetProxy(b).getAsset() == nullptr)
					continue;
					if(qsf::AssetProxy(b).getAsset()->getTypeName() != "texture")
					continue;
					TextbrowserText+= qsf::AssetProxy(b).getLocalAssetName()+"\n";

					//prefix
					std::string prefix = proxy.getAsset()->getName();
					std::string NewPrefix = mUiImageDecompiler->lineedit_prefix->text().toStdString();
					if (NewPrefix != "")
						prefix = NewPrefix;

					bool success = DecompileImage(qsf::AssetProxy(b).getLocalAssetName(), prefix);
					if(success)
						TextbrowserText += "success\n";
					else
					{
						TextbrowserText += "failure\n";
					}

				}
				mUiImageDecompiler->textBrowser->setText(TextbrowserText.c_str());
			}
		}

		void ImageDecompiler::OnPushLoadMaterial_or_texture(const bool pressed)
		{
			uint64 Id = qsf::getUninitialized<uint64>();
			auto Text = mUiImageDecompiler->line_edit_searchprototype->text().toStdString();
			try
			{
				Id = boost::lexical_cast<uint64>(Text);
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(ERROR, e.what())
					return;
			}
			auto proxy = qsf::AssetProxy(Id);
			if (proxy.getAsset() == nullptr)
			{
				mUiImageDecompiler->textBrowser->setText("Global Asset id is not valid");
					return;
			}
			if (proxy.getAsset()->getTypeName() == "texture")
			{
				std::string prefix = "test";
				std::string NewPrefix = mUiImageDecompiler->lineedit_prefix->text().toStdString();
				if (NewPrefix != "")
					prefix = NewPrefix;
				mUiImageDecompiler->textBrowser->setText(proxy.getLocalAssetName().c_str());
			}
			else if (proxy.getAsset()->getTypeName() == "material")
			{
				QSF_LOG_PRINTS(INFO, "parsing a material")
					auto a = qsf::AssetDependencyCollector(proxy.getGlobalAssetId());
				std::vector<uint64> CompleteList;
				a.collectAllDerivedAssets(CompleteList);
				std::string TextbrowserText = "Load Material...\n\n";
				for (auto b : CompleteList)
				{
					if (qsf::AssetProxy(b).getAsset() == nullptr)
						continue;
					if (qsf::AssetProxy(b).getAsset()->getTypeName() != "texture")
						continue;
					TextbrowserText += qsf::AssetProxy(b).getLocalAssetName() + "\n";


				}
				mUiImageDecompiler->textBrowser->setText(TextbrowserText.c_str());
			}
		}



		bool ImageDecompiler::DecompileImage(std::string TextureName,std::string MaterialName)
		{
			QSF_LOG_PRINTS(INFO, "DecompileImage " << TextureName)
				//a what kind is it?
				int textureType = 0;
			if (TextureName.find("crgb_aa") != std::string::npos)
				textureType = 1;
			if (TextureName.find("nag_sr_gb") != std::string::npos)
				textureType = 2;
			if (TextureName.find("tr_og_hb") != std::string::npos)
				textureType = 3;
			if (TextureName.find("_ergb") != std::string::npos)
				textureType = 4;
			if (textureType == 0)
			{
				QSF_LOG_PRINTS(INFO, "unsupported texture, pls only insert compiled textures")
					return false;
			}
			//b is size 2^n?
			
				auto Filepath = qsf::AssetProxy(TextureName).getAbsoluteCachedAssetDataFilename();
				if (textureType == 1)
				{
					//image_magic
					Magick::Image image(Filepath);
					int w = (int)image.columns();
					int h = (int)image.rows();
					if (w <= 0 || h <= 0)
					{
						QSF_LOG_PRINTS(INFO, "broken image (size detection failed)" << Filepath)
					}
					if (image.channels() != 4 && image.channels() != 3)
					{
						QSF_LOG_PRINTS(INFO, "cant detect alpha channel of " << Filepath)
							return false;
					}
					MagickCore::Quantum *pixels = image.getPixels(0, 0, w, h);
					//now create a normal map img
					uint8* bufferColor = new uint8[w * h * 8];
					Ogre::Image ColorMap;
					ColorMap.loadDynamicImage(bufferColor, w, h, Ogre::PixelFormat::PF_FLOAT16_RGB);
					auto channels = image.channels();
					uint8* bufferAlpha = new uint8[w * h * 8]; //red
					Ogre::Image AlphaMap;
					AlphaMap.loadDynamicImage(bufferAlpha, w, h, Ogre::PixelFormat::PF_FLOAT32_GR);

					//scan our dds
					//oben nach unten
					for (size_t row = 0; row < h; row++)
					{
						//links nach rechts
						for (size_t column = 0; column < w; column++)
						{
							uint64 offset = (row* w + column) * channels; //4 is because we use 4 channels
							float Red = (float)(*(pixels + offset) / 65535);
							float Green = (float)(*(pixels + offset + 1) / 65535);
							float Blue = (float)(*(pixels + offset + 2) / 65535);
							if(channels == 4)
							{
								float Alpha = (float)(*(pixels + offset + 3) / 65535);
								auto OgreValSpec = Ogre::ColourValue(Alpha, Alpha, Alpha);
								AlphaMap.setColourAt(OgreValSpec, column, row, 0);
							}
							//z is not transfered so use
							auto OgreValNormal = Ogre::ColourValue(Red, Green,Blue);
							ColorMap.setColourAt(OgreValNormal, column, row, 0);


						}
					}
					//r
					ColorMap.save(GetSavePath() + MaterialName + "_c.tif");
					if(channels == 4)
					AlphaMap.save(GetSavePath()  + MaterialName + "_a.tif");
					QSF_LOG_PRINTS(INFO, "saved color and alpha map map")
						delete[] bufferColor;
					delete[] bufferAlpha;
				}
			else if(textureType == 2)
				{
			//image_magic
				Magick::Image image(Filepath);
				int w = (int)image.columns();
				int h = (int)image.rows();
				if (w <= 0 || h <= 0)
				{
					QSF_LOG_PRINTS(INFO, "broken image (size detection failed)"<< Filepath)
				}
				if (image.channels() != 4)
				{
					QSF_LOG_PRINTS(INFO,"cant detect alpha channel of "<< Filepath)
					return false;
				}
				MagickCore::Quantum *pixels = image.getPixels(0, 0, w, h);
				//now create a normal map img
				uint8* buffer = new uint8[w * h * 8];
				Ogre::Image NormalMap;
				NormalMap.loadDynamicImage(buffer, w, h, Ogre::PixelFormat::PF_FLOAT16_RGB);

				uint8* bufferSpec = new uint8[w * h * 8]; //red
				Ogre::Image SpecMap;
				SpecMap.loadDynamicImage(bufferSpec, w, h, Ogre::PixelFormat::PF_FLOAT32_GR);

				uint8* bufferGloss = new uint8[w * h * 8]; //blue
				Ogre::Image GlossMapp;
				GlossMapp.loadDynamicImage(bufferGloss, w, h, Ogre::PixelFormat::PF_FLOAT32_GR);

				//scan our dds
				//oben nach unten
				for (size_t row = 0; row < h; row++)
				{
					//links nach rechts
					for (size_t column = 0; column < w; column++)
					{
						uint64 offset = (row* w + column)*4; //4 is because we use 4 channels
						float Red = (float)(*(pixels + offset) / 65535);
						float Green = (float)(*(pixels + offset+1) / 65535);
						float Blue = (float)(*(pixels + offset + 2) / 65535);
						float Alpha = (float)(*(pixels + offset + 3) / 65535);
						//z is not transfered so use
						auto OgreValNormal = Ogre::ColourValue(Alpha,Green);
						NormalMap.setColourAt(OgreValNormal,column,row,0);

						auto OgreValSpec = Ogre::ColourValue(Red, Red,Red, 1.f);
						SpecMap.setColourAt(OgreValSpec, column, row, 0);

						auto OgreValGloss = Ogre::ColourValue(Blue, Blue, Blue, 1.f);
						GlossMapp.setColourAt(OgreValGloss, column, row, 0);
					}
				}
				//r
				NormalMap.save(GetSavePath()  + MaterialName + "_n.tif");
				GlossMapp.save(GetSavePath()  + MaterialName + "_g.tif");
				SpecMap.save(GetSavePath()  + MaterialName+"_s.tif");
				QSF_LOG_PRINTS(INFO,"saved normal, gloss and spec map")
					delete[] buffer;
				delete[] bufferSpec;
				delete[] bufferGloss;
			} 
			else if(textureType == 3)
			{
				Magick::Image image(Filepath);
				int w = (int)image.columns();
				int h = (int)image.rows();
				if (w <= 0 || h <= 0)
				{
					QSF_LOG_PRINTS(INFO, "broken image (size detection failed)" << Filepath)
				}
				/*if (image.channels() != 4)
				{
					QSF_LOG_PRINTS(INFO, "cant detect alpha channel of " << Filepath)
						return;
				}*/
				size_t ChannelSize = image.channels();
				MagickCore::Quantum *pixels = image.getPixels(0, 0, w, h);
				//now create a normal map img
				uint8* Tintbuffer = new uint8[w * h * 8];
				Ogre::Image Tint;
				Tint.loadDynamicImage(Tintbuffer, w, h, Ogre::PixelFormat::PF_FLOAT32_GR);

				uint8* Smutbuffer = new uint8[w * h * 8]; //red
				Ogre::Image Smut;
				Smut.loadDynamicImage(Smutbuffer, w, h, Ogre::PixelFormat::PF_FLOAT32_GR);

				uint8* HeightBuffer = new uint8[w * h * 8]; //blue
				Ogre::Image HeightMap;
				HeightMap.loadDynamicImage(HeightBuffer, w, h, Ogre::PixelFormat::PF_FLOAT32_GR);

				//scan our dds
				//oben nach unten
				if(ChannelSize == 4)
				{
				for (size_t row = 0; row < h; row++)
				{
					//links nach rechts
					for (size_t column = 0; column < w; column++)
					{
						uint64 offset = (row* w + column) * 4; //4 is because we use 4 channels
						float Red = (float)(*(pixels + offset) / 65535);
						float Green = (float)(*(pixels + offset + 1) / 65535);
						float Blue = (float)(*(pixels + offset + 2) / 65535);
						float Alpha = (float)(*(pixels + offset + 3) / 65535);
						//z is not transfered so use
						auto OgreValNormal = Ogre::ColourValue(Red, Red,Red);
						Tint.setColourAt(OgreValNormal, column, row, 0);

						auto OgreValSpec = Ogre::ColourValue(Green, Green, Green, 1.f);
						Smut.setColourAt(OgreValSpec, column, row, 0);

						auto OgreValGloss = Ogre::ColourValue(Blue, Blue, Blue, 1.f);
						HeightMap.setColourAt(OgreValGloss, column, row, 0);
					}
				}
				}
				else if(ChannelSize ==3)
				{
					for (size_t row = 0; row < h; row++)
					{
						//links nach rechts
						for (size_t column = 0; column < w; column++)
						{
							uint64 offset = (row* w + column) * 3; //4 is because we use 4 channels
							float Red = (float)(*(pixels + offset) / 65535);
							float Green = (float)(*(pixels + offset + 1) / 65535);
							float Blue = (float)(*(pixels + offset + 2) / 65535);
							//z is not transfered so use
							auto OgreValNormal = Ogre::ColourValue(Red, Red, Red);
							Tint.setColourAt(OgreValNormal, column, row, 0);

							auto OgreValSpec = Ogre::ColourValue(Green, Green, Green, 1.f);
							Smut.setColourAt(OgreValSpec, column, row, 0);

							auto OgreValGloss = Ogre::ColourValue(Blue, Blue, Blue, 1.f);
							HeightMap.setColourAt(OgreValGloss, column, row, 0);
						}
					}
				}
				else
				{
					QSF_LOG_PRINTS(INFO, "channel size of the image is not supported")
				}

				//r
				Tint.save(GetSavePath()  + MaterialName + "_t.tif");
				Smut.save(GetSavePath() + MaterialName + "_smut.tif");
				HeightMap.save(GetSavePath()  + MaterialName + "_h.tif");
				QSF_LOG_PRINTS(INFO, "saved tint,gloss and smut map")
					delete[] Tintbuffer;
				delete[] Smutbuffer;
				delete[] HeightBuffer;
			}
			else if (textureType == 4)
			{
				Magick::Image image(Filepath);
				int w = (int)image.columns();
				int h = (int)image.rows();
				if (w <= 0 || h <= 0)
				{
					QSF_LOG_PRINTS(INFO, "broken image (size detection failed)" << Filepath)
				}
				if (image.channels() != 4 && image.channels() != 3)
				{
					QSF_LOG_PRINTS(INFO, "cant detect alpha channel of " << Filepath)
						return false;
				}
				MagickCore::Quantum *pixels = image.getPixels(0, 0, w, h);
				//now create a normal map img
				uint8* EmissiveBufer = new uint8[w * h * 8];
				Ogre::Image EmissiveMap;
				EmissiveMap.loadDynamicImage(EmissiveBufer, w, h, Ogre::PixelFormat::PF_FLOAT16_RGB);
				size_t Channels = image.channels();

				//scan our dds
				//oben nach unten
				for (size_t row = 0; row < h; row++)
				{
					//links nach rechts
					for (size_t column = 0; column < w; column++)
					{
						uint64 offset = (row* w + column) * Channels; //4 is because we use 4 channels
						float Red = (float)(*(pixels + offset) / 65535);
						float Green = (float)(*(pixels + offset + 1) / 65535);
						float Blue = (float)(*(pixels + offset + 2) / 65535);
						//z is not transfered so use
						auto OgreValNormal = Ogre::ColourValue(Red, Green, Blue);
						EmissiveMap.setColourAt(OgreValNormal, column, row, 0);
					}
				}
				//r
				EmissiveMap.save(GetSavePath() + MaterialName + "_e.tif");
				QSF_LOG_PRINTS(INFO, "saved emissive Map")
					delete[] EmissiveBufer;
			}
			return true;

			
		
		
		}

		void ImageDecompiler::onSetSaveDirectory(const bool pressed)
		{
			QString Path = GetSavePath().c_str();
			QWidget* qtw = new QWidget();
			auto fileName = QFileDialog::getExistingDirectory(qtw,
				tr("Set Save Directory"), Path);
				if(fileName == "")
				return;
			QSF_LOG_PRINTS(INFO, fileName.toStdString()+"/")
				mSavepath = fileName.toStdString() + "//";

			std::ofstream ofs(path + "image_decompiler.txt", std::ofstream::trunc);

			ofs << mSavepath;

			ofs.close();

		}


		std::string ImageDecompiler::GetSavePath()
		{
			return mSavepath;
		}

		std::string ImageDecompiler::InitSavePath()
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
//[-------------------------------------------------------]
//[ Namespace                                             ]
//[-------------------------------------------------------]
	} // editor
} // user
