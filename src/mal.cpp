#include "mal.hpp"

#include <iostream>
#include <string>

static std::string read(std::string str) { return str; }

static std::string eval(std::string ast, void *) { return ast; }

static std::string print(std::string exp) { return exp; }

static std::string rep(std::string str) {
  return print(eval(read(str), nullptr));
}

int main(void) {
  std::string str;

  std::cout << "user> ";

  while (std::getline(std::cin, str)) {
    std::cout << rep(str) << "\n";
    std::cout << "user> ";
  }
}
