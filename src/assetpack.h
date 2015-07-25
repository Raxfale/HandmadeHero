//
// Handmade Hero - asset pack
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#pragma once

#include "platform.h"

#pragma pack(push, 1)

struct PackHeader
{
  uint8_t signature[8];
};

struct PackChunk
{
  uint32_t length;
  uint32_t type;
};

struct PackAssetHeader
{
  uint32_t type;
};

struct PackAssetTag
{
  uint32_t id;
  float value;
};

struct PackImageHeader
{
  uint32_t width;
  uint32_t height;
};

struct PackSoundHeader
{
  uint32_t channels;
};

#pragma pack(pop)
