#pragma once

#include <vector>

class MenuImage {
public:
  ~MenuImage();
  bool Open();
  void Draw(int width, int height) const;

private:
  unsigned image_width = 0, image_height = 0;
  std::vector<int> tiles;
};
