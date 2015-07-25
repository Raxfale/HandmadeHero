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
#include <iostream>

using namespace std;
using namespace HandmadePlatform;


//|---------------------- Platform ------------------------------------------
//|--------------------------------------------------------------------------

class Platform : public PlatformCore
{
  public:


};



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

    PlatformInterface &platform() { return m_platform; }

  private:

    atomic<bool> m_running;

    game_init_t game_init;
    game_reinit_t game_reinit;
    game_update_t game_update;
    game_render_t game_render;

    InputBuffer m_inputbuffer;

    Platform m_platform;

    QLibrary m_game;
};


///////////////////////// Game::Contructor //////////////////////////////////
Game::Game()
{
  m_running = true;
}


///////////////////////// Game::init ////////////////////////////////////////
void Game::init()
{
  QFile::remove("handmade-temp.dll");
  QFile::copy("handmade.dll", "handmade-temp.dll");

  m_game.setFileName("handmade-temp.dll");

  game_init = (game_init_t)m_game.resolve("game_init");
  game_reinit = (game_reinit_t)m_game.resolve("game_reinit");
  game_update = (game_update_t)m_game.resolve("game_update");
  game_render = (game_render_t)m_game.resolve("game_render");

  if (!game_init || !game_update || !game_render)
    throw std::runtime_error("Unable to init game code");

  m_platform.initialise(1*1024*1024*1024);

  game_init(m_platform);
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
}


///////////////////////// Game::terminate ///////////////////////////////////
void Game::terminate()
{
  m_running = false;
}



//|---------------------- Handmade Window -----------------------------------
//|--------------------------------------------------------------------------

class HandmadeWindow : public QOpenGLWindow
{
  public:
    HandmadeWindow(Game *game);

  protected:

    bool event(QEvent *event);

    void initializeGL();

    void resizeGL(int width, int height);

    void paintGL();

  private:

    Game *m_game;
};


///////////////////////// HandmadeWindow::Constructor ///////////////////////
HandmadeWindow::HandmadeWindow(Game *game)
  : m_game(game)
{
  QSurfaceFormat format;

//  format.setSamples(4);
//  format.setDepthBufferSize(24);
//  format.setStencilBufferSize(8);

  setFormat(format);

  resize(960, 540);

  show();
}


///////////////////////// HandmadeWindow::paintGL ///////////////////////////
bool HandmadeWindow::event(QEvent *event)
{
//  qDebug() << event->type();

  if (event->type() == QEvent::KeyRelease)
  {
    cout << static_cast<QKeyEvent*>(event)->key() << endl;
  }

  if (event->type() == QEvent::Close)
    m_game->terminate();

  return QOpenGLWindow::event(event);
}


///////////////////////// HandmadeWindow::initializeGL //////////////////////
void HandmadeWindow::initializeGL()
{
}


///////////////////////// HandmadeWindow::resizeGL //////////////////////////
void HandmadeWindow::resizeGL(int width, int height)
{
}


///////////////////////// HandmadeWindow::paintGL ///////////////////////////
void HandmadeWindow::paintGL()
{
  QOpenGLFunctions *glf = context()->functions();

  glf->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  m_game->render();

  update();
}


//|---------------------- main ----------------------------------------------
//|--------------------------------------------------------------------------

int main(int argc, char **argv)
{
  QGuiApplication app(argc, argv);

  app.setOrganizationName("");
  app.setOrganizationDomain("");
  app.setApplicationName("handmadehero");

  try
  {
    Game game;

    game.init();

    HandmadeWindow window(&game);

#if 0
    thread updatethread([&]() {

      int hz = 30;

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

      while (tick.nsecsElapsed() < dt)
        ;

      tick.restart();

#if 1
      static QDateTime lastmodified = QFileInfo("handmade.dll").lastModified();

      if (QFileInfo("handmade.dll").lastModified() != lastmodified)
      {
        while ((lastmodified = QFileInfo("handmade.dll").lastModified()).addSecs(1) > QDateTime::currentDateTime())
          ;

        game.reinit();
      }
#endif
    }

#endif
  }
  catch(std::exception &e)
  {
    cerr << "Critical Error: " << e.what() << endl;
  }
}
