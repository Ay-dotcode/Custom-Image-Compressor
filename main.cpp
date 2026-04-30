#include "./image.h"

using namespace std;

int main() {
  ColorImage I;
  I.Load("input.png");
  int iw = I.GetWidth(), ih = I.GetHeight();
  ColorImage O(iw, ih);

  I.Save("output.png");
}