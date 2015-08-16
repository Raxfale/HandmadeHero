//
// Handmade Hero - renderer-gl
//

//
// Copyright (c) 2015 Peter Niekamp
//   following Casey Muratori's Handmade Hero (handmadehero.org)
//

#include "renderer.h"
#include <vector>
#include <iostream>
#include <GL/gl.h>
#include <GL/glext.h>

typedef void (APIENTRYP PFNGLVIEWPORTPROC) (GLint x,GLint y,GLsizei width,GLsizei height);

typedef void (APIENTRYP PFNGLCLEARCOLORPROC) (GLclampf red,GLclampf green,GLclampf blue,GLclampf alpha);
typedef void (APIENTRYP PFNGLCLEARPROC) (GLbitfield mask);

typedef void (APIENTRYP PFNGLGENTEXTURESPROC) (GLsizei n, GLuint *textures);
typedef void (APIENTRYP PFNGLACTIVETEXTURE) (GLsizei n);
typedef void (APIENTRYP PFNGLBINDTEXTUREPROC) (GLenum target, GLuint texture);
typedef void (APIENTRYP PFNGLTEXIMAGE2DPROC) (GLenum target,GLint level,GLint internalformat,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const GLvoid *pixels);
typedef void (APIENTRYP PFNGLTEXPARAMETERIPROC) (GLenum target,GLenum pname,GLint param);
typedef void (APIENTRYP PFNGLDELETETEXTURESPROC) (GLsizei n,const GLuint *textures);

typedef void (APIENTRYP PFNGLDRAWARRAYSPROC) (GLenum mode,GLint first,GLsizei count);

typedef void (APIENTRYP PFNGLBLENDFUNCPROC) (GLenum sfactor,GLenum dfactor);

typedef void (APIENTRYP PFNGLENABLEPROC) (GLenum cap);

typedef void (APIENTRYP PFNGLCULLFACEPROC) (GLenum mode);

using namespace std;
using namespace lml;
using namespace HandmadePlatform;

namespace
{
  const char *vssrc = R"(

    attribute highp vec4 vertex_pos;
    attribute highp vec4 vertex_uv;

    uniform mediump mat4 transform;

    varying mediump vec4 uv;

