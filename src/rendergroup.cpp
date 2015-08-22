//
// Handmade Hero - render group
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include "rendergroup.h"
#include "assetpack.h"
#include <iostream>
using namespace std;
using namespace lml;
using namespace HandmadePlatform;


//|---------------------- RenderGroup ---------------------------------------
//|--------------------------------------------------------------------------

///////////////////////// RenderGroup::Constructor //////////////////////////
RenderGroup::RenderGroup(HandmadePlatform::PlatformInterface &platform, AssetManager *assets, allocator_type const &allocator, std::size_t slabsize)
  : m_platform(platform),
    m_buffer(allocator, slabsize)
{
  m_assets = assets;

  m_barrier = m_assets->aquire_barrier();
}


///////////////////////// RenderGroup::Destructor ///////////////////////////
RenderGroup::~RenderGroup()
{
  m_assets->release_barrier(m_barrier);
}


///////////////////////// RenderGroup::projected_scale //////////////////////
float RenderGroup::projected_scale(float z) const
{
  return (1.0f / (1.0 - m_focallength * z));
}


///////////////////////// RenderGroup::projection ///////////////////////////
void RenderGroup::projection(float left, float bottom, float right, float top, float focallength)
{
  auto entry = m_buffer.push<Renderable::Camera>();

  if (entry)
  {
    entry->left = left;
    entry->bottom = bottom;
    entry->right = right;
    entry->top = top;
  }

  m_focallength = focallength;
}


///////////////////////// RenderGroup::clear ////////////////////////////////
void RenderGroup::clear(Color4 const &color)
{
  auto entry = m_buffer.push<Renderable::Clear>();

  if (entry)
  {
    entry->color = color;
  }
}


///////////////////////// RenderGroup::push_rect ////////////////////////////
void RenderGroup::push_rect(Vec3 const &position, Rect2 const &rect, Color4 const &color)
{
  if (position.z > 0.0)
    return;

  auto entry = m_buffer.push<Renderable::Rect>();

  if (entry)
  {
    auto dim = rect.max - rect.min;
    auto align = -rect.min;

    float scale = projected_scale(position.z);

    entry->color = color;

    entry->xaxis = Vec2(scale * dim.x, 0.0f);
    entry->yaxis = Vec2(0.0f, scale * dim.y);
    entry->origin = scale * (position.xy - align);
  }
}


///////////////////////// RenderGroup::push_bitmap //////////////////////////
void RenderGroup::push_bitmap(Vec3 const &position, float size, Asset const *bitmap, Color4 const &color)
{
  if (position.z > 0.0)
    return;

  if (!bitmap)
    return;

  auto bits = m_assets->request(m_platform, bitmap);

  if (bits)
  {
    auto entry = m_buffer.push<Renderable::Bitmap>();

    if (entry)
    {
      auto dim = Vec2(size * bitmap->aspect, size);
      auto align = Vec2(bitmap->alignx * dim.x, bitmap->aligny * dim.y);

      float scale = projected_scale(position.z);

      entry->width = bitmap->width;
      entry->height = bitmap->height;
      entry->bits = bits;

      entry->color = color;

      entry->xaxis = Vec2(scale * dim.x, 0.0f);
      entry->yaxis = Vec2(0.0f, scale * dim.y);
      entry->origin = scale * (position.xy - align);
    }
  }
}


///////////////////////// RenderGroup::push_text ////////////////////////////
void RenderGroup::push_text(Vec3 const &position, float size, Asset const *font, const char *str, Color4 const &color)
{
  if (position.z > 0.0)
    return;

  if (!font)
    return;

  auto table = (PackFontPayload*)m_assets->request(m_platform, font);

  if (table)
  {
    auto glyphtable = reinterpret_cast<AssetType*>(reinterpret_cast<char*>(table) + sizeof(PackFontPayload));
    auto kerningtable = reinterpret_cast<uint8_t*>(reinterpret_cast<char*>(table) + sizeof(PackFontPayload) + table->count * sizeof(uint32_t));

    auto scale = size / (font->ascent + font->descent);

    auto cursor = position;

    uint32_t othercodepoint = 0;

    for(const char *ch = str; *ch; ++ch)
    {
      uint32_t codepoint = *ch;

      if (codepoint < table->count)
      {
        cursor.x += scale * kerningtable[othercodepoint*table->count + codepoint];

        auto glyph = m_assets->find(glyphtable[codepoint]);

        if (glyph)
        {
          push_bitmap(cursor, scale * glyph->height, glyph, color);
        }

        othercodepoint = codepoint;
      }
    }
  }
}


///////////////////////// RenderGroup::render ///////////////////////////////
void render(PlatformInterface &platform, RenderGroup const &rendergroup)
{
  render(platform, rendergroup.m_buffer);
}
