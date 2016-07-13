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

struct shaderprogram {
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
    use_this_prog();
    GLint attr = glGetAttribLocation(id, name);
    assert(glGetError() == GL_NO_ERROR, "couldn't bind attribute %s", name);
    dont_use_this_prog();
    return attr;
  }
  GLint bind_uniform(const char *name) {
    use_this_prog();
    GLint unif = glGetUniformLocation(id, name);
    assert(glGetError() == GL_NO_ERROR, "failed to bind uniform %s", name);
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

shaderprogram *sp;
GLint vattr;
array_buffer *screenverts;
GLint resolution_unif, time_unif;

void load(screen *s) {
  vertexarray vao;

  const float off = 0.075f;
  std::vector<float> vertices = {
    -1.0f + off,  1.0f - off,
    -1.0f + off, -1.0f + off,
     1.0f - off,  1.0f - off,
     1.0f - off,  1.0f - off,
    -1.0f + off, -1.0f + off,
     1.0f - off, -1.0f + off
  };
  screenverts = new array_buffer;
  screenverts->upload(vertices);

  const char *vsrc = _glsl(
    attribute vec2 position;
    void main() {
      gl_Position = vec4(position, 0.0, 1.0);
    }
  );
  const char *fsrc = _glsl(
    uniform vec2 iResolution;
    uniform float iGlobalTime;

    const bool USE_BRANCHLESS_DDA = false;
    const int MAX_RAY_STEPS = 64;

    float sdSphere(vec3 p, float d) { return length(p) - d; }

    float sdBox( vec3 p, vec3 b )
    {
      vec3 d = abs(p) - b;
      return min(max(d.x,max(d.y,d.z)),0.0) +
      length(max(d,0.0));
    }

    bool getVoxel(ivec3 c) {
      vec3 p = vec3(c) + vec3(0.5);
      float d = min(max(-sdSphere(p, 7.5), sdBox(p, vec3(6.0))), -sdSphere(p, 25.0));
      return d < 0.0;
    }

    vec2 rotate2d(vec2 v, float a) {
      float sinA = sin(a);
      float cosA = cos(a);
      return vec2(v.x * cosA - v.y * sinA, v.y * cosA + v.x * sinA);
    }

    void main()
    {
      vec2 screenPos = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
      vec3 cameraDir = vec3(0.0, 0.0, 0.8);
      vec3 cameraPlaneU = vec3(1.0, 0.0, 0.0);
      vec3 cameraPlaneV = vec3(0.0, 1.0, 0.0) * iResolution.y / iResolution.x;
      vec3 rayDir = cameraDir + screenPos.x * cameraPlaneU + screenPos.y * cameraPlaneV;
      vec3 rayPos = vec3(0.0, 2.0 * sin(iGlobalTime * 2.7), -12.0);

      rayPos.xz = rotate2d(rayPos.xz, iGlobalTime);
      rayDir.xz = rotate2d(rayDir.xz, iGlobalTime);

      ivec3 mapPos = ivec3(floor(rayPos + 0.));

      vec3 deltaDist = abs(vec3(length(rayDir)) / rayDir);

      ivec3 rayStep = ivec3(sign(rayDir));

      vec3 sideDist = (sign(rayDir) * (vec3(mapPos) - rayPos) + (sign(rayDir) * 0.5) + 0.5) * deltaDist;

      bvec3 mask;

      for (int i = 0; i < MAX_RAY_STEPS; i++) {
        if (getVoxel(mapPos)) break;
        if (USE_BRANCHLESS_DDA) {
          //Thanks kzy for the suggestion!
          mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
          /*bvec3 b1 = lessThan(sideDist.xyz, sideDist.yzx);
            bvec3 b2 = lessThanEqual(sideDist.xyz, sideDist.zxy);
            mask.x = b1.x && b2.x;
            mask.y = b1.y && b2.y;
            mask.z = b1.z && b2.z;*/
          //Would've done mask = b1 && b2 but the compiler is making me do it component wise.

          //All components of mask are false except for the corresponding largest component
          //of sideDist, which is the axis along which the ray should be incremented.

          sideDist += vec3(mask) * deltaDist;
          mapPos += ivec3(mask) * rayStep;
        }
        else {
          if (sideDist.x < sideDist.y) {
            if (sideDist.x < sideDist.z) {
              sideDist.x += deltaDist.x;
              mapPos.x += rayStep.x;
              mask = bvec3(true, false, false);
            }
            else {
              sideDist.z += deltaDist.z;
              mapPos.z += rayStep.z;
              mask = bvec3(false, false, true);
            }
          }
          else {
            if (sideDist.y < sideDist.z) {
              sideDist.y += deltaDist.y;
              mapPos.y += rayStep.y;
              mask = bvec3(false, true, false);
            }
            else {
              sideDist.z += deltaDist.z;
              mapPos.z += rayStep.z;
              mask = bvec3(false, false, true);
            }
          }
        }
      }

      vec3 color = vec3(1.0, 0.0, 1.0);
      if (mask.x) {
        color = vec3(0.5);
      }
      if (mask.y) {
        color = vec3(1.0);
      }
      if (mask.z) {
        color = vec3(0.75);
      }
      gl_FragColor = vec4(color, 1.0);
    }
  );

  shader vs(vsrc, GL_VERTEX_SHADER), fs(fsrc, GL_FRAGMENT_SHADER);
  sp = new shaderprogram(vs, fs);

  vattr = sp->bind_attrib("position");
  resolution_unif = sp->bind_uniform("iResolution");

  sp->use_this_prog();
  glUniform2f(resolution_unif, s->window_width, s->window_height);
  sp->dont_use_this_prog();

  time_unif = sp->bind_uniform("iGlobalTime");
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
  sp->use_this_prog();
  glUniform1f(time_unif, (double)t / 1000.);
  sp->dont_use_this_prog();
}

void draw() {
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  sp->use_this_prog();
  screenverts->bind();
  glEnableVertexAttribArray(vattr);
  glVertexAttribPointer(vattr, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(vattr);
  sp->dont_use_this_prog();
}

void cleanup() {
  delete sp;
  delete screenverts;
}

int main() {
  screen s(320, 240);

  s.mainloop(load, update, draw, cleanup);

  return 0;
}

