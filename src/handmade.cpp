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
  GameState &state = *new(allocate<GameState>(platform.gamememory)) GameState(platform);

  cout << "Init" << endl;

  initialise_asset_system(platform, state.assets);

}


///////////////////////// game_reinit ///////////////////////////////////////
extern "C" void game_reinit(PlatformInterface &platform)
{
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
//  GameState &state = *static_cast<GameState*>(platform.gamememory.data);
//  cout << "Render" << endl;
}
