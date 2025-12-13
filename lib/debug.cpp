#include "debug.hpp"

#include "chunk.hpp"

#include <cstring>

#include <iostream>

static void instructionAD(const char *op, const byteCode &b) {
  uint16_t a = b.regA();
  uint16_t d = b.regD();
  std::cout << op << " register " << d << " to register " << a << "\n";
}

#define BUILD_DISASSEMBLY(op, type)                                            \
  case opCode::op:                                                             \
    instruction##type(#op, b);                                                 \
    break

void disassembleInstruction(const byteCode &b, size_t offset) {
  static_assert(sizeof(uint32_t) == sizeof(byteCode),
                "Unexpected size of byteCode");
  uint32_t bits;
  std::memcpy(&bits, &b, sizeof(b));
  std::cout << &b << "\t" << offset << "\t" << std::hex << bits << std::dec
            << "\t\t\t";

  switch (b.op()) { OPCODE_BUILDER(BUILD_DISASSEMBLY, ;) }
}

void disassembleChunk(const Chunk &chunk) {
  std::cout << " = Constants\n";
  for (size_t i = 0; i < chunk.constants.size(); i++) {
    std::cout << i << "\t" << (std::string)chunk.constants[i] << "\n";
  }
  std::cout << "\n";

  for (size_t offset = 0; offset < chunk.code.size(); offset++) {
    disassembleInstruction(chunk.code[offset], offset);
  }
}
