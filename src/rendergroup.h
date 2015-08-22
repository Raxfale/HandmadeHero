//
// Handmade Hero - render group
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#pragma once

#include "asset.h"
#include "renderer.h"


//|---------------------- RenderGroup ---------------------------------------
//|--------------------------------------------------------------------------

class RenderGroup
{
  public:

    using Vec2 = lml::Vec2;
    using Vec3 = lml::Vec3;
    using Rect2 = lml::Rect2;
    using Color4 = lml::Color4;

    typedef StackAllocator<> allocator_type;

    RenderGroup(HandmadePlatform::PlatformInterface &platform, AssetManager *assets, allocator_type const &allocator, std::size_t slabsize);

    RenderGroup(PushBuffer const &other) = delete;

    ~RenderGroup();

  public:

    void projection(float left, float bottom, float right, float top, float focallength);

    void clear(Color4 const &color = { 0.0f, 0.0f, 0.0f, 1.0f });

    void push_rect(Vec3 const &position, Rect2 const &rect, Color4 const &color = { 1.0f, 1.0f, 1.0f, 1.0f });

    void push_bitmap(Vec3 const &position, float size, Asset const *bitmap, Color4 const &color = { 1.0f, 1.0f, 1.0f, 1.0f });

    void push_text(Vec3 const &position, float size, Asset const *font, const char *str, Color4 const &color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // Render
    friend void render(HandmadePlatform::PlatformInterface &platform, RenderGroup const &rendergroup);

  protected:

    float projected_scale(float z) const;

  private:

    HandmadePlatform::PlatformInterface &m_platform;

    AssetManager *m_assets;

    uintptr_t m_barrier;

    float m_focallength;

    PushBuffer m_buffer;
};


// Render
void render(HandmadePlatform::PlatformInterface &platform, RenderGroup const &rendergroup);
