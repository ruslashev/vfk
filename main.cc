#include "screen.hh"
#include "utils.hh"
#include <string>
#include <vector>

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

class array_buffer : public ogl_buffer
{
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
  char msg[1024];
  ogl_errmsg_func(id, 1024, NULL, msg);
  std::string msgstr(msg);
  msgstr.pop_back(); // strip trailing newline
  // indent every line
  int indent = 3;
  msgstr.insert(msgstr.begin(), indent, ' ');
  for (size_t i = 0; i < msgstr.size(); i++) {
    if (msgstr[i] != '\n')
      continue;
    msgstr.insert(i, indent, ' ');
  }
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
         , type == GL_VERTEX_SHADER ? "Vertex" : "Fragment", msg);
    }
  }
  ~shader() {
    glDeleteShader(id);
  }
};

class shaderprogram {
  GLint _model_mat_unif;
  GLint _proj_mat_unif;
  GLint _view_mat_unif;

  void link(const shader &vert, const shader &frag);
public:
  GLuint id;
  shaderprogram(const shader &vert, const shader &frag) {
    assert(vert.type == GL_VERTEX_SHADER && frag.type == GL_FRAGMENT_SHADER
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
    GLint attr = glGetAttribLocation(id, name);
    assert(attr != -1, "couldn't bind attribute %s", name);
    return attr;
  }
  void use_this_prog() {
    glUseProgram(id);
  }
  void dont_use_this_prog() {
    glUseProgram(0);
  }
  // void UpdateMatrices(const glm::mat4 &model,
  //     const glm::mat4 &view, const glm::mat4 &proj);
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

shaderprogram *sp;
GLint vattr;

void load() {
  vertexarray vao;

  // std::vector<float> vertices = {
  //    0.0f,  0.5f,
  //    0.5f, -0.5f,
  //   -0.5f, -0.5f
  // };
  // array_buffer screenverts;
  // screenverts.upload(vertices);
  // screenverts.bind();

  const char *vsrc = _glsl(
    attribute vec2 position;
    void main() {
      gl_Position = vec4(position, 0.0, 1.0);
    }
  );
  const char *fsrc = _glsl(
    void main() {
      gl_FragColor = vec4(1.0, 0.0, 1.0, 1.0);
    }
  );

  shader vs(vsrc, GL_VERTEX_SHADER), fs(fsrc, GL_FRAGMENT_SHADER);
  sp = new shaderprogram(vs, fs);
  sp->use_this_prog();
  vattr = sp->bind_attrib("position");
  // sp->bind_vertexattrib(screenverts, "position", 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void update(double dt, uint32_t t, screen *s) {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_QUIT)
      s->running = false;
    else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      uint8_t *keystates = (uint8_t*)SDL_GetKeyboardState(nullptr);
      int fw = keystates[SDL_SCANCODE_W] - keystates[SDL_SCANCODE_S];
      int side = keystates[SDL_SCANCODE_D] - keystates[SDL_SCANCODE_A];
    }
  }
}

void draw() {
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  float vertices[] = {
     0.0f,  0.5f,
     0.5f, -0.5f,
    -0.5f, -0.5f
  };
  sp->use_this_prog();
  glEnableVertexAttribArray(vattr);
  glVertexAttribPointer(vattr, 2, GL_FLOAT, GL_FALSE, 0, vertices);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDisableVertexAttribArray(vattr);
  sp->dont_use_this_prog();
}

void cleanup() {
  delete sp;
}

int main() {
  screen s(800, 600);

  s.mainloop(load, update, draw, cleanup);

  return 0;
}

