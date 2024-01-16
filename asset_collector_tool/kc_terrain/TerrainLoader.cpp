// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool\kc_terrain\TerrainComponent.h>
#include "TerrainLoader.h"
#include <qsf/asset/AssetProxy.h>
#include <ogre\Terrain\OgreTerrainGroup.h>
#include <qsf/QsfHelper.h>
#include <qsf\log\LogSystem.h>
#include <boost\property_tree\ptree.hpp>
#include <qsf\file\FileStream.h>
#include <qsf\file\FileHelper.h>

namespace kc_terrain
{



	int TerrainLoader::LoadTerrain(kc_terrain::TerrainComponent * TC)
	{
		int counter = 0;
		counter = counter + 100 * LoadColorMaps(TC);
		counter = counter + 10 * LoadTerrainTextures(TC);
		counter = counter + LoadTerrainHeigtMap(TC);
		Ogre::TerrainGroup::TerrainIterator terrainIterator = TC->getOgreTerrainGroup()->getTerrainIterator();
		while (terrainIterator.hasMoreElements()) //refresh here because we dont know if anything fails like loading terrain textures because an asset is missing
		{
			Ogre::TerrainGroup::TerrainSlot* a = terrainIterator.getNext();
			TC->RefreshMaterial(a->instance);
		}
		return counter;
	}

