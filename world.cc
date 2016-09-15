#include "world.hh"
#include <random>

uint64_t world::to_index(uint32_t x, uint32_t y, uint32_t z) const {
  return z * h * w + y * w + x;
}

world::world(uint32_t n_w, uint32_t n_h, uint32_t n_d)
  : w(n_w), h(n_h), d(n_d)
    , _data(std::vector<uint8_t>(w * h * d, 0))
    , _texture(0) {
  for (uint32_t z = 0; z < d; z++)
    for (uint32_t y = 0; y < h; y++)
      for (uint32_t x = 0; x < w; x++) {
        _data[to_index(x, y, z)] = 255 * (rand() % 2);
        // printf("wrote %d\n", _data[to_index(x, y, z)]);
      }
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
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  if (_texture > 0)
    glDeleteTextures(1, &_texture);
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &_texture);
  glBindTexture(GL_TEXTURE_3D, _texture);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

  glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, w, h, d, 0, GL_RED, GL_UNSIGNED_BYTE
      , &_data[0]);

  _data_unif = sp->bind_uniform("world_data");
  _w_unif = sp->bind_uniform("w");
  _h_unif = sp->bind_uniform("h");
  _d_unif = sp->bind_uniform("d");
  sp->use_this_prog();
  glUniform1i(_data_unif, 0);
  glUniform1i(_w_unif, 1);
  glUniform1i(_h_unif, 1);
  glUniform1i(_d_unif, 0);
  sp->dont_use_this_prog();
}

