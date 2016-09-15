#include "screen.hh"
#include "utils.hh"
#include "ogl.hh"
#include "world.hh"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

shaderprogram *sp;
GLint vattr;
array_buffer *screenverts;
GLint resolution_unif, time_unif;
shader *vs, *fs;
world *w;

void load(screen *s) {
  int vertex_texture_units;
  glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &vertex_texture_units);
  assertf(vertex_texture_units,
      "unfortunately, your graphic has 0 texture units availiable and does not "
      "support\ntexure lookups in the vertex shader");

  // vertexarray vao;

  glClearColor(0.85f, 0.f, 1.f, 1);

  std::vector<float> vertices = {
    -1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f,  1.0f,
     1.0f,  1.0f,
    -1.0f, -1.0f,
     1.0f, -1.0f
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
    // uniform vec3 viewOrigin;
    // uniform mat4 invProjView;
    uniform sampler3D world_data;
    uniform int w;
    uniform int h;
    uniform int d;

    const bool USE_BRANCHLESS_DDA = false;
    const int MAX_RAY_STEPS = 128;

    float noise(float x) { return fract(sin(x * 113.0) * 43758.5453123); }

    float sdSphere(vec3 p, float d) {
      return length(p) - d;
    }

    float sdBox(vec3 p, vec3 b) {
      vec3 d = abs(p) - b;
      return min(max(d.x,max(d.y,d.z)),0.0) +
      length(max(d,0.0));
    }

    bool getVoxel(ivec3 c) {
      // return noise(c.x) > 0.5;
      // return texture3D(world_data, c).r > 0.5;
      vec3 s = vec3(c) + vec3(0.5);
      // float d = min(max(-sdSphere(p, 7.5), sdBox(p, vec3(6.0))), -sdSphere(p, 25.0));
      // return d < 0.0;
      return distance(s, vec3(0.0)) > 30.0 ? texture3D(world_data, s).r < 0.5 : false;
    }

    vec2 rotate2d(vec2 v, float a) {
      float sinA = sin(a);
      float cosA = cos(a);
      return vec2(v.x * cosA - v.y * sinA, v.y * cosA + v.x * sinA);
    }

    void main() {
      vec2 screenPos = (gl_FragCoord.xy / iResolution.xy) * 2.0 - 1.0;
      vec3 cameraDir = vec3(0.0, 0.0, 0.8);
      vec3 cameraPlaneU = vec3(1.0, 0.0, 0.0);
      vec3 cameraPlaneV = vec3(0.0, 1.0, 0.0) * iResolution.y / iResolution.x;
      vec3 rayDir = cameraDir + screenPos.x * cameraPlaneU + screenPos.y * cameraPlaneV;
      vec3 rayPos = vec3(0.0, 2.0 * sin(iGlobalTime * 2.7), -12.0);

      rayPos.xz = rotate2d(rayPos.xz, iGlobalTime);
      rayDir.xz = rotate2d(rayDir.xz, iGlobalTime);

      ivec3 mapPos = ivec3(floor(rayPos + 0.));

      vec3 deltaDist = abs(vec3(1) / rayDir);

      ivec3 rayStep = ivec3(sign(rayDir));

      vec3 sideDist = (sign(rayDir) * (vec3(mapPos) - rayPos) + sign(rayDir) * 0.5 + 0.5) * deltaDist;

      bvec3 mask;

      for (int i = 0; i < MAX_RAY_STEPS; i++) {
        if (getVoxel(mapPos))
          break;
        if (USE_BRANCHLESS_DDA) {
          mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
          sideDist += vec3(mask) * deltaDist;
          mapPos += ivec3(mask) * rayStep;
        } else {
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
      // gl_FragColor = vec4(texture3D(data, vec3(0,0,0)).rgb, 1.0);
    }
  );

  vs = new shader(vsrc, GL_VERTEX_SHADER);
  fs = new shader(fsrc, GL_FRAGMENT_SHADER);
  sp = new shaderprogram(*vs, *fs);

  vattr = sp->bind_attrib("position");
  resolution_unif = sp->bind_uniform("iResolution");

  sp->use_this_prog();
  glUniform2f(resolution_unif, s->window_width, s->window_height);
  sp->dont_use_this_prog();

  time_unif = sp->bind_uniform("iGlobalTime");

  w = new world(64, 64, 64);
  w->update_texture(sp);
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

  /*
  glm::vec3 pos = glm::vec3(cos(t / 1000.0) * 3.0f, sin(t / 1000.0) * 3.0f, 2.5f)
    , target = glm::vec3(0, 0, 0);
  glm::mat4 proj = glm::perspective(70.f,
      (float)s->window_width / (float)s->window_height, 1.f, 10.f);
  glm::mat4 view = glm::lookAt(pos, target, glm::vec3(0.f, 0.f, 1.f));
  glm::mat4 invProjView = glm::inverse(proj * view);
  glUniformMatrix4fv(sp->bind_attrib("invProjView"), 1, GL_FALSE, glm::value_ptr(invProjView));
  glUniform3f(sp->bind_attrib("viewOrigin"), pos.x, pos.y, pos.z);
  */

  sp->dont_use_this_prog();
}

void draw() {
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
  delete vs;
  delete fs;
  delete sp;
  delete screenverts;
  delete w;
}

int main() {
  screen s(800, 450);

  s.mainloop(load, update, draw, cleanup);

  return 0;
}

