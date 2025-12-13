#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

#define OPCODE_BUILDER(X, sep)                                                 \
  X(CONST, AD)                                                                 \
  sep X(GLOBAL_GET, AD)                                                        \
  sep X(NEW_LIST, AD)                                                          \
  sep X(CALL, AD)                                                              \
  sep X(PRIMITIVE, AD)                                                         \
  sep X(MOV, AD) sep

#define COMMA ,
#define BUILD_OPCODES(op, _) op
enum class opCode : uint8_t { OPCODE_BUILDER(BUILD_OPCODES, COMMA) };

#define KNIL 1
#define KTRUE 2
#define KFALSE 3

typedef uint8_t reg;
static constexpr size_t MAX_REG = std::numeric_limits<reg>::max();

struct byteCode {
  static inline byteCode ABC(opCode op, reg a, reg b, reg c) {
    return byteCode{{(reg)op, a, b, c}};
  };
  static inline byteCode AD(opCode op, reg a, uint16_t d) {
    return byteCode{{(reg)op, a, (reg)(0xff & d), (reg)(d >> 8)}};
  };

  inline opCode op(void) const { return (opCode)bytes[0]; };
  inline reg regA(void) const { return bytes[1]; };
  inline reg &regA(void) { return bytes[1]; };
  inline reg regB(void) const { return bytes[2]; };
  inline reg regC(void) const { return bytes[3]; };
  inline uint16_t regD(void) const {
    return (uint16_t)((((uint16_t)bytes[3]) << 8) | ((uint16_t)bytes[2]));
  };

  alignas(uint32_t) reg bytes[4];
};
static_assert(sizeof(byteCode) == 4, "byteCode is an unexpected size");
static_assert(alignof(byteCode) == 4, "byteCode has an unexpected alignment");
