//
// Handmade Hero - game code
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#pragma once

#include "platform.h"
#include "memory.h"
#include "lml.h"
#include "asset.h"


//|---------------------- GameState -----------------------------------------
//|--------------------------------------------------------------------------

struct GameState
{
  GameState(StackAllocator<> const &allocator);

  float testvalue = 0;

  std::mt19937 entropy;

  AssetManager assets;
};
