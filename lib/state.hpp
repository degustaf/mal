#pragma once

#include "mal.hpp"

#include "chunk.hpp"
#include "types.hpp"

#include <memory>
#include <vector>

struct MALState::State {
  State(MALState &parent)
      : stack(10), stackTop(stack.begin()), error(nullptr), parent(parent) {
    initGlobals();
  };

  bool eval(int);

  std::vector<MALType> stack;
  std::vector<MALType>::iterator stackTop;
  std::unique_ptr<Chunk> chunk;
  std::shared_ptr<MALError> error;

  MALMap globals;

private:
  void initGlobals();
  MALState &parent;
};
