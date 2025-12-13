#include "state.hpp"
#include "types.hpp"

#undef DEBUG

#ifdef DEBUG
#include "debug.hpp"
#endif

#include <cassert>
#include <memory>

bool MALState::State::eval(int) {
#ifdef DEBUG
  disassembleChunk(*chunk);
#endif

  auto &code = chunk->code;
  std::vector<byteCode>::const_iterator ip = code.begin();
  while (ip != code.end()) {
    auto instruction = *ip;
    ip++;
    switch (instruction.op()) {
    case opCode::CONST:
      assert(stackTop + instruction.regA() <= stack.end());
      assert(instruction.regD() <= chunk->constants.size());
      stackTop[instruction.regA()] = chunk->constants[instruction.regD()];
      break;
    case opCode::GLOBAL_GET: {
      assert(instruction.regD() <= chunk->constants.size());
      auto &c = chunk->constants[instruction.regD()];
      auto key = std::get_if<std::shared_ptr<MALString>>(&c.data);
      assert(key);
      auto val = globals.data.find(key->get()->str);
      if (val != globals.data.end()) {
        assert(stackTop + instruction.regA() <= stack.end());
        stackTop[instruction.regA()] = val->second;
      } else {
        error = std::make_shared<MALError>("Unkown global variable");
        return false;
      }
      break;
    }
    case opCode::NEW_LIST:
      assert(stackTop + instruction.regA() <= stack.end());
      stackTop[instruction.regA()] =
          MALType{std::make_shared<MALList>(instruction.regD())};
      break;
    case opCode::CALL: {
      assert(stackTop + instruction.regA() <= stack.end());
      auto m = stackTop[instruction.regA()];
      auto cf = std::make_shared<CallFrame>();
      cf->fn = m;
      cf->parent_ip = ip;
      cf->stack_offset = instruction.regA() + 1;
      auto argCount = instruction.regD();
      // TODO make this more resilient
      auto fn = std::get_if<std::shared_ptr<MALCFunc>>(&m.data);
      assert(fn);
      stackTop[instruction.regA()] = MALType{cf};
      stackTop += instruction.regA() + 1;
      if (!(*fn)->fn(&parent, argCount)) {
        assert(error);
        return false;
      }
      stackTop[-1] = stackTop[0];
      stackTop -= cf->stack_offset;
      ip = cf->parent_ip;
      break;
    }
    case opCode::MOV:
      assert(stackTop + instruction.regA() <= stack.end());
      assert(stackTop + instruction.regD() <= stack.end());
      stackTop[instruction.regA()] = stackTop[instruction.regD()];
      break;
    case opCode::PRIMITIVE:
      assert(stackTop + instruction.regA() <= stack.end());
      auto prim = instruction.regD();
      switch (prim) {
      case KNIL:
        stackTop[instruction.regA()] = MALType();
        break;
      case KTRUE:
        stackTop[instruction.regA()] = MALType{true};
        break;
      case KFALSE:
        stackTop[instruction.regA()] = MALType{false};
        break;
      default:
        assert(false);
      }
      break;
    }
  }
  return true;
}

bool MALState::eval(int r) { return state->eval(r); }
