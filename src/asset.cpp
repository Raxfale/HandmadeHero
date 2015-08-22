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


///////////////////////// AssetEx::Constructor //////////////////////////////
AssetManager::AssetEx::AssetEx(Asset const &asset, allocator_type const &allocator)
  : Asset(asset, allocator)
{
  slot = nullptr;
}



//|---------------------- AssetManager --------------------------------------
//|--------------------------------------------------------------------------

///////////////////////// AssetManager::Constructor /////////////////////////
AssetManager::AssetManager(allocator_type const &allocator)
  : m_allocator(allocator),
    m_assets(allocator)
{
  m_head = nullptr;
}


///////////////////////// AssetManager::initialise //////////////////////////
void AssetManager::initialise(std::vector<Asset, StackAllocator<Asset>> const &assets, std::size_t slabsize)
{
  m_head = new(allocate<char, alignof(Slot)>(m_allocator, slabsize)) Slot;

  m_head->size = slabsize;
  m_head->after = nullptr;
  m_head->prev = m_head;
  m_head->next = m_head;
  m_head->state = Slot::State::Empty;

  for(auto &asset : assets)
  {
    m_assets.emplace(asset.type, asset);
  }

  cout << "Initialised " << m_assets.size() << " assets" << endl;
}


///////////////////////// AssetManager::find ////////////////////////////////
Asset const *AssetManager::find(AssetType type) const
{
  Asset const *result = nullptr;

  auto it = m_assets.find(type);

  if (it != m_assets.end())
    result = &it->second;

  return result;
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


///////////////////////// AssetManager::aquire_slot /////////////////////////
AssetManager::Slot *AssetManager::aquire_slot(size_t size)
{
  auto bytes = ((size + sizeof(Slot) - 1)/alignof(Slot) + 1) * alignof(Slot);

  for(auto slot = m_head; true; slot = slot->next)
  {
    if (slot->state == Slot::State::Barrier)
      return nullptr;

    if (slot->state == Slot::State::Loaded)
    {
      // evict

      slot->asset->slot = nullptr;

      slot->state = Slot::State::Empty;
    }

    if (slot->state == Slot::State::Empty)
    {
      if (slot->after && slot->after->state == Slot::State::Empty)
      {
        // merge

        slot->after->prev->next = slot->after->next;
        slot->after->next->prev = slot->after->prev;

        slot->size += slot->after->size;
        slot->after = slot->after->after;

        m_head = slot;
      }

      if (slot->size > bytes + sizeof(Slot))
      {
        // split

        Slot *newslot = reinterpret_cast<Slot*>(reinterpret_cast<char*>(slot) + bytes);

        newslot->size = slot->size - bytes;
        newslot->after = slot->after;
        newslot->prev = m_head->prev;
        newslot->next = m_head;
        newslot->state = Slot::State::Empty;

        newslot->prev->next = newslot;
        newslot->next->prev = newslot;

        slot->size = bytes;
        slot->after = newslot;

        m_head = newslot;
      }

      if (slot->size >= bytes)
      {
        // match

        touch_slot(slot);

        return slot;
      }
    }

    if (slot->next == m_head)
      return nullptr;
  }
}


///////////////////////// AssetManager::touch_slot //////////////////////////
AssetManager::Slot *AssetManager::touch_slot(AssetManager::Slot *slot)
{
  if (slot == m_head)
    m_head = m_head->next;

  slot->prev->next = slot->next;
  slot->next->prev = slot->prev;

  slot->next = m_head;
  slot->prev = m_head->prev;

  slot->prev->next = slot;
  slot->next->prev = slot;

  return slot;
}


///////////////////////// AssetManager::request /////////////////////////////
void const *AssetManager::request(HandmadePlatform::PlatformInterface &platform, Asset const *asset)
{
  lock_guard<mutex> lock(m_mutex);

  auto &slot = static_cast<AssetEx*>(const_cast<Asset*>(asset))->slot;

  if (!slot)
  {
    slot = aquire_slot(asset->datasize);

    if (slot)
    {
      slot->state = Slot::State::Loading;

      slot->asset = static_cast<AssetEx*>(const_cast<Asset*>(asset));

      platform.submit_work(background_loader, this, slot);
    }

    return nullptr;
  }

  touch_slot(slot);

  if (slot->state != Slot::State::Loaded)
    return nullptr;

  return slot->data;
}


///////////////////////// AssetManager::aquire_barrier //////////////////////
uintptr_t AssetManager::aquire_barrier()
{
  lock_guard<mutex> lock(m_mutex);

  auto slot = aquire_slot(0);

  if (slot)
  {
    slot->state = Slot::State::Barrier;
  }

  return reinterpret_cast<uintptr_t>(slot);
}


///////////////////////// AssetManager::release_barrier /////////////////////
void AssetManager::release_barrier(uintptr_t barrier)
{
  lock_guard<mutex> lock(m_mutex);

  auto slot = reinterpret_cast<Slot*>(barrier);

  if (slot)
  {
    slot->state = Slot::State::Empty;
  }
}


///////////////////////// AssetManager::background_loader ///////////////////
void AssetManager::background_loader(HandmadePlatform::PlatformInterface &platform, void *ldata, void *rdata)
{
  auto &manager = *static_cast<AssetManager*>(ldata);

  auto &slot = *static_cast<Slot*>(rdata);

  try
  {
    platform.read_handle(slot.asset->filehandle, slot.asset->datapos + sizeof(PackChunk), slot.data, slot.asset->datasize);
  }
  catch(exception &e)
  {
    cerr << "Background Read Error: " << e.what() << endl;
  }

  {
    lock_guard<mutex> lock(manager.m_mutex);

    slot.state = Slot::State::Loaded;
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
              asset.aspect = (float)asset.width / (float)asset.height;
              asset.alignx = ihdr.alignx;
              asset.aligny = ihdr.aligny;
              asset.datapos = ihdr.dataoffset;
              asset.datasize = ihdr.width * ihdr.height * sizeof(uint32_t);

              break;
            }

          case 0x52444846: // FHDR
            {
              PackFontHeader fhdr;

              platform.read_handle(handle, position + sizeof(chunk), &fhdr, sizeof(fhdr));

              asset.ascent = fhdr.ascent;
              asset.descent = fhdr.descent;
              asset.leading = fhdr.leading;
              asset.datapos = fhdr.dataoffset;
              asset.datasize = fhdr.datasize;

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

  assetmanager.initialise(assets, 512*1024*1024);
}

