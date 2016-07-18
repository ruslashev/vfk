#pragma once

#include "utils.hh"
#include <GL/glew.h>
#include <vector>
#include <string>

class ogl_buffer {
protected:
  GLuint _id;
  GLenum _type;
public:
  ogl_buffer(GLenum _type = GL_ARRAY_BUFFER) : _type(_type) {
    glGenBuffers(1, &_id);
  }
  ~ogl_buffer() {
    glDeleteBuffers(1, &_id);
  }
  void bind() const {
    glBindBuffer(_type, _id);
  }
  void unbind() const {
    glBindBuffer(_type, 0);
  }
};

class array_buffer : public ogl_buffer {
public:
  void upload(std::vector<GLfloat> &data) {
    bind();
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(data[0]), data.data(),
        GL_STATIC_DRAW);
    unbind();
  }
};

static const char*
get_ogl_shader_err(void (*ogl_errmsg_func)(GLuint, GLsizei, GLsizei*, GLchar*),
    GLuint id) {
  char *msg;
  GLint loglen;
  glGetShaderiv(id, GL_INFO_LOG_LENGTH, &loglen);
  if (loglen == 0) {
    GLint type;
    glGetShaderiv(id, GL_SHADER_TYPE, &type);
    std::string errmsg = "shader has no error log";
    errmsg.insert(0, type == GL_VERTEX_SHADER ? "vertex" : "fragment");
    return errmsg.c_str();
  }
  msg = new char [loglen + 1];
  ogl_errmsg_func(id, loglen, nullptr, msg);
  std::string msgstr(msg);
  /*
  msgstr.pop_back(); // strip trailing newline
  // indent every line
  int indent = 3;
  msgstr.insert(msgstr.begin(), indent, ' ');
  for (size_t i = 0; i < msgstr.size(); i++) {
    if (msgstr[i] != '\n')
      continue;
    msgstr.insert(i, indent, ' ');
  }
  */
  delete [] msg;
  return msgstr.c_str();
}

struct shader {
  GLuint type;
  GLuint id;
  shader(std::string source, GLuint type) : type(type) {
    id = glCreateShader(type);
    const char *csrc = source.c_str();
    glShaderSource(id, 1, &csrc, NULL);
    glCompileShader(id);
    GLint compilesucc;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compilesucc);
    if (compilesucc != GL_TRUE) {
      const char *msg = get_ogl_shader_err(glGetShaderInfoLog, id);
      die("failed to compile %s shader:\n%s"
         , type == GL_VERTEX_SHADER ? "vertex" : "fragment", msg);
    }
  }
  ~shader() {
    glDeleteShader(id);
  }
};

struct shaderprogram {
  GLuint id;
  shaderprogram(const shader &vert, const shader &frag) {
    assertf(vert.type == GL_VERTEX_SHADER && frag.type == GL_FRAGMENT_SHADER
          , "order of shaders in shaderprogram's constructor is reversed");
    id = glCreateProgram();
    glAttachShader(id, vert.id);
    glAttachShader(id, frag.id);
    glLinkProgram(id);
    GLint linksucc;
    glGetProgramiv(id, GL_LINK_STATUS, &linksucc);
    if (linksucc != GL_TRUE) {
      const char *msg = get_ogl_shader_err(glGetProgramInfoLog, id);
      glDetachShader(id, vert.id);
      glDetachShader(id, frag.id);
      die("failed to link shaders:\n%s", msg);
    }
    use_this_prog();
  }
  ~shaderprogram() {
    glDeleteProgram(id);
  }
  void vertexattribptr(const array_buffer &buffer, const char *name,
      GLint size, GLenum type, GLboolean normalized, GLsizei stride,
      const GLvoid *ptr) {
    buffer.bind();
    GLint attr = glGetAttribLocation(id, name);
    glEnableVertexAttribArray(attr);
    glVertexAttribPointer(attr, size, type, normalized, stride, ptr);
    buffer.unbind();
  }
  GLint bind_attrib(const char *name) {
    use_this_prog();
    GLint attr = glGetAttribLocation(id, name);
    assertf(glGetError() == GL_NO_ERROR, "couldn't bind attribute %s", name);
    dont_use_this_prog();
    return attr;
  }
  GLint bind_uniform(const char *name) {
    use_this_prog();
    GLint unif = glGetUniformLocation(id, name);
    assertf(glGetError() == GL_NO_ERROR, "failed to bind uniform %s", name);
    dont_use_this_prog();
    return unif;
  }
  void use_this_prog() {
    glUseProgram(id);
  }
  void dont_use_this_prog() {
    glUseProgram(0);
  }
};

struct vertexarray {
  GLuint id;
  vertexarray() {
    glGenVertexArrays(1, &id);
    glBindVertexArray(id);
  }
  ~vertexarray() {
    glDeleteVertexArrays(1, &id);
  }
};

