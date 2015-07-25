//
// Arena Memory
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#pragma once

#include "platform.h"
#include "cxxports.h"
#include <memory>
#include <scoped_allocator>


//|---------------------- StackAllocator ------------------------------------
//|--------------------------------------------------------------------------

template<typename T = void*, std::size_t alignment = alignof(T)>
class StackAllocator
{
  public:

    typedef T value_type;

    template<typename U>
    struct rebind
    {
      typedef StackAllocator<U> other;
    };

  public:

    StackAllocator(HandmadePlatform::GameMemory &arena);

    template<typename U, std::size_t ulignment>
    StackAllocator(StackAllocator<U, ulignment> const &other);

    HandmadePlatform::GameMemory *arena() const { return m_arena; }

    T *allocate(std::size_t n);

    void deallocate(T * const ptr, std::size_t n);

    template<typename, std::size_t> friend class StackAllocator;

  private:

    HandmadePlatform::GameMemory *m_arena;
};


///////////////////////// StackAllocator::Constructor ///////////////////////
template<typename T, std::size_t alignment>
StackAllocator<T, alignment>::StackAllocator(HandmadePlatform::GameMemory &arena)
  : m_arena(&arena)
{
}


///////////////////////// StackAllocator::rebind ////////////////////////////
template<typename T, std::size_t alignment>
template<typename U, std::size_t ulignment>
StackAllocator<T, alignment>::StackAllocator(StackAllocator<U, ulignment> const &other)
  : StackAllocator(*other.arena())
{
}


///////////////////////// StackAllocator::allocate //////////////////////////
template<typename T, std::size_t alignment>
T *StackAllocator<T, alignment>::allocate(std::size_t n)
{
  std::size_t size = n * sizeof(T);

  if (m_arena->capacity < m_arena->size + alignment + size)
    throw std::bad_alloc();

  void *result = static_cast<char*>(m_arena->data) + m_arena->size;

  std::align(alignment, size, result, m_arena->capacity);

  m_arena->size = static_cast<char*>(result) + size - static_cast<char*>(m_arena->data);
  m_arena->capacity -= size;

  return static_cast<T*>(result);
}


///////////////////////// StackAllocator::deallocate ////////////////////////
template<typename T, std::size_t alignment>
void StackAllocator<T, alignment>::deallocate(T * const ptr, std::size_t n)
{
}


///////////////////////// StackAllocator::operator == ///////////////////////
template<typename T, std::size_t alignment, typename U, std::size_t ulignment>
bool operator ==(StackAllocator<T, alignment> const &lhs, StackAllocator<U, ulignment> const &rhs)
{
  return lhs.arena() == rhs.arena();
}


///////////////////////// StackAllocator::operator != ///////////////////////
template<typename T, std::size_t alignment, typename U, std::size_t ulignment>
bool operator !=(StackAllocator<T, alignment> const &lhs, StackAllocator<U, ulignment> const &rhs)
{
  return !(lhs == rhs);
}



//|---------------------- StackAllocatorWithFreelist ------------------------
//|--------------------------------------------------------------------------

template<typename T, std::size_t alignment = alignof(T)>
class StackAllocatorWithFreelist : public StackAllocator<T, alignment>
{
  public:

    typedef T value_type;

    template<typename U>
    struct rebind
    {
      typedef StackAllocatorWithFreelist<U> other;
    };

  public:

    StackAllocatorWithFreelist(HandmadePlatform::GameMemory &arena);

    template<typename U, std::size_t ulignment>
    StackAllocatorWithFreelist(StackAllocatorWithFreelist<U, ulignment> const &other);

    T *allocate(std::size_t n);

    void deallocate(T * const ptr, std::size_t n);

    template<typename, std::size_t> friend class StackAllocatorWithFreelist;

  private:

    template<typename U, std::size_t ulignment = alignof(U)>
    U *aligned(void *ptr)
    {
      return reinterpret_cast<U*>((reinterpret_cast<std::size_t>(ptr) + ulignment - 1) & -ulignment);
    }

    struct Node
    {
      std::size_t n;

      T *next;
    };

    T *m_freelist;
};


///////////////////////// StackAllocatorWithFreelist::Constructor ///////////
template<typename T, std::size_t alignment>
StackAllocatorWithFreelist<T, alignment>::StackAllocatorWithFreelist(HandmadePlatform::GameMemory &arena)
  : StackAllocator<T, alignment>(arena)
{
  m_freelist = nullptr;
}


///////////////////////// StackAllocatorWithFreelist::rebind ////////////////
template<typename T, std::size_t alignment>
template<typename U, std::size_t ulignment>
StackAllocatorWithFreelist<T, alignment>::StackAllocatorWithFreelist(StackAllocatorWithFreelist<U, ulignment> const &other)
  : StackAllocatorWithFreelist(*other.arena())
{
}


///////////////////////// StackAllocatorWithFreelist::allocate //////////////
template<typename T, std::size_t alignment>
T *StackAllocatorWithFreelist<T, alignment>::allocate(std::size_t n)
{
  T *entry = m_freelist;
  T **into = &m_freelist;

  while (entry != nullptr)
  {
    Node *node = aligned<Node>(entry);

    if (n <= node->n)
    {
      *into = node->next;

      return entry;
    }

    into = &node->next;
    entry = node->next;
  }

  return StackAllocator<T, alignment>::allocate(n);
}


///////////////////////// StackAllocatorWithFreelist::deallocate ////////////
template<typename T, std::size_t alignment>
void StackAllocatorWithFreelist<T, alignment>::deallocate(T * const ptr, std::size_t n)
{
  Node *node = aligned<Node>(ptr);

  node->n = n;
  node->next = m_freelist;

  m_freelist = ptr;
}



//|---------------------- misc routines -------------------------------------
//|--------------------------------------------------------------------------


///////////////////////// inarena ///////////////////////////////////////////
template<typename T>
bool inarena(HandmadePlatform::GameMemory &arena, T *ptr)
{
  return ptr >= arena.data && ptr < (void*)((char*)arena.data + arena.size);
}


///////////////////////// allocate //////////////////////////////////////////
template<typename T>
T *allocate(StackAllocator<T> allocator, std::size_t n = 1)
{
  return allocator.allocate(n);
}


///////////////////////// mark //////////////////////////////////////////////
inline size_t mark(HandmadePlatform::GameMemory &arena)
{
  return arena.size;
}


///////////////////////// rewind ////////////////////////////////////////////
inline void rewind(HandmadePlatform::GameMemory &arena, size_t mark)
{
  arena.size = mark;
}

