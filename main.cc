#include <fstream>
#include "pxdrw.hh"

void drawvline(int x, int sz, uint32_t color, pixeldrawer *pd) {
  for (int y = 0; y < sz; y++) {
    pd->write(x, pd->wheight / 2 - sz / 2 + y, color);
  }
}

void draw(pixeldrawer *pd) {
  drawvline(1, 10, 0xFF0000FF, pd);
  drawvline(3, 3, 0x00FF00FF, pd);
}

void update(uint32_t dtms, uint32_t t) {

}

int main() {
  pixeldrawer screen(800, 600);

  screen.mainloop(update, draw);

  return 0;
}

