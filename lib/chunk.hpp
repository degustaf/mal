#pragma once

#include "types.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

struct Chunk {
  Chunk() = default;
  std::vector<byteCode> code;
  std::vector<MALType> constants;

  uint16_t addConstant(MALType t) {
    constants.push_back(t);
    assert(constants.size() == (size_t)(uint16_t)constants.size());
    return (uint16_t)constants.size() - 1;
  };

  uint16_t addConstant(std::string &str) {
    constants.push_back(MALType{std::make_shared<MALString>(str)});
    assert(constants.size() == (size_t)(uint16_t)constants.size());
    return (uint16_t)constants.size() - 1;
  }

  uint16_t addKeyword(std::string &str) {
    constants.push_back(MALType{std::make_shared<MALKeyword>(str)});
    assert(constants.size() == (size_t)(uint16_t)constants.size());
    return (uint16_t)constants.size() - 1;
  }
};
