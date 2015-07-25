//
// Handmade Hero - asset system
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include "asset.h"
#include "assetpack.h"
#include <algorithm>
#include <iostream>

using namespace std;


//|---------------------- Asset ---------------------------------------------
//|--------------------------------------------------------------------------

///////////////////////// Asset::Constructor ////////////////////////////////
Asset::Asset(allocator_type const &allocator)
  : tags(allocator)
{
}

Asset::Asset(Asset const &other, allocator_type const &allocator)
  : tags(allocator)
{
  *this = other;
}


//|---------------------- AssetManager --------------------------------------
//|--------------------------------------------------------------------------

///////////////////////// AssetManager::Constructor /////////////////////////
AssetManager::AssetManager(allocator_type const &allocator)
  : m_assets(allocator)
{
}


///////////////////////// AssetManager::initialise //////////////////////////
void AssetManager::initialise(std::vector<Asset, StackAllocator<Asset>> const &assets)
{
  for(auto &asset : assets)
    m_assets.insert({ asset.type, asset });

  cout << "Initialised " << m_assets.size() << " assets" << endl;
}



///////////////////////// initialise_asset_system ///////////////////////////
void initialise_asset_system(HandmadePlatform::PlatformInterface &platform, AssetManager &assetmanager)
{
  std::vector<Asset, StackAllocator<Asset>> assets(platform.gamescratchmemory);

  auto hhas = platform.open_type_enumerator("hha");

  while (auto handle = platform.iterate_type_enumerator(hhas))
  {
    try
    {
      platform.open_handle(handle);

      PackHeader header;

      platform.read_handle(handle, 0, &header, sizeof(header));

      if (header.signature[0] != 0xCA || header.signature[1] != 'H' || header.signature[2] != 'H' || header.signature[3] != 'A')
        throw runtime_error("Invalid hha file");

      uint64_t position = sizeof(header);

      Asset asset(platform.gamescratchmemory);

      asset.file = handle;

      while (true)
      {
        PackChunk chunk;

        platform.read_handle(handle, position, &chunk, sizeof(chunk));

        switch (chunk.type)
        {
          case 0x54455341: // ASET
            {
              PackAssetHeader assetheader;

              platform.read_handle(handle, position + sizeof(chunk), &assetheader, sizeof(assetheader));

              asset.type = static_cast<AssetType>(assetheader.type);

              asset.tags = {};

              break;
            }

          case 0x47415441: // ATAG
            {
              PackAssetTag assettag;

              platform.read_handle(handle, position + sizeof(chunk), &assettag, sizeof(assettag));

              asset.tags.push_back({ static_cast<AssetTagId>(assettag.id), assettag.value });

              break;
            }

          case 0x52444849: // IHDR
            {
              PackImageHeader imageheader;

              platform.read_handle(handle, position + sizeof(chunk), &imageheader, sizeof(imageheader));

              asset.width = imageheader.width;
              asset.height = imageheader.height;

              break;
            }

          case 0x54414449: // IDAT
            {
              asset.fileposition = position;

              break;
            }

          case 0x444e4541: // AEND
            {
              assets.push_back(asset);

              break;
            }
        }

        if (chunk.type == 0x444e4548) // HEND
          break;

        position += chunk.length + sizeof(chunk) + sizeof(uint32_t);
      }
    }
    catch(std::exception &e)
    {
      cerr << "Error on asset file : " << e.what() << endl;
    }
  }

  platform.close_type_enumerator(hhas);

  assetmanager.initialise(assets);
}

