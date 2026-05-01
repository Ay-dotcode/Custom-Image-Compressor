#include "./image.h"
#include <cstdlib>
#include <ctime>
#include <vector>

using namespace std;

int main() {
  srand(static_cast<unsigned>(time(nullptr)));

  int width = 1024;
  int height = 1024;
  ColorImage img(width, height);

  vector<RGBA> palette = {RGBA(255, 0, 0, 255), RGBA(0, 255, 0, 255),
                          RGBA(0, 0, 255, 255), RGBA(255, 255, 0, 255),
                          RGBA(0, 0, 0, 255),   RGBA(255, 255, 255, 255)};

  int currentRunLength = 0;
  RGBA currentColor = palette[0];

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      if (currentRunLength == 0) {
        currentColor = palette[rand() % palette.size()];
        currentRunLength = 20 + (rand() % 130);
      }
      img(x, y) = currentColor;
      currentRunLength--;
    }
  }

  img.Save("input.png");
  return 0;
}