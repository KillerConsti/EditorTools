// Copyright (C) 2012-2019 Promotion Software GmbH


//[-------------------------------------------------------]
//[ Includes                                              ]
//[-------------------------------------------------------]
#include "asset_collector_tool/PrecompiledHeader.h"
#include <asset_collector_tool\kc_terrain\TerrainComponent.h>
#include "TerrainScaler.h"
#include <qsf/asset/AssetProxy.h>
#include <ogre\Terrain\OgreTerrainGroup.h>
#include <qsf/QsfHelper.h>
#include <qsf\log\LogSystem.h>
#include <boost\property_tree\ptree.hpp>
#include <qsf\file\FileStream.h>
#include <qsf\file\FileHelper.h>
#include <asset_collector_tool\extern\include\Magick++.h>
namespace kc_terrain
{



	bool TerrainScaler::ScaleHeightMap(int NewSize, kc_terrain::TerrainComponent* TC)
	{
		auto HeightMapToRead = TC->GetNewHeightMap();
		if (HeightMapToRead.getAsset() == nullptr)
		{
			QSF_LOG_PRINTS(INFO, "no height map is set in terrain component")
				return false;
		}


		try
		{
			auto image = new Magick::Image(HeightMapToRead.getAbsoluteCachedAssetDataFilename());
			if (image == nullptr)
				return false;
			image->scale(Magick::Geometry(NewSize, NewSize, 0, 0));
			image->syncPixels();
			image->write(HeightMapToRead.getAbsoluteCachedAssetDataFilename());
		}
		catch (const std::exception& e)
		{
			QSF_LOG_PRINTS(INFO, e.what())
				return false;
		}
		UpdateAsset(TC->GetNewHeightMap().getGlobalAssetId());
		TC->SetHeightMapSize(NewSize);
		TC->setAllPropertyOverrideFlags(true);
		return true;
	}

	bool TerrainScaler::ScaleColorMap(int NewSize, kc_terrain::TerrainComponent * TC)
	{
		auto ColorMapToRead = TC->GetColorMap();
		if (ColorMapToRead.getAsset() == nullptr)
		{
			QSF_LOG_PRINTS(INFO, "no color map is set in terrain component")
				return false;
		}


		try
		{
			auto image = new Magick::Image(ColorMapToRead.getAbsoluteCachedAssetDataFilename());
			if (image == nullptr)
				return false;
			image->scale(Magick::Geometry(NewSize, NewSize, 0, 0));
			image->syncPixels();
			image->write(ColorMapToRead.getAbsoluteCachedAssetDataFilename());
		}
		catch (const std::exception& e)
		{
			QSF_LOG_PRINTS(INFO, e.what())
				return false;
		}
		UpdateAsset(ColorMapToRead.getGlobalAssetId());
		//Call generate
		TC->setAllPropertyOverrideFlags(true);
		return true;
	}

	bool TerrainScaler::ScaleTerrainTextures(int NewSize, kc_terrain::TerrainComponent * TC)
	{
		auto TexMap1 = TC->GetNewTextureMap1_4();
		if (TexMap1.getAsset() == nullptr)
		{
			QSF_LOG_PRINTS(INFO, "no terrain texture map is set in terrain component")
				return false;
		}
		auto TexMap2 = TC->GetNewTextureMap1_4();
		if (TexMap2.getAsset() == nullptr)
		{
			QSF_LOG_PRINTS(INFO, "no terrain texture map is set in terrain component")
				return false;
		}


		try
		{
			auto image = new Magick::Image(TexMap1.getAbsoluteCachedAssetDataFilename());
			if (image == nullptr)
				return false;
			auto image2 = new Magick::Image(TexMap2.getAbsoluteCachedAssetDataFilename());
			if (image == nullptr)
				return false;
			image->scale(Magick::Geometry(NewSize, NewSize, 0, 0));
			image->syncPixels();
			image->write(TexMap1.getAbsoluteCachedAssetDataFilename());

			image2->scale(Magick::Geometry(NewSize, NewSize, 0, 0));
			image2->syncPixels();
			image2->write(TexMap2.getAbsoluteCachedAssetDataFilename());
		}
		catch (const std::exception& e)
		{
			QSF_LOG_PRINTS(INFO, e.what())
				return false;
		}
		UpdateAsset(TexMap1.getGlobalAssetId());
		UpdateAsset(TexMap2.getGlobalAssetId());
		TC->SetBlendMapSize(NewSize);
		TC->setAllPropertyOverrideFlags(true);
		return true;
	}

