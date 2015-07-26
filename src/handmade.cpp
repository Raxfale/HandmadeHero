//
// Handmade Hero - game code
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include "handmade.h"
#include "asset.h"
#include <cassert>
#include <iostream>

using namespace std;
using namespace HandmadePlatform;


///////////////////////// GameState::Constructor ////////////////////////////
GameState::GameState(PlatformInterface &platform)
  : assets(platform.gamememory)
{
  assert(this == platform.gamememory.data);
}


///////////////////////// game_init /////////////////////////////////////////
extern "C" void game_init(PlatformInterface &platform)
{ 
  cout << "Init" << endl;

  GameState &state = *new(allocate<GameState>(platform.gamememory)) GameState(platform);

  state.entropy.seed(random_device()());

  initialise_asset_system(platform, state.assets);
}


///////////////////////// game_reinit ///////////////////////////////////////
extern "C" void game_reinit(PlatformInterface &platform)
{
//  GameState &state = *static_cast<GameState*>(platform.gamememory.data);

  cout << "ReInit" << endl;
}


///////////////////////// game_update ///////////////////////////////////////
extern "C" void game_update(PlatformInterface &platform, GameInput const &input, float dt)
{
//  GameState &state = *static_cast<GameState*>(platform.gamememory.data);
}


///////////////////////// game_render ///////////////////////////////////////
extern "C" void game_render(PlatformInterface &platform, uint32_t *bits)
{
  GameState &state = *static_cast<GameState*>(platform.gamememory.data);

  state.testvalue = fmod(state.testvalue + 0.1, 2*3.14159265f);

//  auto asset = state.assets.find(state.entropy, AssetType::Tree);
  auto asset = state.assets.find(state.entropy, AssetType::HeroHead, array<AssetTag, 1>({ AssetTagId::Orientation, state.testvalue }));

  if (asset)
  {
    auto data = state.assets.request(platform, asset);

    if (data)
    {
      for(int y = 0; y < asset->height; ++y)
      {
        for(int x = 0; x < asset->width; ++x)
        {
          bits[(y+50)*960+(x+50)] = *(static_cast<uint32_t*>(data) + y*asset->width + x);
        }
      }
    }
  }
}
