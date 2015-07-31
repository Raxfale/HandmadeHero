//
// Handmade Hero - platform interface
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#pragma once

#include <cstdint>

namespace HandmadePlatform
{
  inline namespace v1
  {

    //|---------------------- GameMemory ----------------------------------------
    //|--------------------------------------------------------------------------

    struct GameMemory
    {
      std::size_t size;
      std::size_t capacity;

      void *data;
    };


    //|---------------------- GameInput -----------------------------------------
    //|--------------------------------------------------------------------------

    struct GameButton
    {
      bool state;
      int transitions;
    };

    struct GameController
    {
      GameButton move_up;
      GameButton move_down;
      GameButton move_left;
      GameButton move_right;

      GameButton action_up;
      GameButton action_down;
      GameButton action_left;
      GameButton action_right;

      GameButton left_shoulder;
      GameButton right_shoulder;

      GameButton back;
      GameButton start;
    };

    struct GameInput
    {
      int mousex, mousey, mousez;

      GameButton mousebuttons[5];

      GameController controllers[5];
    };


    //|---------------------- PlatformInterface ---------------------------------
    //|--------------------------------------------------------------------------

    struct PlatformInterface
    {
      GameMemory gamememory;
      GameMemory gamescratchmemory;
      GameMemory renderscratchmemory;


      // data access

      typedef void *handle_t;

      virtual void read_handle(handle_t handle, uint64_t position, void *buffer, std::size_t n) = 0;

      virtual void close_handle(handle_t handle) = 0;


      // data type enumerator

      typedef void *type_enumerator;

      virtual type_enumerator open_type_enumerator(const char *type) = 0;

      virtual handle_t iterate_type_enumerator(type_enumerator enumerator) = 0;

      virtual void close_type_enumerator(type_enumerator enumerator) = 0;


      // work queue

      virtual void submit_work(void (*func)(PlatformInterface &, void*, void*), void *ldata, void *rdata) = 0;


      // opengl

      virtual void *gl_request_proc(const char *proc) = 0;


      // misc

      virtual void terminate() = 0;
    };
  }
}


// Game Interface

typedef void (*game_init_t)(HandmadePlatform::PlatformInterface &platform);
typedef void (*game_reinit_t)(HandmadePlatform::PlatformInterface &platform);
typedef void (*game_update_t)(HandmadePlatform::PlatformInterface &platform, HandmadePlatform::GameInput const &input, float dt);
typedef void (*game_render_t)(HandmadePlatform::PlatformInterface &platform);

