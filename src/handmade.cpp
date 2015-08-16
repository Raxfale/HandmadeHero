//
// Handmade Hero - game code
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include "handmade.h"
#include "renderer.h"
#include <cassert>
#include <iostream>

using namespace std;
using namespace lml;
using namespace HandmadePlatform;


///////////////////////// GameState::Constructor ////////////////////////////
GameState::GameState(StackAllocator<> const &allocator)
  : assets(allocator)
{
}


///////////////////////// game_init /////////////////////////////////////////
extern "C" void game_init(PlatformInterface &platform)
{  
  cout << "Init" << endl;

  GameState &state = *new(allocate<GameState>(platform.gamememory)) GameState(platform.gamememory);

  assert(&state == platform.gamememory.data);

  state.entropy.seed(random_device()());

  initialise_asset_system(platform, state.assets);
}


///////////////////////// game_reinit ///////////////////////////////////////
extern "C" void game_reinit(PlatformInterface &platform)
{
  cout << "ReInit" << endl;

//  GameState &state = *static_cast<GameState*>(platform.gamememory.data);
}


///////////////////////// game_update ///////////////////////////////////////
extern "C" void game_update(PlatformInterface &platform, GameInput const &input, float dt)
{
//  GameState &state = *static_cast<GameState*>(platform.gamememory.data);

}


///////////////////////// game_render ///////////////////////////////////////
extern "C" void game_render(PlatformInterface &platform)
{
  GameState &state = *static_cast<GameState*>(platform.gamememory.data);

  auto barrier = state.assets.aquire_barrier();

  PushBuffer pushbuffer(platform.renderscratchmemory, 1*1024*1024);

  *pushbuffer.push<Renderable::Camera>() = { 22, 12 };

  *pushbuffer.push<Renderable::Clear>() = { Color4(1.0, 0.3, 0.5) };

  state.testvalue = fmod(state.testvalue + 0.1, 2*3.14159265f);

//  auto asset = state.assets.find(state.entropy, AssetType::Tree);
  auto asset = state.assets.find(state.entropy, AssetType::HeroHead, array<AssetTag, 1>({ AssetTagId::Orientation, state.testvalue }));

  if (asset)
  {
    auto data = state.assets.request(platform, asset);

    if (data)
    {
      *pushbuffer.push<Renderable::Bitmap>() = { asset->width, asset->height, data };
    }
  }

  render(platform, pushbuffer);

  state.assets.release_barrier(barrier);
}
