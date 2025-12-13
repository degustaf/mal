#include "state.hpp"
#include "mal.hpp"
#include "types.hpp"

#include <memory>

MALState::MALState() : state(new MALState::State(*this)) {}

MALState::~MALState() { delete state; }

std::string MALState::print_str(int reg) const {
  return state->stack[(size_t)reg];
}

std::string MALState::get_error() const { return *state->error; }

void MALState::clear_error() { state->error = nullptr; }

bool MALState::add(MALState *M, size_t argCount) {
  assert(argCount == 2);
  auto state = M->state;
  auto a = state->stackTop[0];
  auto b = state->stackTop[1];
  auto a_int = std::get_if<int>(&a.data);
  auto b_int = std::get_if<int>(&b.data);
  auto a_float = std::get_if<double>(&a.data);
  auto b_float = std::get_if<double>(&b.data);
  if (a_int && b_int) {
    state->stackTop[0] = MALType{*a_int + *b_int};
    return true;
  }
  if (a_int && b_float) {
    state->stackTop[0] = MALType{*a_int + *b_float};
    return true;
  }
  if (a_float && b_int) {
    state->stackTop[0] = MALType{*a_float + *b_int};
    return true;
  }

  state->error = std::make_shared<MALError>("Illegal arguments to add");
  return false;
}

bool MALState::mult(MALState *M, size_t argCount) {
  assert(argCount == 2);
  auto state = M->state;
  auto a = state->stackTop[0];
  auto b = state->stackTop[1];
  auto a_int = std::get_if<int>(&a.data);
  auto b_int = std::get_if<int>(&b.data);
  auto a_float = std::get_if<double>(&a.data);
  auto b_float = std::get_if<double>(&b.data);
  if (a_int && b_int) {
    state->stackTop[0] = MALType{*a_int * *b_int};
    return true;
  }
  if (a_int && b_float) {
    state->stackTop[0] = MALType{*a_int * *b_float};
    return true;
  }
  if (a_float && b_int) {
    state->stackTop[0] = MALType{*a_float * *b_int};
    return true;
  }

  state->error = std::make_shared<MALError>("Illegal arguments to multiply");
  return false;
}

bool MALState::sub(MALState *M, size_t argCount) {
  assert(argCount == 2);
  auto state = M->state;
  auto a = state->stackTop[0];
  auto b = state->stackTop[1];
  auto a_int = std::get_if<int>(&a.data);
  auto b_int = std::get_if<int>(&b.data);
  auto a_float = std::get_if<double>(&a.data);
  auto b_float = std::get_if<double>(&b.data);
  if (a_int && b_int) {
    state->stackTop[0] = MALType{*a_int - *b_int};
    return true;
  }
  if (a_int && b_float) {
    state->stackTop[0] = MALType{*a_int - *b_float};
    return true;
  }
  if (a_float && b_int) {
    state->stackTop[0] = MALType{*a_float - *b_int};
    return true;
  }

  state->error = std::make_shared<MALError>("Illegal arguments to sub");
  return false;
}

bool MALState::div(MALState *M, size_t argCount) {
  assert(argCount == 2);
  auto state = M->state;
  auto a = state->stackTop[0];
  auto b = state->stackTop[1];
  auto a_int = std::get_if<int>(&a.data);
  auto b_int = std::get_if<int>(&b.data);
  auto a_float = std::get_if<double>(&a.data);
  auto b_float = std::get_if<double>(&b.data);
  if (a_int && b_int) {
    state->stackTop[0] = MALType{*a_int / *b_int};
    return true;
  }
  if (a_int && b_float) {
    state->stackTop[0] = MALType{*a_int / *b_float};
    return true;
  }
  if (a_float && b_int) {
    state->stackTop[0] = MALType{*a_float / *b_int};
    return true;
  }

  state->error = std::make_shared<MALError>("Illegal arguments to sub");
  return false;
}

bool MALState::vec(MALState *M, size_t argCount) {
  auto ret = std::make_shared<MALVector>(argCount);
  auto stackPtr = M->state->stackTop;
  for (size_t i = 0; i < argCount; i++) {
    ret->data.push_back(*stackPtr);
    stackPtr++;
  }
  M->state->stackTop[0] = MALType{ret};
  return true;
}

bool MALState::hash_map(MALState *M, size_t argCount) {
  if (argCount & 1) {
    M->state->error = std::make_shared<MALError>(
        "hash-map requires an even number of arguments");
    return false;
  }

  auto ret = std::make_shared<MALMap>(argCount);
  auto stackPtr = M->state->stackTop;
  for (size_t i = 0; i < argCount; i += 2) {
    ret->data[(std::string)stackPtr[0]] = stackPtr[1];
    stackPtr += 2;
  }

  M->state->stackTop[0] = MALType{ret};
  return true;
}

void MALState::State::initGlobals() {
  globals.data["+"] = MALType{std::make_shared<MALCFunc>(add, "+")};
  globals.data["-"] = MALType{std::make_shared<MALCFunc>(sub, "-")};
  globals.data["*"] = MALType{std::make_shared<MALCFunc>(mult, "*")};
  globals.data["/"] = MALType{std::make_shared<MALCFunc>(div, "/")};

  globals.data["vec"] = MALType{std::make_shared<MALCFunc>(vec, "vec")};
  globals.data["hash-map"] =
      MALType{std::make_shared<MALCFunc>(hash_map, "hash-map")};
}
