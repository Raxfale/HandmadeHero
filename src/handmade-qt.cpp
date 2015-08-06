//
// Handmade Hero - qt platform layer
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include "platform.h"
#include "platformcore.h"
#include <QGuiApplication>
#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QLibrary>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDateTime>
#include <QThread>
#include <iostream>
#include <chrono>

using namespace std;
using namespace std::literals;
using namespace HandmadePlatform;

#ifdef _WIN32
  const char *libhandmade = "handmade.dll";
#else
  const char *libhandmade = "./libhandmade.so";
#endif


//|---------------------- Platform ------------------------------------------
//|--------------------------------------------------------------------------

class Platform : public PlatformCore
{
  public:

    // opengl

    void *gl_request_proc(const char *proc) override;

};



///////////////////////// Platform::gl_request_proc /////////////////////////
void *Platform::gl_request_proc(const char *proc)
{
  return (void*)QOpenGLContext::currentContext()->getProcAddress(proc);
}



//|---------------------- Game ----------------------------------------------
//|--------------------------------------------------------------------------

class Game
{
  public:

    Game();

    void init();

    void reinit();

    void update(float dt);

    void render();

    void terminate();

  public:

    bool running() { return m_running.load(std::memory_order_relaxed); }

    InputBuffer &inputbuffer() { return m_inputbuffer; }

    Platform &platform() { return m_platform; }

  private:

    atomic<bool> m_running;

    game_init_t game_init;
    game_reinit_t game_reinit;
    game_update_t game_update;
    game_render_t game_render;

    InputBuffer m_inputbuffer;

    Platform m_platform;

    QLibrary m_game;

    int m_fpscount;
    QTime m_fpstimer;
};


///////////////////////// Game::Contructor //////////////////////////////////
Game::Game()
{
  m_running = false;

  m_fpscount = 0;
  m_fpstimer.start();
}


///////////////////////// Game::init ////////////////////////////////////////
void Game::init()
{
#ifdef _WIN32

  QFile::remove("handmade-temp.dll");
  QFile::copy(libhandmade, "handmade-temp.dll");

  m_game.setFileName("handmade-temp.dll");

#else

  m_game.setFileName(libhandmade);

#endif

  game_init = (game_init_t)m_game.resolve("game_init");
  game_reinit = (game_reinit_t)m_game.resolve("game_reinit");
  game_update = (game_update_t)m_game.resolve("game_update");
  game_render = (game_render_t)m_game.resolve("game_render");

  if (!game_init || !game_update || !game_render)
    throw std::runtime_error("Unable to init game code");

  m_platform.initialise(1*1024*1024*1024);

  game_init(m_platform);

  m_running = true;
}


///////////////////////// Game::reinit //////////////////////////////////////
void Game::reinit()
{
  m_game.unload();

  QFile::remove("handmade-temp.dll");
  QFile::copy("handmade.dll", "handmade-temp.dll");

  game_init = (game_init_t)m_game.resolve("game_init");
  game_reinit = (game_reinit_t)m_game.resolve("game_reinit");
  game_update = (game_update_t)m_game.resolve("game_update");
  game_render = (game_render_t)m_game.resolve("game_render");

  if (!game_init || !game_update || !game_render)
    throw std::runtime_error("Unable to reinit game code");

  if (game_reinit)
  {
    m_platform.gamescratchmemory.size = 0;
    m_platform.renderscratchmemory.size = 0;

    game_reinit(m_platform);
  }
}


///////////////////////// Game::update //////////////////////////////////////
void Game::update(float dt)
{
  GameInput input = m_inputbuffer.grab();

  m_platform.gamescratchmemory.size = 0;

  game_update(m_platform, input, dt);

  if (m_platform.terminate_requested())
    terminate();
}


