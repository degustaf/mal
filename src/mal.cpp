#include "mal.hpp"

#include <iostream>
#include <string>

static void read(MALState &state, std::string &str) {
  return state.read_str(str);
}

static void eval(const MALState &) {}

static std::string print(const MALState &state) { return state.print_str(); }

static std::string rep(MALState &state, std::string &str) {
  read(state, str);
  eval(state);
  return print(state);
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
