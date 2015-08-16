//
// Handmade Hero - Platform Core
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include "platformcore.h"
#include <memory>
#include <cstddef>

using namespace std;

namespace
{
  ///////////////////////// map_key_to_button ///////////////////////////////
  HandmadePlatform::GameButton *map_key_to_button(HandmadePlatform::GameController &input, int key)
  {
    switch (key)
    {
      case 0x01000012:
        return &input.move_left;

      case 0x01000013:
        return &input.move_up;

      case 0x01000014:
        return &input.move_right;

      case 0x01000015:
        return &input.move_down;

      default:
        return nullptr;
    }
  }
}


namespace HandmadePlatform
{

  //|---------------------- GameMemory --------------------------------------
  //|------------------------------------------------------------------------

  ///////////////////////// GameMemory::initialise ////////////////////////////
  void gamememory_initialise(GameMemory &pool, void *data, size_t capacity)
  {
    pool.size = 0;
    pool.data = data;
    pool.capacity = capacity;

    std::align(alignof(max_align_t), 0, pool.data, pool.capacity);
  }



  //|---------------------- Input Buffer ------------------------------------
  //|------------------------------------------------------------------------

  ///////////////////////// InputBuffer::Constructor ////////////////////////
  InputBuffer::InputBuffer()
  {
    m_input = {};
  }


  ///////////////////////// InputBuffer::register_mousemove /////////////////
  void InputBuffer::register_mousemove(int x, int y)
  {
    lock_guard<mutex> lock(m_mutex);

    m_events.push_back({ EventType::MouseMoveX, x });
    m_events.push_back({ EventType::MouseMoveY, y });
  }


  ///////////////////////// InputBuffer::register_keydown ///////////////////
  void InputBuffer::register_keydown(int key)
  {
    lock_guard<mutex> lock(m_mutex);

    m_events.push_back({ EventType::KeyDown, key });
  }


  ///////////////////////// InputBuffer::register_keyup /////////////////////
  void InputBuffer::register_keyup(int key)
  {
    lock_guard<mutex> lock(m_mutex);

    m_events.push_back({ EventType::KeyUp, key });
  }


  ///////////////////////// InputBuffer::grab ///////////////////////////////
  GameInput InputBuffer::grab()
  {
    lock_guard<mutex> lock(m_mutex);

    // Keyboard
    m_input.controllers[0].move_up.transitions = 0;
    m_input.controllers[0].move_down.transitions = 0;
    m_input.controllers[0].move_left.transitions = 0;
    m_input.controllers[0].move_right.transitions = 0;

    for(auto &evt : m_events)
    {
      switch(evt.type)
      {
        case EventType::KeyDown:
          {
            auto button = map_key_to_button(m_input.controllers[0], evt.data);

            if (button)
            {
              button->state = true;
              button->transitions += 1;
            }

            break;
          }

        case EventType::KeyUp:
          {
            auto button = map_key_to_button(m_input.controllers[0], evt.data);

            if (button)
            {
              button->state = false;
              button->transitions += 1;
            }

            break;
          }

        case EventType::MouseMoveX:
          m_input.mousex = evt.data;
          break;

        case EventType::MouseMoveY:
          m_input.mousey = evt.data;
          break;

        case EventType::MouseMoveZ:
          m_input.mousez = evt.data;
          break;

        case EventType::MousePress:
          break;

        case EventType::MouseRelease:
          break;
      }
    }

    m_events.clear();

    return m_input;
  }



  //|---------------------- WorkQueue ---------------------------------------
  //|------------------------------------------------------------------------

  ///////////////////////// WorkQueue::Constructor //////////////////////////
  WorkQueue::WorkQueue(int threads)
  {
    m_done = false;

    for(int i = 0; i < threads; ++i)
    {
      m_threads.push_back(std::thread([=]() {

        while (!m_done)
        {
          std::function<void()> work;

          {
            unique_lock<std::mutex> lock(m_mutex);

            while (m_queue.empty())
            {
              m_signal.wait(lock);
            }

            work = std::move(m_queue.front());

            m_queue.pop_front();
          }

          work();
        }

      }));
    }
  }


  ///////////////////////// WorkQueue::Destructor ///////////////////////////
  WorkQueue::~WorkQueue()
  {
    for(size_t i = 0; i < m_threads.size(); ++i)
      push([=]() { m_done = true; });

    for(auto &thread : m_threads)
      thread.join();
  }



  //|---------------------- PlatformCore ------------------------------------
  //|------------------------------------------------------------------------


  ///////////////////////// PlatformCore::Constructor ///////////////////////
  PlatformCore::PlatformCore()
  {
    m_terminaterequested = false;
  }


  ///////////////////////// PlatformCore::initialise ////////////////////////
  void PlatformCore::initialise(std::size_t gamememorysize)
  {
    m_gamememory.reserve(gamememorysize);
    m_gamescratchmemory.reserve(256*1024*1024);
    m_renderscratchmemory.reserve(256*1024*1024);

    gamememory_initialise(gamememory, m_gamememory.data(), m_gamememory.capacity());

    gamememory_initialise(gamescratchmemory, m_gamescratchmemory.data(), m_gamescratchmemory.capacity());

    gamememory_initialise(renderscratchmemory, m_renderscratchmemory.data(), m_renderscratchmemory.capacity());
  }


  ///////////////////////// PlatformCore::read_handle ///////////////////////
  void PlatformCore::read_handle(PlatformInterface::handle_t handle, uint64_t position, void *buffer, size_t n)
  {
    auto file = static_cast<platform_handle_t*>(handle);

    lock_guard<mutex> lock(file->lock);

    file->fio.seekg(position);

    file->fio.read((char*)buffer, n);

    if (!file->fio)
      throw runtime_error("Data Read Error");
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

        handle->fio.open(directory->iterator->string(), ios::in | ios::binary);
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


  ///////////////////////// PlatformCore::submit_work ///////////////////////
  void PlatformCore::submit_work(void (*func)(PlatformInterface &, void*, void*), void *ldata, void *rdata)
  {
    m_workqueue.push([=]() { func(*this, ldata, rdata); });
  }


  ///////////////////////// PlatformCore::terminate /////////////////////////
  void PlatformCore::terminate()
  {
    m_terminaterequested = true;
  }


} // namespace
