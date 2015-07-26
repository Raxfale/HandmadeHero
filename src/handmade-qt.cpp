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
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QLibrary>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDateTime>
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

    void render(uint32_t *bits);

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
void Game::render(uint32_t *bits)
{
  m_platform.renderscratchmemory.size = 0;

  game_render(m_platform, bits);
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

    QImage *m_buffer;

    QOpenGLTexture *m_texture;
    QOpenGLShaderProgram *m_program;
    QOpenGLBuffer *m_vertexbuffer;
    QOpenGLVertexArrayObject *m_vertexarrayobject;
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

  m_buffer = new QImage(960, 540, QImage::Format_ARGB32);

  resize(m_buffer->width(), m_buffer->height());

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
  const char *vs =
     "attribute highp vec4 vertex_pos;\n"
     "attribute mediump vec4 vertex_uv;\n"
     "varying mediump vec4 uv;\n"
     "uniform mediump mat4 world;\n"
     "void main(void)\n"
     "{\n"
     "  uv = vertex_uv;\n"
     "  gl_Position = world * vertex_pos;\n"
     "}\n";

  const char *fs =
     "uniform sampler2D texture;\n"
     "varying mediump vec4 uv;\n"
     "void main(void)\n"
     "{\n"
     "  gl_FragColor = texture2D(texture, uv.st);\n"
     "}\n";

  m_program = new QOpenGLShaderProgram;
  m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vs);
  m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fs);
  m_program->link();

  m_vertexbuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);

  GLfloat verts[4][5] = {
    { -1.0, -1.0, -1.0, 0.0, 0.0 },
    { -1.0, +1.0, -1.0, 0.0, 1.0 },
    { +1.0, -1.0, -1.0, 1.0, 0.0 },
    { +1.0, +1.0, -1.0, 1.0, 1.0 },
  };

  m_vertexbuffer->create();
  m_vertexbuffer->setUsagePattern(QOpenGLBuffer::StaticDraw);
  m_vertexbuffer->bind();
  m_vertexbuffer->allocate(verts, sizeof(verts));
  m_vertexbuffer->release();

  m_vertexarrayobject = new QOpenGLVertexArrayObject;
  m_vertexarrayobject->bind();
  m_program->bind();

  m_vertexbuffer->bind();
  m_program->enableAttributeArray("vertex_pos");
  m_program->setAttributeBuffer("vertex_pos", GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
  m_program->enableAttributeArray("vertex_uv");
  m_program->setAttributeBuffer("vertex_uv", GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));
  m_vertexbuffer->release();

  m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);

  m_texture->setSize(m_buffer->width(), m_buffer->height());

  m_texture->setData(*m_buffer);

  resizeGL(width(), height());
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

  m_game->render((uint32_t*)m_buffer->bits());

  m_texture->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, m_buffer->bits());

  QMatrix4x4 world;
  world.ortho(-1.0f, +1.0f, +1.0f, -1.0f, 4.0f, 15.0f);
  world.translate(0.0f, 0.0f, -10.0f);

  m_program->bind();
  m_program->setUniformValue("texture", 0);
  m_program->setUniformValue("world", world);

  m_vertexarrayobject->bind();

  m_texture->bind();

  glf->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

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
  }
  catch(std::exception &e)
  {
    cerr << "Critical Error: " << e.what() << endl;
  }
}
