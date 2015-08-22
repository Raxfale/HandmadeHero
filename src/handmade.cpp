//
// Handmade Hero - game code
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include "handmade.h"
#include "rendergroup.h"
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

  RenderGroup rendergroup(platform, &state.assets, platform.renderscratchmemory, 1*1024*1024);

  rendergroup.projection(-11.0f, -6.0f, 11.0f, 6.0f, 0.6f/8.0f);

  rendergroup.clear({ 0.5f, 0.2f, 0.7f });

  rendergroup.push_rect(Vec3(0.0f, 0.0f, 0.0f), Rect2({ 0.0f, 0.0f }, { 3.0f, 5.0f }), Color4(0.0f, 1.0f, 0.0f, 1.0f));
  rendergroup.push_rect(Vec3(0.0f, 0.0f, 0.0f), Rect2({ -1.0f, -1.0f }, { 1.0f, 1.0f }), Color4(0.0f, 0.0f, 1.0f, 0.5f));

  rendergroup.push_rect(Vec3(-6.0f, 3.0f, 0.0f), Rect2({ -0.5f, -1.0f }, { 0.5f, 1.0f }), Color4(0.5f, 0.8f, 0.0f, 0.9f));
  rendergroup.push_rect(Vec3(-6.0f, -3.0f, 0.0f), Rect2({ -0.5f, -1.0f }, { 0.5f, 1.0f }), Color4(0.5f, 0.8f, 0.0f, 0.9f));
  rendergroup.push_rect(Vec3(6.0f, 3.0f, 0.0f), Rect2({ -0.5f, -1.0f }, { 0.5f, 1.0f }), Color4(0.5f, 0.8f, 0.0f, 0.9f));
  rendergroup.push_rect(Vec3(6.0f, -3.0f, 0.0f), Rect2({ -0.5f, -1.0f }, { 0.5f, 1.0f }), Color4(0.5f, 0.8f, 0.0f, 0.9f));

  rendergroup.push_rect(Vec3(-6.0f, 3.0f, -3.0f), Rect2({ -0.5f, -1.0f }, { 0.5f, 1.0f }), Color4(0.5f, 0.8f, 0.0f, 0.6f));
  rendergroup.push_rect(Vec3(-6.0f, -3.0f, -3.0f), Rect2({ -0.5f, -1.0f }, { 0.5f, 1.0f }), Color4(0.5f, 0.8f, 0.0f, 0.6f));
  rendergroup.push_rect(Vec3(6.0f, 3.0f, -3.0f), Rect2({ -0.5f, -1.0f }, { 0.5f, 1.0f }), Color4(0.5f, 0.8f, 0.0f, 0.6f));
  rendergroup.push_rect(Vec3(6.0f, -3.0f, -3.0f), Rect2({ -0.5f, -1.0f }, { 0.5f, 1.0f }), Color4(0.5f, 0.8f, 0.0f, 0.6f));

  auto tree = state.assets.find(state.entropy, AssetType::Tree);

  rendergroup.push_bitmap(Vec3(-6.0f, 3.0f, 0.0f), 2.0f, tree, Color4(0.5f, 0.8f, 0.0f, 0.9f));
  rendergroup.push_bitmap(Vec3(-6.0f, -3.0f, 0.0f), 2.0f, tree, Color4(0.5f, 0.8f, 0.0f, 0.9f));
  rendergroup.push_bitmap(Vec3(6.0f, 3.0f, 0.0f), 2.0f, tree, Color4(0.5f, 0.8f, 0.0f, 0.9f));
  rendergroup.push_bitmap(Vec3(6.0f, -3.0f, 0.0f), 2.0f, tree, Color4(0.5f, 0.8f, 0.0f, 0.9f));

  rendergroup.push_bitmap(Vec3(-6.0f, 3.0f, -3.0f), 2.0f, tree, Color4(0.5f, 0.8f, 0.0f, 0.9f));
  rendergroup.push_bitmap(Vec3(-6.0f, -3.0f, -3.0f), 2.0f, tree, Color4(0.5f, 0.8f, 0.0f, 0.9f));
  rendergroup.push_bitmap(Vec3(6.0f, 3.0f, -3.0f), 2.0f, tree, Color4(0.5f, 0.8f, 0.0f, 0.9f));
  rendergroup.push_bitmap(Vec3(6.0f, -3.0f, -3.0f), 2.0f, tree, Color4(0.5f, 0.8f, 0.0f, 0.9f));

  state.testvalue = fmod(state.testvalue + 0.1, 2*3.14159265f);

  auto asset = state.assets.find(state.entropy, AssetType::HeroHead, array<AssetTag, 1>({ AssetTagId::Orientation, state.testvalue }));

  rendergroup.push_bitmap(Vec3(0.0f, -4.0f, 0.0f), 5.0f, asset);

  render(platform, rendergroup);

  RenderGroup debuggroup(platform, &state.assets, platform.renderscratchmemory, 1*1024*1024);

  debuggroup.projection(0, 0, 960, 540, 0);

  auto font = state.assets.find(AssetType::Font);

  debuggroup.push_text(Vec3(0, 0.0f, 0.0f), 64, font, "Hello World");
  debuggroup.push_text(Vec3(0, font->ascent + font->descent + font->leading, 0.0f), 64, font, "WA To iyjgf");

  render(platform, debuggroup);
}
