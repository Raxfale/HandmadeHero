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
#include <cassert>
#include <iostream>

using namespace std;

namespace
{

  ///////////////////////// asset_match_factor ////////////////////////////////
  float asset_match_factor(Asset const &asset, AssetTag const *tags, float const *weights, size_t n)
  {
    float result = 0.0;

    for(size_t i = 0; i < n; ++i)
    {
      float factor = 0.0;

      for(auto &tag: asset.tags)
      {
        if (tag.id == tags[i].id)
        {
          factor = max(factor, 1/(abs(tag.value - tags[i].value) + 1e-6f));
        }
      }

      result += weights[i] * factor;
    }

    return result;
  }

}


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
    asset.second.slot = slot;

    m_slots[slot++].state = Slot::State::Empty;
  }

  cout << "Initialised " << m_assets.size() << " assets" << endl;
}


///////////////////////// AssetManager::find ////////////////////////////////
Asset const *AssetManager::find(random_type &random, AssetType type, AssetTag const *tags, float const *weights, std::size_t n) const
{
  Asset const *result = nullptr;

  auto range = m_assets.equal_range(type);

  int bestcount = 0;
  float bestfactor = -1.0;

  for(auto it = range.first; it != range.second; ++it)
  {
    auto &asset = it->second;

    auto matchfactor = asset_match_factor(asset, tags, weights, n);

    if (matchfactor == bestfactor)
      ++bestcount;

    if (matchfactor > bestfactor)
    {
      bestcount = 1;
      bestfactor = matchfactor;

      result = &asset;
    }
  }

  if (bestcount > 1)
  {
    auto selected = uniform_int_distribution<int>(1, bestcount)(random);

    for(auto it = range.first; selected != 0; --selected, ++it)
    {
      // TODO: I'm a little worried about the termination condition of this loop, have another look
      while(asset_match_factor(it->second, tags, weights, n) != bestfactor)
        ++it;

      result = &it->second;
    }
  }

  return result;
}


///////////////////////// AssetManager::request /////////////////////////////
void const *AssetManager::request(HandmadePlatform::v1::PlatformInterface &platform, Asset const *asset)
{
  Slot &slot = m_slots[asset->slot];

  if (slot.state.load(std::memory_order_relaxed) == Slot::State::Loaded)
  {
    std::atomic_thread_fence(std::memory_order_acquire);

    return slot.data;
  }

  fetch(platform, asset);

  return nullptr;
}


///////////////////////// AssetManager::background_loader ///////////////////
void AssetManager::background_loader(HandmadePlatform::PlatformInterface &platform, void *ldata, void *rdata)
{
  Asset &asset = *static_cast<Asset*>(rdata);
  Slot &slot = static_cast<AssetManager*>(ldata)->m_slots[asset.slot];

  try
  {
    platform.read_handle(asset.filehandle, asset.datapos + sizeof(PackChunk), slot.data, asset.datasize);
  }
  catch(exception &e)
  {
    cerr << "Background Read Error: " << e.what() << endl;
  }

  slot.state = Slot::State::Loaded;

  cout << "Asset " << asset.slot << " loaded" << endl;
}


///////////////////////// AssetManager::fetch ///////////////////////////////
void AssetManager::fetch(HandmadePlatform::PlatformInterface &platform, Asset const *asset)
{
  Slot::State expected = Slot::State::Empty;

  if (atomic_compare_exchange_strong(&m_slots[asset->slot].state, &expected, Slot::State::Loading))
  {
    m_slots[asset->slot].data = allocate<char>(m_allocator, asset->datasize);

    platform.submit_work(background_loader, this, const_cast<Asset*>(asset));
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

        if (chunk.type == 0x444e4548) // HEND
          break;

        switch (chunk.type)
        {
          case 0x54455341: // ASET
            {
              PackAssetHeader aset;

              platform.read_handle(handle, position + sizeof(chunk), &aset, sizeof(aset));

              asset.type = static_cast<AssetType>(aset.type);

              asset.tags = {};

              break;
            }

          case 0x47415441: // ATAG
            {
              PackAssetTag atag;

              platform.read_handle(handle, position + sizeof(chunk), &atag, sizeof(atag));

              asset.tags.push_back({ static_cast<AssetTagId>(atag.id), atag.value });

              break;
            }

          case 0x52444849: // IHDR
            {
              PackImageHeader ihdr;

              platform.read_handle(handle, position + sizeof(chunk), &ihdr, sizeof(ihdr));

              asset.width = ihdr.width;
              asset.height = ihdr.height;
              asset.datapos = ihdr.dataoffset;
              asset.datasize = ihdr.width * ihdr.height * sizeof(uint32_t);

              break;
            }

          case 0x444e4541: // AEND
            {
              assets.push_back(asset);

              break;
            }
        }

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

