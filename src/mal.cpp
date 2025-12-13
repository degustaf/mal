#include "mal.hpp"

#include <iostream>
#include <string>

static std::string rep(MALState &state, std::string &str) {
  int reg = 0;
  if (!state.read_str(str, reg)) {
    auto ret = state.get_error();
    state.clear_error();
    return ret;
  }
  if (!state.compile(reg)) {
    auto ret = state.get_error();
    state.clear_error();
    return ret;
  }
  if (!state.eval(reg)) {
    auto ret = state.get_error();
    state.clear_error();
    return ret;
  }
  return state.print_str(reg);
}

int main(void) {
  MALState state;
  std::string str;

  std::cout << "user> ";

  while (std::getline(std::cin, str)) {
    std::cout << rep(state, str) << "\n";
    std::cout << "user> ";
  }
  std::cout << "\n";
}
