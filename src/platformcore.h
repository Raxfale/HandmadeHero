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
#include <vector>

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
        MouseMove,
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

//      void register_keydown();
//      void register_keyup();

    public:

      GameInput grab();

    private:

      std::vector<InputEvent> m_events;

      mutable std::mutex m_mutex;
  };


  //|---------------------- PlatformCore --------------------------------------
  //|--------------------------------------------------------------------------

  class PlatformCore : public PlatformInterface
  {
    public:

      void init(size_t gamememorysize);

    public:

      // data access

      struct platform_handle_t
      {
        std::mutex lock;

        std::string path;

        std::ifstream fio;
      };

      void open_handle(handle_t handle) override;

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


      // misc

      void terminate() override;

    public:

      bool terminate_requested() const { return m_terminaterequested.load(std::memory_order_relaxed); }

    protected:

      std::atomic<bool> m_terminaterequested;

      std::vector<char> m_gamememory;
      std::vector<char> m_gamescratchmemory;
      std::vector<char> m_renderscratchmemory;
  };

} // namespace
