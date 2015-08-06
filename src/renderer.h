//
// Handmade Hero - renderer
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#pragma once

#include "platform.h"
#include "memory.h"
#include "lml.h"


// Renderables
namespace Renderable
{
  using Vec3 = lml::Vec3;
  using Color4 = lml::Color4;

  enum class Type : uint16_t
  {
    Camera,
    Clear,
    Rect,
    Bitmap,
  };

  struct Camera
  {
    static const Type type = Type::Camera;

    float width;
    float height;
  };

  struct Clear
  {
    static const Type type = Type::Clear;

    Color4 color;
  };

  struct Rect
  {
    static const Type type = Type::Rect;

    Color4 color;
  };

  struct Bitmap
  {
    static const Type type = Type::Bitmap;

    int width;
    int height;
    void const *bits;
  };
}


//|---------------------- PushBuffer ----------------------------------------
//|--------------------------------------------------------------------------

class PushBuffer
{
  public:

    struct Header
    {
      Renderable::Type type;
      uint16_t size;
    };

    class const_iterator : public std::iterator<std::forward_iterator_tag, Header>
    {
      public:
        explicit const_iterator(Header const *position) : m_header(position) { }

        bool operator ==(const_iterator const &that) const { return m_header == that.m_header; }
        bool operator !=(const_iterator const &that) const { return m_header != that.m_header; }

        Header const &operator *() { return *m_header; }
        Header const *operator ->() { return &*m_header; }

        iterator &operator++()
        {
          m_header = reinterpret_cast<Header const *>(reinterpret_cast<char const *>(m_header) + m_header->size);

          return *this;
        }

      private:

        Header const *m_header;
    };

  public:

    typedef StackAllocator<> allocator_type;

    PushBuffer(allocator_type const &allocator, std::size_t slabsize);

    PushBuffer(PushBuffer const &other) = delete;

  public:

    // Add a renderable to the push buffer
    template<typename T>
    T *push()
    {
      return reinterpret_cast<T*>(push(T::type, sizeof(T), alignof(T)));
    }

    // Iterate a pushbuffer
    const_iterator begin() const { return const_iterator(reinterpret_cast<Header*>(m_slab)); }
    const_iterator end() const { return const_iterator(reinterpret_cast<Header*>(m_tail)); }

  protected:

    void *push(Renderable::Type type, std::size_t size, std::size_t alignment);

  private:

    std::size_t m_slabsize;

    void *m_slab;
    void *m_tail;
};

template<typename T>
T const *renderable_cast(PushBuffer::Header const *header)
{
  assert(T::type == header->type);

  return reinterpret_cast<T const *>((reinterpret_cast<std::size_t>(header + 1) + alignof(T) - 1) & -alignof(T));
}


// Render
void render(HandmadePlatform::PlatformInterface &platform, PushBuffer const &renderables);
