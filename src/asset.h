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
#include <atomic>
#include <vector>
#include <unordered_map>
#include <random>
#include <mutex>

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

    std::vector<AssetTag, allocator_type> tags;

    HandmadePlatform::PlatformInterface::handle_t filehandle;

    uint64_t datapos;
    std::size_t datasize;

    union
    {
      struct // image info
      {
        int width;
        int height;
      };

      struct // audio info
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

    typedef std::mt19937 random_type;

    typedef StackAllocator<> allocator_type;

    AssetManager(allocator_type const &allocator);

    AssetManager(AssetManager const &) = delete;

  public:

    // initialise asset metadata
    void initialise(std::vector<Asset, StackAllocator<Asset>> const &assets, std::size_t slabsize);

    // Find an asset by metadata
    template<std::size_t N = 0>
    Asset const *find(random_type &random, AssetType type, std::array<AssetTag, N> const &tags = {}, std::array<float, N> const &weights = []() { std::array<float, N> one; std::fill_n(one.data(), N, 1.0f); return one; }()) const
    {
      return find(random, type, tags.data(), weights.data(), N);
    }

    // Request asset payload. May not be loaded, will initiate background load and return null.
    void const *request(HandmadePlatform::PlatformInterface &platform, Asset const *asset);

  public:

    uintptr_t aquire_barrier();

    void release_barrier(uintptr_t barrier);

  protected:

    Asset const *find(random_type &random, AssetType type, AssetTag const *tags, float const *weights, std::size_t n) const;

  private:

    allocator_type m_allocator;

  private:

    struct Slot;

    struct AssetEx : public Asset
    {
      AssetEx(Asset const &asset, allocator_type const &allocator);

      Slot *slot;
    };

    std::unordered_multimap<AssetType, AssetEx, std::hash<AssetType>, std::equal_to<>, std::scoped_allocator_adaptor<allocator_type>> m_assets;

  private:

    struct Slot
    {
      enum class State
      {
        Empty,
        Barrier,
        Loading,
        Loaded
      };

      State state;

      AssetEx *asset;

      std::size_t size;

      Slot *after;

      Slot *prev;
      Slot *next;

      char data[];
    };

    Slot *m_head;

    Slot *aquire_slot(std::size_t size);

    Slot *touch_slot(Slot *slot);

    static void background_loader(HandmadePlatform::PlatformInterface &platform, void *ldata, void *rdata);

  private:

    mutable std::mutex m_mutex;
};

// Initialise
void initialise_asset_system(HandmadePlatform::PlatformInterface &platform, AssetManager &assetmanager);
