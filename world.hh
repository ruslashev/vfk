#pragma once

#include "ogl.hh"
#include <GL/glew.h>
#include <vector>
#include <cstdint>

class world {
  std::vector<uint8_t> _data;
  uint64_t to_index(uint32_t x, uint32_t y, uint32_t z) const;
  GLuint _texture;
  GLint _data_attr, _w_attr, _h_attr, _d_attr;
public:
  uint32_t w, h, d;
  world(uint32_t w, uint32_t h, uint32_t d);
  ~world();
  uint8_t get(uint32_t x, uint32_t y, uint32_t z) const;
  void update_texture(shaderprogram *sp);
};

