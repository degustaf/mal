#include "state.hpp"

#include "chunk.hpp"
#include "types.hpp"

#include <cassert>
#include <cstdint>
#include <memory>
#include <variant>

enum class ExpKind {
  NIL,         //
  TRUE,        //
  FALSE,       //
  FLOAT,       // x is value
  INT,         // n is value
  STRING,      // str is value
  KEYWORD,     // str is keyword name
  GLOBAL,      // str is symbol name
  CALL,        // s.info is instruction index, s.aux is base register
  RELOCABLE,   // s.info is instruction index
  NONRELOCABLE // r is value register
};

struct ExpDesc {
  ExpDesc() : kind(ExpKind::NIL){};
  ExpDesc(bool b) : kind(b ? ExpKind::TRUE : ExpKind::FALSE){};
  ExpDesc(double x) : kind(ExpKind::FLOAT) { u.x = x; };
  ExpDesc(int n) : kind(ExpKind::INT) { u.n = n; };
  ExpDesc(std::string str) : str(str), kind(ExpKind::STRING){};

  union {
    int n;
    double x;
    reg r;
    struct {
      uint32_t info;
      reg aux;
    } s;
  } u;
  std::string str;
  ExpKind kind;
};

struct FuncState {
  FuncState() : chunk(std::make_unique<Chunk>()){};

  reg regReserve(reg n) {
    size_t sz = nextFreeReg + n;
    if (sz > frameSize) {
      if (sz > MAX_REG) {
        // TODO How to handle this error?
        assert(false);
      }
      frameSize = static_cast<reg>(sz);
    }
    nextFreeReg = static_cast<reg>(sz);
    return nextFreeReg - 1;
  };

  inline void setNextReg(reg n) { nextFreeReg = n; }

  void expr2Reg(ExpDesc &e, reg r) {
    expr2Reg_noBranch(e, r);
    // TODO Handle jumps
    e.kind = ExpKind::NONRELOCABLE;
    e.u.r = r;
  }

  void expr2nextReg(ExpDesc &e) {
    exprDischarge(e);
    exprFree(e);
    regReserve(1);
    expr2Reg(e, nextFreeReg - 1);
  };

  void varLookup(std::string &name, ExpDesc &e) {
    // TODO handle local variables.
    e = ExpDesc(name);
    e.kind = ExpKind::GLOBAL;
  };

  std::unique_ptr<Chunk> getChunk() {
    auto ret = std::move(chunk);
    chunk = nullptr;
    return ret;
  }

  uint32_t emit_ins(byteCode ins) {
    // TODO handle jumps
    chunk->code.push_back(ins);
    return (uint32_t)chunk->code.size() - 1;
  }

private:
  void regFree(reg r) {
    if (r >= nVars) {
      nextFreeReg--;
      assert(r == nextFreeReg);
    }
  }

  void exprDischarge(ExpDesc &e) {
    switch (e.kind) {
    case ExpKind::GLOBAL: {
      auto str =
          chunk->addConstant(MALType{std::make_shared<MALString>(e.str)});
      auto ins = byteCode::AD(opCode::GLOBAL_GET, 0, str);
      e.u.s.info = emit_ins(ins);
      e.kind = ExpKind::RELOCABLE;
      break;
    }
    case ExpKind::CALL:
      e.kind = ExpKind::NONRELOCABLE;
      e.u.r = e.u.s.aux;
      break;
    case ExpKind::NIL:
    case ExpKind::TRUE:
    case ExpKind::FALSE:
    case ExpKind::FLOAT:
    case ExpKind::INT:
    case ExpKind::STRING:
    case ExpKind::KEYWORD:
    case ExpKind::RELOCABLE:
    case ExpKind::NONRELOCABLE:
      return;
    }
  };

  void exprFree(ExpDesc &e) {
    if (e.kind == ExpKind::NONRELOCABLE) {
      regFree(e.u.r);
    }
  }

