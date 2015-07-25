//
// Handmade Hero - asset system
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#pragma once

#include "platform.h"
#include "memory.h"
#include <vector>
#include <unordered_map>

enum class AssetType
{
  HeroHead = 0x4f01,
  HeroTorso = 0x3402,
  HeroCape = 0x26f2,

  Tree = 0x2279,

};

enum class AssetTagId
{
  Orientation = 0x57d8,
};

struct AssetTag
{
  AssetTagId id;

  float value;
};


//|---------------------- Asset ---------------------------------------------
//|--------------------------------------------------------------------------

class Asset
{
  public:

    typedef StackAllocator<> allocator_type;

    Asset(allocator_type const &allocator);

    Asset(Asset const &other, allocator_type const &allocator);

  public:

    AssetType type;

    std::vector<AssetTag, StackAllocator<AssetTag>> tags;

    HandmadePlatform::PlatformInterface::handle_t file;

    uint64_t fileposition;

    union
    {
      struct
      {
        int width;
        int height;
      };

      struct
      {
        int channels;
      };
    };
};


//|---------------------- AssetManager --------------------------------------
//|--------------------------------------------------------------------------

class AssetManager
{
  public:

    typedef StackAllocator<> allocator_type;

    AssetManager(allocator_type const &allocator);

  public:

    void initialise(std::vector<Asset, StackAllocator<Asset>> const &assets);

  private:

    std::unordered_multimap<AssetType, Asset, std::hash<AssetType>, std::equal_to<>, std::scoped_allocator_adaptor<allocator_type>> m_assets;
};

// Initialise
void initialise_asset_system(HandmadePlatform::PlatformInterface &platform, AssetManager &assetmanager);