	bool TerrainScaler::IncreaseChunks(int NewSize, kc_terrain::TerrainComponent * TC)
	{
		if (TC->GetTerrainLayerList().getAsset() == nullptr)
			return false;
		//this may double number of chunks.. It is not too hard to perform at all
		boost::property_tree::ptree rootPTree;
		auto it = TC->getOgreTerrainGroup()->getTerrainIterator();
		int counter = 0;

		while (it.hasMoreElements()) // add the layer to all terrains in the terrainGroup
		{
			Ogre::TerrainGroup::TerrainSlot* a = it.getNext();
			boost::property_tree::ptree layers;
			for (size_t x = 0; x < a->instance->getLayerCount() && x < 6; x++)
			{
				layers.put("Layer" + boost::lexical_cast<std::string>(x), a->instance->getLayerTextureName((uint8)x, 0).c_str());
				QSF_LOG_PRINTS(INFO, a->instance->getLayerTextureName((uint8)x, 0).c_str())
			}
			QSF_LOG_PRINTS(INFO, "Write Terrain Textures 2 " << counter)
				counter++;
			rootPTree.add_child(boost::lexical_cast<std::string>(a->x * 2) + "_" + boost::lexical_cast<std::string>(a->y * 2), layers);
			rootPTree.add_child(boost::lexical_cast<std::string>(a->x * 2 + 1) + "_" + boost::lexical_cast<std::string>(a->y * 2), layers);
			rootPTree.add_child(boost::lexical_cast<std::string>(a->x * 2) + "_" + boost::lexical_cast<std::string>(a->y * 2 + 1), layers);
			rootPTree.add_child(boost::lexical_cast<std::string>(a->x * 2 + 1) + "_" + boost::lexical_cast<std::string>(a->y * 2 + 1), layers);
		}
		//does it work with sort?
		//sorting bla bla
		//rootPTree.sort(rootPTree);
		TC->kc_setTerrainChunksPerEdge(TC->kc_getTerrainChunksPerEdge() * 2);
		boost::nowide::ofstream ofs(TC->GetTerrainLayerList().getAbsoluteCachedAssetDataFilename());
		qsf::FileHelper::writeJson(ofs, rootPTree);
		UpdateAsset(TC->GetTerrainLayerList().getGlobalAssetId());
		TC->setAllPropertyOverrideFlags(true);
		return true;
	}