  void expr2Reg_noBranch(ExpDesc &e, reg r) {
    exprDischarge(e);
    switch (e.kind) {
    case ExpKind::NIL:
      emit_ins(byteCode::AD(opCode::PRIMITIVE, r, KNIL));
      break;
    case ExpKind::INT: {
      emit_ins(
          byteCode::AD(opCode::CONST, r, chunk->addConstant(MALType{e.u.n})));
      break;
    }
    case ExpKind::FLOAT: {
      emit_ins(
          byteCode::AD(opCode::CONST, r, chunk->addConstant(MALType{e.u.x})));
      break;
    }
    case ExpKind::STRING:
      emit_ins(byteCode::AD(opCode::CONST, r, chunk->addConstant(e.str)));
      break;
    case ExpKind::KEYWORD:
      emit_ins(byteCode::AD(opCode::CONST, r, chunk->addKeyword(e.str)));
      break;
    case ExpKind::RELOCABLE:
      chunk->code[e.u.s.info].regA() = r;
      e.u.r = r;
      e.kind = ExpKind::NONRELOCABLE;
      break;
    case ExpKind::NONRELOCABLE:
      if (r != e.u.r) {
        emit_ins(byteCode::AD(opCode::MOV, r, e.u.r));
        e.u.r = r;
        e.kind = ExpKind::NONRELOCABLE;
      }
      break;
    default:
      assert(false);
    }
  }

  reg nVars = 0;
  reg nextFreeReg = 0;
  uint8_t frameSize = 0;
  std::unique_ptr<Chunk> chunk;
};

struct Compiler {
  Compiler(ExpDesc &e) : fn(), e(&e), error(nullptr){};
  FuncState fn;
  ExpDesc *e;
  std::shared_ptr<MALError> error;

  void operator()(std::monostate) { *e = ExpDesc(); };
  void operator()(bool b) { *e = ExpDesc(b); };
  void operator()(int n) { *e = ExpDesc(n); };
  void operator()(double x) { *e = ExpDesc(x); };
  void operator()(std::shared_ptr<MALSymbol> sym) {
    fn.varLookup(sym->symbol, *e);
  };
  void operator()(std::shared_ptr<MALKeyword> key) {
    *e = ExpDesc(key->keyword);
    e->kind = ExpKind::KEYWORD;
  };
  void operator()(std::shared_ptr<MALString> str) { *e = ExpDesc(str->str); };
  void operator()(std::shared_ptr<MALCFunc>) { assert(false); };
  void operator()(std::shared_ptr<CallFrame>) { assert(false); };

  void operator()(std::shared_ptr<MALList> l) {
    if (l->empty()) {
      auto r = fn.regReserve(1);
      fn.emit_ins(byteCode::AD(opCode::NEW_LIST, r, 0));
      e->kind = ExpKind::NONRELOCABLE;
      e->u.r = r;
      return;
    };

    // TODO handle special forms

    // a non-empty list is a function call.
    functionCall(*l);
  };

  void operator()(std::shared_ptr<MALVector>) { /* TODO */ };
  void operator()(std::shared_ptr<MALMap>) { /* TODO */ };

private:
  void functionCall(const MALList &l) {
    assert(!l.empty());
    auto it = l.begin();
    std::visit(*this, it->data);
    fn.expr2nextReg(*e);

    uint16_t argCount = 0;
    it++;
    if (it != l.end()) {
      ExpDesc *e_cache = e;
      ExpDesc args;
      e = &args;
      std::visit(*this, it->data);
      argCount++;
      for (it++; it != l.end(); it++) {
        fn.expr2nextReg(*e);
        std::visit(*this, it->data);
        argCount++;
      }
      fn.expr2nextReg(args);
      e = e_cache;
    }
    auto base = e->u.r;
    e->u.s.info = fn.emit_ins(byteCode::AD(opCode::CALL, base, argCount));
    e->u.s.aux = base;
    e->kind = ExpKind::CALL;
    fn.setNextReg(base + 1);
  }
};

bool MALState::compile(int r) {
  auto &code = state->stack[(size_t)r];
  ExpDesc e;
  auto compiler = Compiler(e);
  std::visit(compiler, code.data);
  if (compiler.error) {
    state->error = compiler.error;
    return false;
  }
  compiler.fn.expr2nextReg(e);
  if (compiler.error) {
    state->error = compiler.error;
    return false;
  }
  state->chunk = compiler.fn.getChunk();
  return true;
}