	bool TerrainLoader::LoadTerrainTextures(kc_terrain::TerrainComponent * TC)
	{
		//create layers
		qsf::Asset* TerrainLayerList = TC->GetTerrainLayerList().getAsset();
		qsf::Asset* Terrain1_4 = TC->GetNewTextureMap1_4().getAsset();
		qsf::Asset* Terrain5_8 = TC->GetNewTextureMap5_8().getAsset();

		//check if everything is there
		if (TerrainLayerList == nullptr)
		{
			QSF_LOG_PRINTS(INFO, "LoadTerrainTextures: TerrainLayerList was not found")
				return false;
		}
		if (Terrain5_8 == nullptr || Terrain1_4 == nullptr)
		{
			QSF_LOG_PRINTS(INFO, "LoadTerrainTextures: TerrainLayerTexture was not found")
				return false;
		}
		//first read layer list
		std::vector<std::vector<std::string>> LayerList;
		try
		{
			boost::property_tree::ptree	mRootTree;
			qsf::FileStream file(TC->GetTerrainLayerList().getLocalAssetName() + ".json", qsf::File::READ_MODE);
			//QSF_LOG_PRINTS(INFO,"path "<< path)

			qsf::FileHelper::readJson(file, mRootTree);
			for (auto& it : mRootTree)
			{
				std::vector<std::string> Output;
				//QSF_LOG_PRINTS(INFO, it.first);
				for (auto& it2 : it.second)
				{
					//QSF_LOG_PRINTS(INFO, "Layer " << it2.first << ": " << it2.second.data())
					if (it2.second.data() != "") //it tries to push in empty terrain <-> maybe bad translation
						Output.push_back(it2.second.data());
				}
				LayerList.push_back(Output);

			}

			// v.first is the name of the child.
			// v.second is the child tree.
		}
		catch (const std::exception& e)
		{
			QSF_LOG_PRINTS(INFO, "LoadTerrainTextures (1)" << e.what())
				return false;
		}
		//then load in images
		Ogre::Image OI1_4;
		Ogre::Image OI5_8;
		try
		{
			OI1_4.load(std::string(TC->GetNewTextureMap1_4().getLocalAssetName() + ".tif").c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			OI5_8.load(std::string(TC->GetNewTextureMap5_8().getLocalAssetName() + ".tif").c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			if (&OI1_4 == nullptr)
			{
				QSF_LOG_PRINTS(ERROR, "sth went wrong when loading texture maps (1-4)")
					return false;
			}
			if (&OI5_8 == nullptr)
			{
				QSF_LOG_PRINTS(ERROR, "sth went wrong when loading texture maps (5-8)")
					return false;
			}
		}
		catch (const std::exception& e)
		{
			QSF_LOG_PRINTS(INFO, "LoadTerrainTextures (2)" << e.what())
				return false;
		}
		auto width = OI1_4.getHeight();
		auto height = OI5_8.getWidth();

		//we need to flip images and even flip y of each part
		OI1_4.flipAroundX();
		OI5_8.flipAroundX();
		//finally we need partsize and number of parts
		Ogre::TerrainGroup::TerrainIterator terrainIterator = TC->getOgreTerrainGroup()->getTerrainIterator();
		int Totalparts = 0;
		while (terrainIterator.hasMoreElements()) // add the layer to all terrains in the terrainGroup
		{
			Ogre::TerrainGroup::TerrainSlot* a = terrainIterator.getNext();
			Totalparts++;

		}
		//is rounding required?
		//for squares its easy as we just take square root
		int parts_one_axis = (int)glm::round(glm::sqrt(Totalparts));
		int partsize = width / parts_one_axis;
		//QSF_LOG_PRINTS(INFO, "poa " << parts_one_axis << " ps " << partsize)
			// finally start
		Ogre::TerrainGroup::TerrainIterator terrainIterator2 = TC->getOgreTerrainGroup()->getTerrainIterator();
		int counter = 0;
		while (terrainIterator2.hasMoreElements()) // add the layer to all terrains in the terrainGroup
		{

			Ogre::TerrainGroup::TerrainSlot* a = terrainIterator2.getNext();
			if (LayerList.at(counter).empty())
			{
				counter++;
				continue;
			}

			auto Terrain = a->instance;
			/*while (Terrain->getLayerCount() > 1)
			{
				Terrain->removeLayer(1);
			}*/
			//add first layer if not existing
			if (Terrain->getLayerCount() == 0)
				Terrain->addLayer();
			Terrain->setLayerTextureName(0, 0, LayerList.at(counter).at(0));
			//QSF_LOG_PRINTS(INFO, "Layerlist for " << counter)
			//QSF_LOG_PRINTS(INFO, "0 " << LayerList.at(counter).at(0))
			for (uint8 t = 1; t < LayerList.at(counter).size(); t++)
			{
				//QSF_LOG_PRINTS(INFO, (int)t << " " << LayerList.at(counter).at(t))
				Terrain->addLayer();
				Terrain->setLayerTextureName(t, 0, LayerList.at(counter).at(t));
				auto CurrentBlendMap = Terrain->getLayerBlendMap(t);
				int OffsetX = a->x*partsize;
				int OffsetY = a->y*partsize;
				if (CurrentBlendMap == nullptr)
				{
					QSF_LOG_PRINTS(INFO, "cant write blendmap at terrain" << counter << " BM " << t << " cannot open blendmap")
						continue;
				}
				for (int x = 0; x < partsize; x++)
				{
					for (int y = 0; y < partsize; y++)
					{
						int  x_for_pixel = x + OffsetX;
						//mirrored
						int y_for_pixel = OffsetY + partsize - 1 - y;
						float color = 0.f;
						if (t == 1)
						{
							color = OI1_4.getColourAt(x_for_pixel, y_for_pixel, 0).r;
							//if (x % 30 == 0)
								//QSF_LOG_PRINTS(INFO, "r " << color << " " << x_for_pixel << " " << y_for_pixel << " y "<< y << " x " << x);
						}
						else if (t == 2)
						{
							color = OI1_4.getColourAt(x_for_pixel, y_for_pixel, 0).g;
						}
						else if (t == 3)
						{
							color = OI1_4.getColourAt(x_for_pixel, y_for_pixel, 0).b;
						}
						else if (t == 4)
						{
							color = OI1_4.getColourAt(x_for_pixel, y_for_pixel, 0).a;
							//if (x == 1 && y == 1)
								//QSF_LOG_PRINTS(INFO, "a " << color);
						}
						else if (t == 5)
						{
							color = OI5_8.getColourAt(x_for_pixel, y_for_pixel, 0).r;
						}
						else
						{
							QSF_LOG_PRINTS(INFO, "t is " << t << " but why?")
						}
						CurrentBlendMap->setBlendValue(x, y, color);
					}
				}
				CurrentBlendMap->dirty();
				CurrentBlendMap->update();
			}
			counter++;
		}

		return true;
	}

	bool TerrainLoader::LoadTerrainHeigtMap(kc_terrain::TerrainComponent * TC)
	{
		qsf::AssetProxy HeightMapAsset = TC->GetNewHeightMap();
		float Min = TC->GetMinHeight();
		float Max = TC->GetMaxHeight();

		//check if everything is there
		if (HeightMapAsset.getAsset() == nullptr)
		{
			QSF_LOG_PRINTS(INFO, "LoadTerrainHeigtMap: TerrainLayerList was not found")
				return false;
		}
		Ogre::Image OI_HeightMap;
		try
		{
			OI_HeightMap.load(std::string(HeightMapAsset.getLocalAssetName() + ".tif").c_str(), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			if (&OI_HeightMap == nullptr)
			{
				QSF_LOG_PRINTS(ERROR, "sth went wrong when loading texture maps (1-4)")
					return false;
			}
		}
		catch (const std::exception& e)
		{
			QSF_LOG_PRINTS(INFO, "LoadTerrainHeigtMap (2)" << e.what())
				return false;
		}
		auto width = OI_HeightMap.getHeight();
		auto height = OI_HeightMap.getWidth();
		OI_HeightMap.flipAroundX();
		Ogre::TerrainGroup::TerrainIterator terrainIterator2 = TC->getOgreTerrainGroup()->getTerrainIterator();
		int counter = 0;
		while (terrainIterator2.hasMoreElements()) // add the layer to all terrains in the terrainGroup
		{

			Ogre::TerrainGroup::TerrainSlot* a = terrainIterator2.getNext();
			counter++;
		}
		int parts_one_axis = (int)glm::round(glm::sqrt(counter));
		//shouldnt do that... better use Terrain->getsize
		int partsize = TC->getOgreTerrainGroup()->getTerrain(0, 0)->getSize();
		//QSF_LOG_PRINTS(INFO, "Partsize " << partsize << " parts_one_axis " << parts_one_axis)
			Ogre::TerrainGroup::TerrainIterator terrainIterator = TC->getOgreTerrainGroup()->getTerrainIterator();

		/* notice that partsize is handled different as pixels are used on borders share same px on image*/
		while (terrainIterator.hasMoreElements()) // add the layer to all terrains in the terrainGroup
		{

			Ogre::TerrainGroup::TerrainSlot* a = terrainIterator.getNext();
			auto Terrain = a->instance;
			int OffsetX = (partsize - 1)*a->x;
			int OffsetY = (partsize - 1)*a->y;
			for (long x = 0; x <= partsize; x++)
			{
				for (long y = 0; y <= partsize; y++)
				{
					float HeightVal = Min + (Max - Min)*OI_HeightMap.getColourAt(OffsetX + x, OffsetY + y, 0).r;
					Terrain->setHeightAtPoint(x, y, HeightVal);
				}
			}
			Terrain->update(true);
		}
		return true;
	}

	bool TerrainLoader::LoadColorMaps(kc_terrain::TerrainComponent * TC)
	{
		qsf::AssetProxy ColorMap = TC->GetColorMap();
		if (ColorMap.getAsset() == nullptr)
		{
			QSF_LOG_PRINTS(INFO, "no colormap was set")
				return false;

		}
		TC->InformMaterialGeneratorAboutNewColorMap(ColorMap.getGlobalAssetId());
		//no need to update materials as we do it later when doing blendmaps
		return true;
	}

}
