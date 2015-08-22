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
  float alignx;
  float aligny;
  uint64_t dataoffset;
};

struct PackSoundHeader
{
  uint32_t channels;
  uint64_t dataoffset;
};

struct PackFontHeader
{
  uint32_t ascent;
  uint32_t descent;
  uint32_t leading;
  uint32_t datasize;
  uint64_t dataoffset;
};

struct PackFontPayload
{
  uint32_t count;
  // uint32_t type[count];
  // uint8_t kerning[count][count];
};

#pragma pack(pop)