    void main(void)
    {
      uv = vertex_uv;
      gl_Position = transform * vertex_pos;
    }

  )";

  const char *fssrc = R"(

    uniform sampler2D diffuse;

    varying mediump vec4 uv;

    void main()
    {
      gl_FragColor = texture2D(diffuse, uv.st);
    }

  )";

  struct Transform
  {
    GLuint uniform;

    GLfloat data[4][4];
  };

  void identity(Transform *transform)
  {
    transform->data[0][0] = 1; transform->data[1][0] = 0; transform->data[2][0] = 0; transform->data[3][0] = 0;
    transform->data[0][1] = 0; transform->data[1][1] = 1; transform->data[2][1] = 0; transform->data[3][1] = 0;
    transform->data[0][2] = 0; transform->data[1][2] = 0; transform->data[2][2] = 1; transform->data[3][2] = 0;
    transform->data[0][3] = 0; transform->data[1][3] = 0; transform->data[2][3] = 0; transform->data[3][3] = 1;
  }

  void orthographic(Transform *transform, float left, float bottom, float right, float top, float nearz, float farz)
  {
    identity(transform);

    transform->data[0][0] = 2 / (right - left);
    transform->data[1][1] = 2 / (top - bottom);
    transform->data[2][2] = -2 / (farz - nearz);

    transform->data[3][0] = -(right + left) / (right - left);
    transform->data[3][1] = -(top + bottom) / (top - bottom);
    transform->data[3][2] = -(farz + nearz) / (farz - nearz);
  }

  Transform bybasis(Transform const &transform, Vec2 const &xaxis, Vec2 const &yaxis, Vec2 const &origin)
  {
    Transform result = transform;

    result.data[0][0] = xaxis.x*transform.data[0][0] + xaxis.y*transform.data[1][0];
    result.data[0][1] = xaxis.x*transform.data[0][1] + xaxis.y*transform.data[1][1];
    result.data[0][2] = xaxis.x*transform.data[0][2] + xaxis.y*transform.data[1][2];

    result.data[1][0] = yaxis.x*transform.data[0][0] + yaxis.y*transform.data[1][0];
    result.data[1][1] = yaxis.x*transform.data[0][1] + yaxis.y*transform.data[1][1];
    result.data[1][2] = yaxis.x*transform.data[0][2] + yaxis.y*transform.data[1][2];

    result.data[3][0] = origin.x*transform.data[0][0] + origin.y*transform.data[1][0] + transform.data[3][0];
    result.data[3][1] = origin.x*transform.data[0][1] + origin.y*transform.data[1][1] + transform.data[3][1];
    result.data[3][2] = origin.x*transform.data[0][2] + origin.y*transform.data[1][2] + transform.data[3][2];

    return result;
  }

  void draw_clear(HandmadePlatform::PlatformInterface &platform, Transform const &projection, float r, float g, float b)
  {
    auto glClearColor = (PFNGLCLEARCOLORPROC)platform.gl_request_proc("glClearColor");
    auto glClear = (PFNGLCLEARPROC)platform.gl_request_proc("glClear");

    assert(glClearColor && glClear);

    glClearColor(r, g, b, 1.0);

    glClear(GL_COLOR_BUFFER_BIT);
  }


  void draw_rect(HandmadePlatform::PlatformInterface &platform, Transform const &projection, float r, float g, float b, float a)
  {
    auto transform = bybasis(projection, Vec2(0.25, 0.25), perp(Vec2(0.25, 0.25)), Vec2(-0.5, -0.5));

    auto glGenTextures = (PFNGLGENTEXTURESPROC)platform.gl_request_proc("glGenTextures");
    auto glActiveTexture = (PFNGLACTIVETEXTURE)platform.gl_request_proc("glActiveTexture");
    auto glBindTexture = (PFNGLBINDTEXTUREPROC)platform.gl_request_proc("glBindTexture");
    auto glTexImage2D = (PFNGLTEXIMAGE2DPROC)platform.gl_request_proc("glTexImage2D");
    auto glTexParameteri = (PFNGLTEXPARAMETERIPROC)platform.gl_request_proc("glTexParameteri");
    auto glDeleteTextures = (PFNGLDELETETEXTURESPROC)platform.gl_request_proc("glDeleteTextures");

    assert(glGenTextures && glBindTexture && glTexImage2D && glTexParameteri);

    GLuint texture;

    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    GLbyte texel[4] = { GLbyte(a*b*255), GLbyte(a*g*255), GLbyte(a*r*255), GLbyte(a*255) };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, &texel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    auto glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)platform.gl_request_proc("glUniformMatrix4fv");;

    glUniformMatrix4fv(transform.uniform, 1, GL_FALSE, (GLfloat*)transform.data);

    auto glDrawArrays = (PFNGLDRAWARRAYSPROC)platform.gl_request_proc("glDrawArrays");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDeleteTextures(1, &texture);
  }


  void draw_bitmap(HandmadePlatform::PlatformInterface &platform, Transform const &projection, int width, int height, void const *bits)
  {
    auto transform = bybasis(projection, Vec2(144/15.0, 0), Vec2(0, 217/15.0), Vec2(0.0, 0.0));

    auto glGenTextures = (PFNGLGENTEXTURESPROC)platform.gl_request_proc("glGenTextures");
    auto glActiveTexture = (PFNGLACTIVETEXTURE)platform.gl_request_proc("glActiveTexture");
    auto glBindTexture = (PFNGLBINDTEXTUREPROC)platform.gl_request_proc("glBindTexture");
    auto glTexImage2D = (PFNGLTEXIMAGE2DPROC)platform.gl_request_proc("glTexImage2D");
    auto glTexParameteri = (PFNGLTEXPARAMETERIPROC)platform.gl_request_proc("glTexParameteri");
    auto glDeleteTextures = (PFNGLDELETETEXTURESPROC)platform.gl_request_proc("glDeleteTextures");

    assert(glGenTextures && glBindTexture && glTexImage2D && glTexParameteri);

    GLuint texture;

    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, bits);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    auto glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)platform.gl_request_proc("glUniformMatrix4fv");;

    glUniformMatrix4fv(transform.uniform, 1, GL_FALSE, (GLfloat*)transform.data);

    auto glDrawArrays = (PFNGLDRAWARRAYSPROC)platform.gl_request_proc("glDrawArrays");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDeleteTextures(1, &texture);
  }

}