	bool TerrainScaler::DecreaseChunks(int NewSize, kc_terrain::TerrainComponent * TC)
	{
		std::vector<LayerData> MyNewChunks;
		int PixelsPerSite = TC->GetBlendtMapSize() / TC->kc_getTerrainChunksPerEdge();
		for (int _x_ = 0; _x_ < TC->kc_getTerrainChunksPerEdge() / 2; _x_++)
		{
			for (int _y_ = 0; _y_ < TC->kc_getTerrainChunksPerEdge() / 2; _y_++)
			{
				int RealX = _x_ * 2;
				int RealY = _y_ * 2;
				auto Terrain1 = TC->getOgreTerrainGroup()->getTerrain(RealX, RealY);
				auto Terrain2 = TC->getOgreTerrainGroup()->getTerrain(RealX, RealY + 1);
				auto Terrain3 = TC->getOgreTerrainGroup()->getTerrain(RealX + 1, RealY);
				auto Terrain4 = TC->getOgreTerrainGroup()->getTerrain(RealX + 1, RealY + 1);

				LayerData LayerDataResult1 = ScanLayers(Terrain1, PixelsPerSite);
				LayerData LayerDataResult2 = ScanLayers(Terrain2, PixelsPerSite);
				LayerData LayerDataResult3 = ScanLayers(Terrain3, PixelsPerSite);
				LayerData LayerDataResult4 = ScanLayers(Terrain4, PixelsPerSite);

				LayerDataResult1 = MixLayers(LayerDataResult1,LayerDataResult2, PixelsPerSite,0, PixelsPerSite);
				LayerDataResult1 = MixLayers(LayerDataResult1, LayerDataResult3,0, PixelsPerSite, PixelsPerSite);
				LayerDataResult1 = MixLayers(LayerDataResult1, LayerDataResult4, PixelsPerSite, PixelsPerSite, PixelsPerSite);
				LayerDataResult1.xTerrain = _x_;
				LayerDataResult1.yTerrain = _y_;
				//well now we have mixed everything together
				//we need to cut it down now a little
				bool Sorted = false;
				while (!Sorted)
				{
					Sorted = true;
					if(LayerDataResult1.BlendMaps.size() < 6)
					break; //no need to sort for less then 6 Blendmaps
					//this sort shall make the largest BlendMap at front 
					//the smallest Blendmaps get kicked out if they get rank 6 and above
					for (size_t t = 0; t < LayerDataResult1.BlendMaps.size() - 1; t++)
					{
						if (LayerDataResult1.BlendMaps.at(t).second.size() < LayerDataResult1.BlendMaps.at(t + 1).second.size())
						{
							Sorted = false;
							LayerDataResult1.BlendMaps.at(t).second.swap(LayerDataResult1.BlendMaps.at(t + 1).second);
						}
					}
				}
				if (LayerDataResult1.BlendMaps.size() >= 6)
				{
					LayerDataResult1.BlendMaps.erase(LayerDataResult1.BlendMaps.begin()+5,LayerDataResult1.BlendMaps.end());
				}
				MyNewChunks.push_back(LayerDataResult1);
			}

		}
		WriteNewChunkData(MyNewChunks,TC,NewSize);
		return true;
	}

	TerrainScaler::LayerData TerrainScaler::ScanLayers(Ogre::Terrain * OI, int PixelsPerSite)
	{
		LayerData Result;
		for (size_t Layers = 0; Layers < OI->getLayerCount() && Layers < 6; Layers++)
		{
			std::string LayerTextureName = OI->getLayerTextureName((uint8)Layers, 0).c_str();
			int Number = 0;
			//QSF_LOG_PRINTS(INFO, a->instance->getLayerTextureName((uint8)x, 0).c_str())
			if (Layers == 0)
			{
				Result.BaseLayer = LayerTextureName;
				continue;
				//this layer is special it is 
			}
			if (OI->getBlendTextureCount() <= Layers)
			{
				//how to handle? we got an empty layer? maybe just forgot about him
				continue;
			}
			auto Blendmap = OI->getLayerBlendMap((uint8)Layers);
			if (Blendmap == nullptr)
				continue;
			std::pair<std::string, std::vector<glm::vec3>> BlendMapValues;
			BlendMapValues.first = LayerTextureName;
			for (size_t terrain_x = 0; terrain_x < PixelsPerSite; terrain_x++)
			{
				for (size_t terrain_y = 0; terrain_y < PixelsPerSite; terrain_y++)
				{
					float val = Blendmap->getBlendValue(terrain_x, terrain_y);
					if (val != 0)
						BlendMapValues.second.push_back(glm::vec3((float)terrain_x, (float)terrain_y, val));
				}
			}
			Result.BlendMaps.push_back(BlendMapValues);
		}
		return Result;
	}

