#pragma once

#include <string>

struct MALState {
  MALState();
  ~MALState();

  MALState(MALState &&s) = default;
  MALState &operator=(MALState &&s) = default;

  MALState(const MALState &s) = delete;
  MALState &operator=(const MALState &s) = delete;

  void read_str(std::string &);
  std::string print_str(void) const;

private:
  struct State;
  State *state;
};
