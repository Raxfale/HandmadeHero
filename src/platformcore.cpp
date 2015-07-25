//
// Handmade Hero - Platform Core
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include "platformcore.h"
#include <memory>

using namespace std;
using namespace std::literals;

namespace HandmadePlatform
{

  //|---------------------- GameMemory ----------------------------------------
  //|--------------------------------------------------------------------------

  ///////////////////////// GameMemory::init //////////////////////////////////
  void gamememory_init(GameMemory &pool, void *data, size_t capacity)
  {
    pool.size = 0;
    pool.data = data;
    pool.capacity = capacity;

    std::align(16, 0, pool.data, pool.capacity);
  }



  //|---------------------- Input Buffer --------------------------------------
  //|--------------------------------------------------------------------------

  ///////////////////////// InputBuffer::Constructor //////////////////////////
  InputBuffer::InputBuffer()
  {
  }


  ///////////////////////// InputBuffer::grab /////////////////////////////////
  GameInput InputBuffer::grab()
  {
    GameInput input;

    input.terminaterequest = false;

    lock_guard<mutex> M(m_mutex);

    for(auto &evt : m_events)
    {
      switch(evt.type)
      {
        case EventType::KeyDown:
          break;

        case EventType::KeyUp:
          break;

        case EventType::MouseMove:
          break;

        case EventType::MousePress:
          break;

        case EventType::MouseRelease:
          break;
      }
    }

    m_events.clear();

    return input;
  }



  //|---------------------- PlatformCore --------------------------------------
  //|--------------------------------------------------------------------------


  ///////////////////////// PlatformCore::init ////////////////////////////////
  void PlatformCore::init(std::size_t gamememorysize)
  {
    m_terminaterequested = false;

    m_gamememory.resize(gamememorysize);
    m_gamescratchmemory.resize(256*1024*1024);
    m_renderscratchmemory.resize(256*1024*1024);

    gamememory_init(gamememory, m_gamememory.data(), m_gamememory.size());

    gamememory_init(gamescratchmemory, m_gamescratchmemory.data(), m_gamescratchmemory.size());

    gamememory_init(renderscratchmemory, m_renderscratchmemory.data(), m_renderscratchmemory.size());
  }


  ///////////////////////// PlatformCore::open_handle ///////////////////////
  void PlatformCore::open_handle(PlatformInterface::handle_t handle)
  {
    auto file = static_cast<platform_handle_t*>(handle);

    file->fio.open(file->path, ios::binary);

    if (!file->fio)
      throw runtime_error("File Open Error on " + file->path);
  }


  ///////////////////////// PlatformCore::read_handle ///////////////////////
  void PlatformCore::read_handle(PlatformInterface::handle_t handle, uint64_t position, void *buffer, size_t n)
  {
    auto file = static_cast<platform_handle_t*>(handle);

    lock_guard<mutex> M(file->lock);

    file->fio.seekg(position);

    file->fio.read((char*)buffer, n);

    if (!file->fio)
      throw runtime_error("File Read Error on " + file->path);
  }


  ///////////////////////// PlatformCore::close_handle //////////////////////
  void PlatformCore::close_handle(PlatformInterface::handle_t handle)
  {
    delete static_cast<platform_handle_t*>(handle);
  }


  ///////////////////////// PlatformCore::open_type_enumerator //////////////
  PlatformInterface::type_enumerator PlatformCore::open_type_enumerator(const char *type)
  {
    auto directory = new platform_type_enumerator;

    directory->type = string(".") + string(type);
    directory->iterator = filesystem::directory_iterator(".");

    return directory;
  }


  ///////////////////////// PlatformCore::iterate_type_enumerator ///////////
  PlatformInterface::handle_t PlatformCore::iterate_type_enumerator(PlatformInterface::type_enumerator enumerator)
  {
    auto directory = static_cast<platform_type_enumerator*>(enumerator);

    platform_handle_t *handle = nullptr;

    while (!handle && directory->iterator != filesystem::directory_iterator())
    {
      if (directory->iterator->extention() == directory->type)
      {
        handle = new platform_handle_t;

        handle->path = directory->iterator->string();
      }

      ++directory->iterator;
    }

    return handle;
  }


  ///////////////////////// PlatformCore::close_type_enumerator /////////////
  void PlatformCore::close_type_enumerator(PlatformInterface::type_enumerator enumerator)
  {
    delete static_cast<platform_type_enumerator*>(enumerator);
  }


  ///////////////////////// PlatformCore::terminate ///////////////////////////
  void PlatformCore::terminate()
  {
    m_terminaterequested = true;
  }

} // namespace
