#include "screen.hh"
#include "utils.hh"

screen::screen(int window_width, int window_height)
  : window_width(window_width), window_height(window_height) {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  _window = SDL_CreateWindow("vfk", SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_OPENGL);

  _gl_context = SDL_GL_CreateContext(_window);

  GLenum err = glewInit();
  assert(err == GLEW_OK, "failed to initialze glew: %s",
      glewGetErrorString(err));

  assert(GLEW_VERSION_2_0, "your graphic card does not support OpenGL 2.0");

  running = true;
}

screen::~screen() {
  SDL_GL_DeleteContext(_gl_context);
  SDL_Quit();
}

void screen::mainloop(void (*load_cb)(void)
    , void (*update_cb)(double, uint32_t, screen*)
    , void (*draw_cb)(void)
    , void (*cleanup_cb)(void)) {
  load_cb();

  uint32_t simtime = 0;

  while (running) {
    uint32_t realtime = SDL_GetTicks();

    while (simtime < realtime) {
      simtime += 16;

      update_cb(16. / 1000., simtime, this);
    }

    draw_cb();

    SDL_GL_SwapWindow(_window);
  }

}
