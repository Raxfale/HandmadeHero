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
  : m_allocator(allocator),
    m_assets(allocator),
    m_slots(nullptr)
{
}


///////////////////////// AssetManager::initialise //////////////////////////
void AssetManager::initialise(std::vector<Asset, StackAllocator<Asset>> const &assets)
{
  for(auto &asset : assets)
    m_assets.insert({ asset.type, asset });

  m_slots = new(allocate<Slot>(m_allocator, m_assets.size())) Slot[m_assets.size()];

  size_t slot = 0;
  for(auto &asset : m_assets)
  {
    m_slots[slot].state = Slot::State::Empty;

    asset.second.slot = slot++;
  }

  cout << "Initialised " << m_assets.size() << " assets" << endl;
}


///////////////////////// AssetManager::background_loader ///////////////////
void AssetManager::background_loader(HandmadePlatform::PlatformInterface &platform, void *ldata, void *rdata)
{
  Asset &asset = *static_cast<Asset*>(rdata);
  Slot &slot = static_cast<AssetManager*>(ldata)->m_slots[asset.slot];

  try
  {
    platform.read_handle(asset.filehandle, asset.fileposition + sizeof(PackChunk), slot.data, asset.datasize);
  }
  catch(exception &e)
  {
    cerr << "Background Read Error: " << e.what() << endl;
  }

  slot.state = Slot::State::Loaded;

  cout << "Asset " << asset.slot << " loaded" << endl;
}


///////////////////////// AssetManager::fetch ///////////////////////////////
void AssetManager::fetch(HandmadePlatform::PlatformInterface &platform, Asset *asset)
{
  Slot::State expected = Slot::State::Empty;

  if (atomic_compare_exchange_strong(&m_slots[asset->slot].state, &expected, Slot::State::Loading))
  {
    m_slots[asset->slot].data = allocate<char>(m_allocator, asset->datasize);

    platform.submit_work(background_loader, this, asset);
  }
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

      asset.filehandle = handle;

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
              asset.datasize = imageheader.width * imageheader.height * sizeof(uint32_t);

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

