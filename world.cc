#include "world.hh"
#include <random>

uint64_t world::to_index(uint32_t x, uint32_t y, uint32_t z) const {
  return z * h * w + y * w + x;
}

world::world(uint32_t w, uint32_t h, uint32_t d)
  : w(w), h(h), d(d), _data(std::vector<uint8_t>(w * h * d, 0)), _texture(0) {
  for (int z = 0; z < d; z++)
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++)
        if (y < h / 2)
          _data[to_index(x, y, z)] = 1;
        else if (y > h / 2)
          _data[to_index(x, y, z)] = 0;
        else
          _data[to_index(x, y, z)] = rand() % 2;
}

world::~world() {
  if (_texture > 0)
    glDeleteTextures(1, &_texture);
}

uint8_t world::get(uint32_t x, uint32_t y, uint32_t z) const {
  const uint64_t idx = to_index(x, y, z);
  return (idx < w * h * d) ? _data[idx] : 0;
}

void world::update_texture(shaderprogram *sp) {
  if (_texture > 0)
    glDeleteTextures(1, &_texture);
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &_texture);
  glBindTexture(GL_TEXTURE_3D, _texture);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_R8UI, w, h, d, 0, GL_RED_INTEGER
      , GL_UNSIGNED_INT, &_data[0]);

  _data_attr = sp->bind_attrib("data");
  _w_attr = sp->bind_attrib("w");
  _h_attr = sp->bind_attrib("h");
  _d_attr = sp->bind_attrib("d");
  glUniform1f(_data_attr, 0);
  glUniform1i(_w_attr, w);
  glUniform1i(_h_attr, h);
  glUniform1i(_d_attr, d);
}