	TerrainScaler::LayerData TerrainScaler::MixLayers(LayerData L1, LayerData L2, int OffsetX, int OffsetY, int ChunkSize)
	{
		//first check out how many different base layers we have
		std::vector<std::string> BaseLayers;
		int Counter = 0;
		if (L1.BaseLayer != "")
		{
			Counter = Counter + 1;
			BaseLayers.push_back(L1.BaseLayer);
		}
		if (L2.BaseLayer != "")
		{
			Counter = Counter + 2;
			BaseLayers.push_back(L2.BaseLayer);
		}
		if (L1.BaseLayer != ""  && L2.BaseLayer != "")
		{
			if (L1.BaseLayer != L2.BaseLayer)
				Counter = Counter + 4;
			else
			{
				Counter = Counter + 8;
			}
		}

		if (Counter == 0) //both datas are invalid /empty
			return L1;
		if (Counter == 1) //only L1 is valid
			return L1;
		if (Counter == 2) //only L2 valid
			return L2;
		//L1 and L2 are valid
		if (Counter == 7) //different BaseLayers
		{
			LayerData ResultLayer;
			ResultLayer.BaseLayer = L1.BaseLayer;
			ResultLayer.BlendMaps = L1.BlendMaps;
			//handle base layer
			int i = IsIncluded(ResultLayer, L2.BaseLayer);
			//0 can be ignored here as it is best result
			if (i == -1) //it is not included anywhere
			{
				std::vector<glm::vec3> Mappoints;
				for (size_t x = 0; x < ChunkSize; x++)
				{
					for (size_t y = 0; y < ChunkSize; y++) //set all values to 1
					{
						Mappoints.push_back(glm::vec3((float)(x + OffsetX),(float)( y + OffsetY), 1.f));
					}

				}
				ResultLayer.BlendMaps.insert(ResultLayer.BlendMaps.begin(), std::pair<std::string, std::vector<glm::vec3>>(L2.BaseLayer, Mappoints));

				//all other LAyers ... handle like case counter =11
				for (auto MixMaps : L2.BlendMaps)
				{
					int i = IsIncluded(ResultLayer, MixMaps.first);
					if (i == -1)
					{
						ResultLayer.BlendMaps.push_back(MixMaps);
						//we have to think about positions
						for (size_t tz = 0; tz < ResultLayer.BlendMaps.at(ResultLayer.BlendMaps.size() - 1).second.size(); tz++)
						{
							ResultLayer.BlendMaps.at(ResultLayer.BlendMaps.size() - 1).second.at(tz).x += OffsetX;
							ResultLayer.BlendMaps.at(ResultLayer.BlendMaps.size() - 1).second.at(tz).x += OffsetY;

						}
					}
					else if (i == 0) //can happen but has no effect (as it is always handled like it is 1)
					{

					}
					else
					{
						for (auto a : MixMaps.second)
						{
							ResultLayer.BlendMaps.at(i - 1).second.push_back(glm::vec3(a.x + OffsetX, a.y + OffsetY, a.z));
						}
					}
				}
			}
			return ResultLayer;
		}
		else if (Counter == 11) //same base layers
		{
			LayerData ResultLayer;
			ResultLayer.BaseLayer = L1.BaseLayer;
			ResultLayer.BlendMaps = L1.BlendMaps;
			for (auto MixMaps : L2.BlendMaps)
			{
				int i = IsIncluded(ResultLayer, MixMaps.first);
				if (i == -1)
				{
					ResultLayer.BlendMaps.push_back(MixMaps);
					//we have to think about positions
					for (size_t tz = 0; tz < ResultLayer.BlendMaps.at(ResultLayer.BlendMaps.size() - 1).second.size(); tz++)
					{
						ResultLayer.BlendMaps.at(ResultLayer.BlendMaps.size() - 1).second.at(tz).x += OffsetX;
						ResultLayer.BlendMaps.at(ResultLayer.BlendMaps.size() - 1).second.at(tz).x += OffsetY;

					}
				}
				else if (i == 0) //shoudlnt happen
				{

				}
				else
				{
					for (auto a : MixMaps.second)
					{
						ResultLayer.BlendMaps.at(i - 1).second.push_back(glm::vec3(a.x + OffsetX, a.y + OffsetY, a.z));
					}
				}
			}
			return ResultLayer;
		}
		return LayerData();
	}

