#pragma once

#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL2/SDL.h>

class screen {
  SDL_Window *_window;
  SDL_GLContext _gl_context;
public:
  int window_width, window_height;
  bool running;
  screen(int window_width, int window_height);
  ~screen();
  void mainloop(void (*load_cb)(void)
      , void (*update_cb)(double, uint32_t, screen*)
      , void (*draw_cb)(void)
      , void (*cleanup_cb)(void));
};
