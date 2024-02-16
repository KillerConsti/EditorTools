// Copyright (C) 2012-2017 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include "asset_collector_tool/view/UnoImageWriter.h"
#include "ui_UnoImageWriter.h" // Automatically created by Qt's uic (output directory is "tmp\qt\uic\qsf_editor" within the hand configured Visual Studio files, another directory when using CMake)


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
#include <QtGui\qdesktopservices.h>

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
		const uint32 UnoImageWriter::PLUGINABLE_ID = qsf::StringHash("user::editor::UnoImageWriter");


		//[-------------------------------------------------------]
		//[ Public methods                                        ]
		//[-------------------------------------------------------]
		UnoImageWriter::UnoImageWriter(qsf::editor::ViewManager* viewManager, QWidget* qWidgetParent) :
			qsf::editor::View(viewManager, qWidgetParent),
			//View(viewManager, qWidgetParent),
			mUiUnoImageWriter(nullptr),
			mSavepath("")
		{
			// Add the created Qt dock widget to the given Qt main window and tabify it for better usability
			addViewAndTabify(reinterpret_cast<QMainWindow&>(*qWidgetParent), Qt::RightDockWidgetArea);

		}

		UnoImageWriter::~UnoImageWriter()
		{

			// Destroy the UI view instance
			if (nullptr != mUiUnoImageWriter)
			{
				delete mUiUnoImageWriter;
			}
		}


		//[-------------------------------------------------------]
		//[ Protected virtual qsf::editor::View methods           ]
		//[-------------------------------------------------------]
		void UnoImageWriter::retranslateUi()
		{
			// Retranslate the content of the UI, this automatically clears the previous content
			mUiUnoImageWriter->retranslateUi(this);
		}

		void UnoImageWriter::changeVisibility(bool visible)
		{
			// Lazy evaluation: If the view is shown the first time, create its content
			if (visible && nullptr == mUiUnoImageWriter)
			{
				// Setup the view content
				QWidget* contentWidget = new QWidget(this);
				{

					// Load content to widget
					mUiUnoImageWriter = new Ui::UnoImageWriter();
					mUiUnoImageWriter->setupUi(contentWidget);
				}

				// Set content to view
				setWidget(contentWidget);
				// Connect Qt signals/slots
				connect(mUiUnoImageWriter->openpicturebutton, SIGNAL(clicked(bool)), this, SLOT(OnPushLoadMaterial_or_texture(bool)));
				connect(mUiUnoImageWriter->decompilebutton, SIGNAL(clicked(bool)), this, SLOT(onPushDecompileButton(bool)));
				connect(mUiUnoImageWriter->SetSaveLocationButton, SIGNAL(clicked(bool)), this, SLOT(onSetSaveDirectory(bool)));
				connect(mUiUnoImageWriter->openloc, SIGNAL(clicked(bool)), this, SLOT(onopenloc(bool)));
				InitSavePath();
			}
			else if (!visible && nullptr == mUiUnoImageWriter)
			{
			}

		}

		void UnoImageWriter::showEvent(QShowEvent * qShowEvent)
		{
			// Call the base implementation
			View::showEvent(qShowEvent);

			// Perform a GUI rebuild to ensure the GUI content is up-to-date
			rebuildGui();
		}

		void UnoImageWriter::hideEvent(QHideEvent * qHideEvent)
		{
			// Call the base implementation
			View::hideEvent(qHideEvent);
		}


		//[-------------------------------------------------------]
		//[ Private methods                                       ]
		//[-------------------------------------------------------]
		void UnoImageWriter::rebuildGui()
		{
		}

		void UnoImageWriter::onPushDecompileButton(const bool pressed)
		{
			DecompileImage("","");
		}

		void UnoImageWriter::OnPushLoadMaterial_or_texture(const bool pressed)
		{
			path = path + "/../EMERGENCY 5/log";
			auto path2 = QString(path.c_str());
			QString filter = "PNG File (*.png)";
			auto Dialog = QFileDialog::getOpenFileName(nullptr, QString("Logfile to open"), path2, filter);
			if (Dialog.size() < 1)
				return;
			std::string Dia = Dialog.toStdString();
			mUiUnoImageWriter->line_edit_searchprototype->setText(Dia.c_str());

		}



		bool UnoImageWriter::DecompileImage(std::string TextureName, std::string MaterialName)
		{
			try
			{


				std::string Filepath = mUiUnoImageWriter->line_edit_searchprototype->text().toStdString();
				Magick::Image image(Filepath);
				int w = (int)image.columns();
				int h = (int)image.rows();
				if (w <= 0 || h <= 0)
				{
					QSF_LOG_PRINTS(INFO, "broken image (size detection failed)" << Filepath)
						QSF_LOG_PRINTS(INFO, "width " << w << " height " << h << " channels " << image.channels())
						return false;
				}
				if (image.channels() != 4 && image.channels() != 3)
				{
					QSF_LOG_PRINTS(INFO, "cant detect alpha channel of " << Filepath)
						return false;
				}
				MagickCore::Quantum *pixels = image.getPixels(0, 0, w, h);
				std::vector<uint16> MyValues;
				//now create a normal map img
				for (size_t row = 0; row < h; row++)
				{
					//links nach rechts
					for (size_t column = 0; column < w; column++)
					{
						uint64 offset = (row* w + column) * image.channels(); //4 is because we use 4 channels
						float Red = (float)(*(pixels + offset) / 256);
						float Green = (float)(*(pixels + offset + 1) / 256);
						float Blue = (float)(*(pixels + offset + 2) / 256);
						//z is not transfered so use
						MyValues.push_back(color565(Red,Green,Blue));
					}
				}
				std::ofstream ofs(Filepath+".h", std::ofstream::trunc);
				ofs << w << " x " << h<<"\n";
				for (auto a : MyValues)
				{
					ofs << a<< ",";
				}

				ofs.close();
			}
			catch (const std::exception& e)
			{
				QSF_LOG_PRINTS(INFO, e.what())
					return false;
			}
			return true;




		}

		void UnoImageWriter::onSetSaveDirectory(const bool pressed)
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

		void UnoImageWriter::onopenloc(const bool pressed)
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile(GetSavePath().c_str()));
		}


		std::string UnoImageWriter::GetSavePath()
		{
			return mSavepath;
		}

		std::string UnoImageWriter::InitSavePath()
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
		uint16_t UnoImageWriter::color565(uint8_t r, uint8_t g, uint8_t b)
		{

			return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
		}
		//[-------------------------------------------------------]
		//[ Namespace                                             ]
		//[-------------------------------------------------------]
	} // editor
} // user
