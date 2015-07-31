//
// Handmade Hero - platform core
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#pragma once

#include "platform.h"
#include "cxxports.h"
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <deque>
#include <functional>

namespace HandmadePlatform
{

  //|---------------------- Input Buffer --------------------------------------
  //|--------------------------------------------------------------------------

  class InputBuffer
  {
    public:

      enum class EventType
      {
        KeyDown,
        KeyUp,
        MouseMoveX,
        MouseMoveY,
        MouseMoveZ,
        MousePress,
        MouseRelease,
      };

      struct InputEvent
      {
        EventType type;

        int data;
      };

    public:
      InputBuffer();

      void register_mousemove(int x, int y);

      void register_keydown(int key);
      void register_keyup(int key);

    public:

      GameInput grab();

    private:

      GameInput m_input;

      std::vector<InputEvent> m_events;

      mutable std::mutex m_mutex;
  };



  //|---------------------- WorkQueue -----------------------------------------
  //|--------------------------------------------------------------------------

  class WorkQueue
  {
    public:

      WorkQueue(int threads = 4);
      ~WorkQueue();

      template<typename Func>
      void push(Func &&func)
      {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_queue.push_back(std::forward<Func>(func));

        m_signal.notify_one();
      }

    private:

      std::atomic<bool> m_done;

      std::mutex m_mutex;

      std::condition_variable m_signal;

      std::deque<std::function<void()>> m_queue;

      std::vector<std::thread> m_threads;
  };



  //|---------------------- PlatformCore --------------------------------------
  //|--------------------------------------------------------------------------

  class PlatformCore : public PlatformInterface
  {
    public:

      PlatformCore();

      void initialise(size_t gamememorysize);

    public:

      // data access

      struct platform_handle_t
      {
        std::mutex lock;

        std::fstream fio;
      };

      void read_handle(handle_t handle, uint64_t position, void *buffer, std::size_t n) override;

      void close_handle(handle_t handle) override;


      // type enumerator

      struct platform_type_enumerator
      {
        std::string type;
        std::filesystem::directory_iterator iterator;
      };

      type_enumerator open_type_enumerator(const char *type) override;

      handle_t iterate_type_enumerator(type_enumerator enumerator) override;

      void close_type_enumerator(type_enumerator enumerator) override;


      // work queue

      void submit_work(void (*func)(PlatformInterface &, void*, void*), void *ldata, void *rdata) override;


      // misc

      void terminate() override;

    public:

      bool terminate_requested() const { return m_terminaterequested.load(std::memory_order_relaxed); }

    protected:

      std::atomic<bool> m_terminaterequested;

      std::vector<char> m_gamememory;
      std::vector<char> m_gamescratchmemory;
      std::vector<char> m_renderscratchmemory;

      WorkQueue m_workqueue;
  };

} // namespace