// Render
void render(HandmadePlatform::PlatformInterface &platform, PushBuffer const &renderables)
{
  // TODO: Obviously, shouldn't be doing this every frame

  GLint ok = 0;

  auto glViewport = (PFNGLVIEWPORTPROC)platform.gl_request_proc("glViewport");

  glViewport(0, 0, 960, 540);

  auto glCreateShader = (PFNGLCREATESHADERPROC)platform.gl_request_proc("glCreateShader");
  auto glShaderSource = (PFNGLSHADERSOURCEPROC)platform.gl_request_proc("glShaderSource");
  auto glCompileShader = (PFNGLCOMPILESHADERPROC)platform.gl_request_proc("glCompileShader");
  auto glGetShaderiv = (PFNGLGETSHADERIVPROC)platform.gl_request_proc("glGetShaderiv");
  auto glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)platform.gl_request_proc("glGetShaderInfoLog");
  auto glDeleteShader = (PFNGLDELETESHADERPROC)platform.gl_request_proc("glDeleteShader");

  assert(glCreateShader && glShaderSource && glCompileShader && glGetShaderiv && glGetShaderInfoLog);

  GLuint vs = glCreateShader(GL_VERTEX_SHADER);

  glShaderSource(vs, 1, &vssrc, 0);

  glCompileShader(vs);

  glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);

  if(ok == GL_FALSE)
  {
    GLint length = 0;
    glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &length);

    std::vector<GLchar, StackAllocator<GLchar>> infolog(length, platform.renderscratchmemory);
    glGetShaderInfoLog(vs, infolog.size(), &length, infolog.data());

    cerr << infolog.data() << endl;

    glDeleteShader(vs);

    throw std::runtime_error("Error Compiling Vertex Shader");
  }

  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(fs, 1, &fssrc, 0);

  glCompileShader(fs);

  glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);

  if(ok == GL_FALSE)
  {
    GLint length = 0;
    glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &length);

    std::vector<GLchar, StackAllocator<GLchar>> infolog(length, platform.renderscratchmemory);
    glGetShaderInfoLog(fs, infolog.size(), &length, infolog.data());

    cerr << infolog.data() << endl;

    glDeleteShader(fs);

    throw std::runtime_error("Error Compiling Fragment Shader");
  }

  auto glCreateProgram = (PFNGLCREATEPROGRAMPROC)platform.gl_request_proc("glCreateProgram");
  auto glAttachShader = (PFNGLATTACHSHADERPROC)platform.gl_request_proc("glAttachShader");
  auto glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)platform.gl_request_proc("glBindAttribLocation");
  auto glLinkProgram = (PFNGLLINKPROGRAMPROC)platform.gl_request_proc("glLinkProgram");
  auto glGetProgramiv = (PFNGLGETPROGRAMIVPROC)platform.gl_request_proc("glGetProgramiv");
  auto glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)platform.gl_request_proc("glGetProgramInfoLog");
  auto glDetachShader = (PFNGLDETACHSHADERPROC)platform.gl_request_proc("glDetachShader");
  auto glDeleteProgram = (PFNGLDELETEPROGRAMPROC)platform.gl_request_proc("glDeleteProgram");
  auto glUseProgram = (PFNGLUSEPROGRAMPROC)platform.gl_request_proc("glUseProgram");

  assert(glCreateProgram && glAttachShader && glBindAttribLocation && glLinkProgram && glGetProgramiv);

  GLuint program = glCreateProgram();

  glAttachShader(program, vs);
  glAttachShader(program, fs);

  glBindAttribLocation(program, 0, "vertex_pos");
  glBindAttribLocation(program, 1, "vertex_uv");

  glLinkProgram(program);

  glGetProgramiv(program, GL_LINK_STATUS, &ok);

  if(ok == GL_FALSE)
  {
    GLint length = 0;
    glGetProgramiv(fs, GL_INFO_LOG_LENGTH, &length);

    std::vector<GLchar, StackAllocator<GLchar>> infolog(length, platform.gamescratchmemory);
    glGetProgramInfoLog(program, infolog.size(), &length, infolog.data());

    cerr << infolog.data() << endl;

    throw std::runtime_error("Error Linking Program");
  }

  glDetachShader(program, vs);
  glDetachShader(program, fs);

  auto glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)platform.gl_request_proc("glGetUniformLocation");
  auto glUniform1i = (PFNGLUNIFORM1IPROC)platform.gl_request_proc("glUniform1i");
  auto glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)platform.gl_request_proc("glUniformMatrix4fv");;

  GLint diffuseuniform = glGetUniformLocation(program, "diffuse");
  GLint transformuniform = glGetUniformLocation(program, "transform");

  glUseProgram(program);

  glUniform1i(diffuseuniform, 0);

  GLfloat verts[4][5] = {
    { -0.5, +0.5, 0.0, 0.0, 0.0 },
    { -0.5, -0.5, 0.0, 0.0, 1.0 },
    { +0.5, +0.5, 0.0, 1.0, 0.0 },
    { +0.5, -0.5, 0.0, 1.0, 1.0 },
  };

  auto glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)platform.gl_request_proc("glGenVertexArrays");
  auto glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)platform.gl_request_proc("glBindVertexArray");
  auto glGenBuffers = (PFNGLGENBUFFERSPROC)platform.gl_request_proc("glGenBuffers");
  auto glBindBuffer = (PFNGLBINDBUFFERPROC)platform.gl_request_proc("glBindBuffer");
  auto glBufferData = (PFNGLBUFFERDATAPROC)platform.gl_request_proc("glBufferData");
  auto glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)platform.gl_request_proc("glVertexAttribPointer");
  auto glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)platform.gl_request_proc("glEnableVertexAttribArray");
  auto glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)platform.gl_request_proc("glDisableVertexAttribArray");
  auto glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)platform.gl_request_proc("glDeleteBuffers");
  auto glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)platform.gl_request_proc("glDeleteVertexArrays");

  assert(glGenVertexArrays && glBindVertexArray && glGenBuffers && glBindBuffer && glBufferData);

  GLuint vao;

  glGenVertexArrays(1, &vao);

  glBindVertexArray(vao);

  GLuint vbo;

  glGenBuffers(1, &vbo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  auto glEnable = (PFNGLENABLEPROC)platform.gl_request_proc("glEnable");

  auto glCullFace = (PFNGLCULLFACEPROC)platform.gl_request_proc("glCullFace");

  glCullFace(GL_BACK);

  glEnable(GL_CULL_FACE);

  auto glBlendFunc = (PFNGLBLENDFUNCPROC)platform.gl_request_proc("glBlendFunc");

  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glEnable(GL_BLEND);

  //
  // Renderables
  //

  Transform projection;

  projection.uniform = transformuniform;

  identity(&projection);

  glUniformMatrix4fv(projection.uniform, 1, GL_FALSE, (GLfloat*)projection.data);

  for(auto &renderable : renderables)
  {
    switch (renderable.type)
    {
      case Renderable::Type::Camera:
        {
          auto data = renderable_cast<Renderable::Camera>(&renderable);

          orthographic(&projection, -0.5*data->width, -0.5*data->height, 0.5*data->width, 0.5*data->height, 0.0f, 15.0f);

          break;
        }

      case Renderable::Type::Clear:
        {
          auto data = renderable_cast<Renderable::Clear>(&renderable);

          draw_clear(platform, projection, data->color.r, data->color.g, data->color.b);

          break;
        }

      case Renderable::Type::Rect:
        {
          auto data = renderable_cast<Renderable::Rect>(&renderable);

          draw_rect(platform, projection, data->color.r, data->color.g, data->color.b, data->color.a);

          break;
        }

      case Renderable::Type::Bitmap:
        {
          auto data = renderable_cast<Renderable::Bitmap>(&renderable);

          draw_bitmap(platform, projection, data->width, data->height, data->bits);

          break;
        }
    }
  }

  // TODO: Again, not every frame
  glUseProgram(0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDeleteProgram(program);
  glDeleteShader(vs);
  glDeleteShader(fs);
  glDeleteBuffers(1, &vbo);
  glDeleteVertexArrays(1, &vao);
}
