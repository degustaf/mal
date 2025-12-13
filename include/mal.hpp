#pragma once

#include <string>

struct MALState {
  MALState();
  ~MALState();

  MALState(MALState &&s) = default;
  MALState &operator=(MALState &&s) = default;

  MALState(const MALState &s) = delete;
  MALState &operator=(const MALState &s) = delete;

  bool read_str(std::string &, int);
  bool compile(int);
  bool eval(int);
  std::string print_str(int) const;
  std::string get_error() const;
  void clear_error();

private:
  struct State;
  State *state;

  static bool add(MALState *, size_t);
  static bool mult(MALState *, size_t);
  static bool sub(MALState *, size_t);
  static bool div(MALState *, size_t);
  static bool vec(MALState *, size_t);
  static bool hash_map(MALState *, size_t);
};

typedef bool (*CFunction)(MALState *state, size_t argCount);