///////////////////////// Game::render //////////////////////////////////////
void Game::render()
{
  m_platform.renderscratchmemory.size = 0;

  game_render(m_platform);

  ++m_fpscount;

  if (m_fpstimer.elapsed() > 1000)
  {
    cout << m_fpscount * 1000.0 / m_fpstimer.elapsed() << "fps" << endl;

    m_fpscount = 0;
    m_fpstimer.restart();
  }

}


///////////////////////// Game::terminate ///////////////////////////////////
void Game::terminate()
{
  m_running = false;
}



//|---------------------- Handmade Window -----------------------------------
//|--------------------------------------------------------------------------

class HandmadeWindow : public QWindow
{
  public:
    HandmadeWindow(Game *game);

  protected:

    bool event(QEvent *event);

  private:

    Game *m_game;
};


///////////////////////// HandmadeWindow::Constructor ///////////////////////
HandmadeWindow::HandmadeWindow(Game *game)
  : m_game(game)
{
  setSurfaceType(QWindow::OpenGLSurface);

  resize(960, 540);

  create();

  show();
}


///////////////////////// HandmadeWindow::paintGL ///////////////////////////
bool HandmadeWindow::event(QEvent *event)
{
//  qDebug() << event->type();

  switch (event->type())
  {
    case QEvent::MouseMove:
      {
        auto mouseevent = static_cast<QMouseEvent*>(event);

        m_game->inputbuffer().register_mousemove(mouseevent->pos().x(), mouseevent->pos().y());

        break;
      }

    case QEvent::KeyPress:
      {
        auto keyevent = static_cast<QKeyEvent*>(event);

        if (!keyevent->isAutoRepeat())
          m_game->inputbuffer().register_keydown(keyevent->key());

        break;
      }

    case QEvent::KeyRelease:
      {
        auto keyevent = static_cast<QKeyEvent*>(event);

        if (!keyevent->isAutoRepeat())
          m_game->inputbuffer().register_keyup(keyevent->key());

        break;
      }

    default:
      break;
  }

  if (event->type() == QEvent::Close)
    m_game->terminate();

  return QWindow::event(event);
}




//|---------------------- main ----------------------------------------------
//|--------------------------------------------------------------------------

int main(int argc, char **argv)
{
  QGuiApplication app(argc, argv);

  app.setOrganizationName("");
  app.setOrganizationDomain("");
  app.setApplicationName("handmadehero");

  QSurfaceFormat format;

//  format.setSamples(4);
//  format.setDepthBufferSize(24);
//  format.setStencilBufferSize(8);

  QSurfaceFormat::setDefaultFormat(format);

  QOpenGLContext context;

  context.create();

  try
  {
    Game game;

    HandmadeWindow window(&game);

    context.makeCurrent(&window);

    game.init();

#if 0
    thread updatethread([&]() {

      int hz = 60;

      qint64 dt = chrono::nanoseconds(1s).count() / hz;

      QElapsedTimer tick;

      tick.start();

      while (game.running())
      {
        game.update(1.0f/hz);

        while (tick.nsecsElapsed() < dt)
          ;

        tick.restart();
      }
    });

    while (game.running())
    {
      app.processEvents();

      game.render();

      context.swapBuffers(&window);
    }

    updatethread.join();

#else

    int hz = 30;

    qint64 dt = chrono::nanoseconds(1s).count() / hz;

    QElapsedTimer tick;

    tick.start();

    while (game.running())
    {
      app.processEvents();

      game.update(1.0f/hz);

      game.render();

      context.swapBuffers(&window);

      while (tick.nsecsElapsed() < dt)
        ;

      tick.restart();

#if 1
      static QDateTime lastmodified = QFileInfo(libhandmade).lastModified();

      if (QFileInfo(libhandmade).lastModified() != lastmodified)
      {
        while ((lastmodified = QFileInfo(libhandmade).lastModified()).addSecs(2) > QDateTime::currentDateTime())
          ;

        game.reinit();
      }
#endif
    }

#endif

    return window.isVisible() ? app.exec() : 0;
  }
  catch(std::exception &e)
  {
    cerr << "Critical Error: " << e.what() << endl;
  }
}
