#include <fstream>
#include "pxdrw.hh"

void drawvline(pixeldrawer *pd, int x, int sz, uint32_t color) {
  for (int y = 0; y < sz; y++)
    pd->write(x, pd->wheight / 2 - sz / 2 + y, color);
}

void drawsq(pixeldrawer *pd, int x, int y, int sz, uint32_t color) {
  for (int dy = 0; dy < sz; dy++)
    for (int dx = 0; dx < sz; dx++)
      pd->write(x + dx, y + dy, color);
}

int tilecolor(int t) {
  switch (t) {
    case 1: return 0xFFFFFF;
    case 2: return 0xFF0000;
    case 3: return 0x00FF00;
    case 4: return 0x0000FF;
    case 5: return 0xFF00FF;
  }
}

const int mapsz = 10;
const int map[mapsz][mapsz] = {
  {1,1,1,2,3,4,2,1,1,1},
  {1,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,5},
  {1,0,0,0,0,0,0,0,0,5},
  {1,0,0,0,0,0,0,0,0,6},
  {1,0,0,0,0,0,0,0,0,5},
  {1,0,0,0,0,0,0,0,0,5},
  {1,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,1},
  {1,1,5,2,3,4,2,1,1,1},
};
static double playerx = 30, playery = 40, playerang = 0;
const double tilesize = 10;
bool running = true;

void update(double dt, uint32_t t) {
  SDL_Event event;
  while (SDL_PollEvent(&event) != 0) {
    if (event.type == SDL_QUIT)
      running = false;
    else if (event.type == SDL_KEYDOWN) {

    }
  }
  playerang += 3 * dt;
  double ddy = tilesize * tan(playerang);
}

void draw(pixeldrawer *pd) {
  const int offset = 5, scale = 5;
  for (int y = 0; y < mapsz; y++)
    for (int x = 0; x < mapsz; x++)
      if (map[y][x])
        drawsq(pd, offset + x * scale, offset + y * scale, scale,
            tilecolor(map[y][x]));
  int plx = offset + (playerx / tilesize) * scale,
      ply = offset + (playery / tilesize) * scale;
  drawsq(pd, plx - 1, ply - 1, 3, 0xAAAAAA);
  for (int i = 0; i < 5; i++)
    pd->write(round(plx + i * cos(playerang)), round(ply + i * sin(playerang)),
        0xFF0000);
}

int main() {
  pixeldrawer screen(800, 600);

  screen.mainloop(update, draw);

  return 0;
}