	bool TerrainScaler::WriteNewChunkData(std::vector<LayerData> ResultData, kc_terrain::TerrainComponent* TC, int NumberOfParts)
	{
		std::string widthandheight = boost::lexical_cast<std::string>(TC->GetBlendtMapSize()) + "x" + boost::lexical_cast<std::string>(TC->GetBlendtMapSize());
		int Partsize = TC->getBlendMapSize()/NumberOfParts;
		int BlendMapSize = TC->GetBlendtMapSize();
		Magick::Image* MagImage_1_4 = new Magick::Image();
		MagImage_1_4->size(widthandheight);
		MagImage_1_4->magick("TIF");
		MagImage_1_4->type(Magick::ImageType::TrueColorAlphaType);

		//for now we support only #5 so make it grayscale
		Magick::Image* MagImage_5_8 = new Magick::Image();
		MagImage_5_8->size(widthandheight);
		MagImage_5_8->magick("TIF");
		MagImage_5_8->type(Magick::ImageType::GrayscaleType);

		auto Quant1_4 = MagImage_1_4->getPixels(0, 0, TC->GetBlendtMapSize(), TC->GetBlendtMapSize());
		auto Quant5_8 = MagImage_5_8->getPixels(0, 0, TC->GetBlendtMapSize(), TC->GetBlendtMapSize());
		for (auto Data : ResultData)
			{

				int Offsetx = Data.xTerrain*NumberOfParts;
				int Offsety = Data.yTerrain* NumberOfParts;
				for (uint32 layerIndex = 0; layerIndex < Data.BlendMaps.size(); ++layerIndex)
				{
					for(auto BlendData : Data.BlendMaps.at(layerIndex).second)
							//Read and write data
							//map 1
							if (layerIndex < 5) //1 - r 2-g  3-b 4-a
							{
								uint64 offset = ((Offsety + Partsize - 1 - (int)BlendData.y)* BlendMapSize + Offsetx + (int)BlendData.x) * 4 + layerIndex; //4 is because we use 4 channels
																																			   //layerindex -1 is the channel selection 0 is red, 1 is green, 2 is blue and 3 is alpha. As we start at layerindex 1 we have to substract -1
								*(Quant1_4 + offset) = BlendData.z * 65535.f;
							}
							else
							{
								uint64 offset = ((Offsety + Partsize - 1 - (int)BlendData.y)* BlendMapSize + Offsetx + (int)BlendData.x); //just one greyscale channel <-> no offset
								*(Quant5_8 + offset) = BlendData.z * 65535.f;
							}
						}
					}
		//flip?
		MagImage_1_4->flip();
		MagImage_5_8->flip();
		MagImage_1_4->syncPixels();
		MagImage_5_8->syncPixels();

		MagImage_1_4->write(TC->GetNewTextureMap1_4().getAbsoluteCachedAssetDataFilename());
		MagImage_5_8->write(TC->GetNewTextureMap1_4().getAbsoluteCachedAssetDataFilename());

		//write json
		boost::property_tree::ptree rootPTree;

		for(auto Datas : ResultData) // add the layer to all terrains in the terrainGroup
		{
			if (Datas.BaseLayer == "")
			{
				boost::property_tree::ptree layers;
				rootPTree.add_child(boost::lexical_cast<std::string>(Datas.xTerrain) + "_" + boost::lexical_cast<std::string>(Datas.yTerrain), layers);
			}
			else
			{
				boost::property_tree::ptree layers;
				layers.put("Layer" + boost::lexical_cast<std::string>(0), Datas.BaseLayer);
				for (size_t t = 0; t < Datas.BlendMaps.size();t++)
				{
					layers.put("Layer" + boost::lexical_cast<std::string>(t+1),Datas.BlendMaps.at(t).first);
				}
				rootPTree.add_child(boost::lexical_cast<std::string>(Datas.xTerrain) + "_" + boost::lexical_cast<std::string>(Datas.yTerrain), layers);
			}
		}
		//does it work with sort?
		boost::nowide::ofstream ofs(TC->GetTerrainLayerList().getAbsoluteCachedAssetDataFilename());
		qsf::FileHelper::writeJson(ofs, rootPTree);
		TC->kc_setTerrainChunksPerEdge(NumberOfParts);
		UpdateAsset(TC->GetNewTextureMap1_4().getGlobalAssetId());
		UpdateAsset(TC->GetNewTextureMap5_8().getGlobalAssetId());
		UpdateAsset(TC->GetTerrainLayerList().getGlobalAssetId());
		TC->setAllPropertyOverrideFlags(true);
		
		return true;
	}

	int TerrainScaler::IsIncluded(LayerData SearchLayers, std::string TextureName)
	{
		if (SearchLayers.BaseLayer == TextureName)
			return 0;
		for (size_t t = 0; t < SearchLayers.BlendMaps.size(); t++)
		{
			if (SearchLayers.BlendMaps.at(t).first == TextureName)
				return (int)t + 1;
		}
		return -1;
	}

	void TerrainScaler::UpdateAsset(qsf::GlobalAssetId uid)
	{
	}

}
