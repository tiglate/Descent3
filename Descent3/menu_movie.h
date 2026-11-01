#pragma once

#include <memory>

class MenuMovie {
public:
  MenuMovie();
  ~MenuMovie();
  bool Open();
  bool Draw(int width, int height);

private:
  struct State;
  std::unique_ptr<State> state;
};
