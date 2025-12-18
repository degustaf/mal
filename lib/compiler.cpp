#include "bytecode.hpp"
#include "chunk.hpp"
#include "state.hpp"
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
  LOCAL,       // s.aux is local register, s.info is vars index
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

struct Scope {
  Scope *outer = nullptr;
  size_t nVars; // Number of variables in outer scope.
};

struct variableInfo {
  std::string name;
  reg slot;
  uint8_t flags;
};

struct FuncState {
  FuncState(std::vector<variableInfo> &vars)
      : chunk(std::make_unique<Chunk>()), varsRef(vars), outer(nullptr){};
  FuncState(FuncState &outer)
      : chunk(std::make_unique<Chunk>()), varsRef(outer.varsRef),
        outer(&outer){};

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

  reg expr2anyReg(ExpDesc &e) {
    exprDischarge(e);
    if (e.kind == ExpKind::NONRELOCABLE) {
      // if(!has jump)
      return e.u.r;
      // else ...
    }
    expr2nextReg(e);
    return e.u.r;
  }

  int varLookup(std::string &name, ExpDesc &e, bool) {
    auto r = varLookupLocal(name);
    if (r >= 0) {
      e.u.s.aux = (reg)r;
      e.kind = ExpKind::LOCAL;
      // TODO upvalue logic
      e.u.s.info = varMap[(reg)r];
      return (int)e.u.s.info;
    }
    if (outer) {
      auto vidx = outer->varLookup(name, e, false);
      if (vidx >= 0) {
        // TODO it's an upvalue
      }
      return vidx;
    }

    // Must be a global variable.
    e = ExpDesc(name);
    e.kind = ExpKind::GLOBAL;
    return -1;
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

  void emitGlobalStore(std::string &var, ExpDesc &e) {
    auto r = expr2anyReg(e);
    auto str = chunk->addConstant(MALType{std::make_shared<MALString>(var)});
    auto ins = byteCode::AD(opCode::GLOBAL_SET, r, str);
    emit_ins(ins);
  }

  void beginScope(Scope &scope) {
    scope.nVars = nVars;
    scope.outer = this->scope;
    this->scope = &scope;
    assert(nextFreeReg == nVars);
  }

  void endScope() {
    auto block = scope;
    scope = block->outer;
    while (nVars > block->nVars) {
      varsRef.pop_back();
      nVars--;
    }
    nextFreeReg = nVars;
    // TODO upvalues
  }

  reg nVars = 0;
  std::vector<uint16_t> varMap;

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
    case ExpKind::LOCAL:
      e.kind = ExpKind::NONRELOCABLE;
      e.u.r = e.u.s.aux;
      break;
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

  int varLookupLocal(std::string &str) {
    for (int i = (int)varsRef.size() - 1; i >= 0; i--) {
      if (varsRef[varMap[(size_t)i]].name == str) {
        return i;
      }
    }
    return -1;
  }

  reg nextFreeReg = 0;
  uint8_t frameSize = 0;
  std::unique_ptr<Chunk> chunk;
  std::vector<variableInfo> &varsRef;
  Scope *scope = nullptr;
  FuncState *outer;
};

struct Compiler {
  Compiler(ExpDesc &e)
      : vars(), fn(new FuncState(vars)), e(&e), error(nullptr){};
  Compiler(const Compiler &) = delete;
  Compiler &operator=(const Compiler &) = delete;
  ~Compiler() { delete fn; }

  std::vector<variableInfo> vars;
  FuncState *fn;
  ExpDesc *e;
  std::shared_ptr<MALError> error;

  void operator()(std::monostate) { *e = ExpDesc(); };
  void operator()(bool b) { *e = ExpDesc(b); };
  void operator()(int n) { *e = ExpDesc(n); };
  void operator()(double x) { *e = ExpDesc(x); };
  void operator()(const std::shared_ptr<MALSymbol> &sym) {
    fn->varLookup(sym->symbol, *e, true);
  };
  void operator()(const std::shared_ptr<MALKeyword> &key) {
    *e = ExpDesc(key->keyword);
    e->kind = ExpKind::KEYWORD;
  };
  void operator()(std::shared_ptr<MALString> str) { *e = ExpDesc(str->str); };
  void operator()(std::shared_ptr<MALCFunc>) { assert(false); };
  void operator()(std::shared_ptr<CallFrame>) { assert(false); };
  void operator()(std::shared_ptr<MALMap>) { assert(false); };

  void operator()(const std::shared_ptr<MALList> &l) {
    if (l->empty()) {
      auto r = fn->regReserve(1);
      fn->emit_ins(byteCode::AD(opCode::NEW_LIST, r, 0));
      e->kind = ExpKind::NONRELOCABLE;
      e->u.r = r;
      return;
    };

    // Special forms
    if (auto m = std::get_if<std::shared_ptr<MALSymbol>>(&l->data[0].data); m) {
      auto &form = (*m)->symbol;
      if (form == "def!") {
        defCall(*l);
        return;
      } else if (form == "let*") {
        letCall(*l);
        return;
      }
    }

    // a non-empty list is a function call.
    functionCall(*l);
  };

  void operator()(const std::shared_ptr<MALVector> &v) {
    auto l = MALList(v->size() + 1);
    l.data.push_back(MALType{std::make_shared<MALSymbol>("vec")});
    for (auto &m : *v) {
      l.data.push_back(m);
    }
    functionCall(l);
  };

private:
  void functionCall(const MALList &l) {
    assert(!l.empty());
    auto it = l.begin();
    std::visit(*this, it->data);
    if (error)
      return;
    fn->expr2nextReg(*e);

    uint16_t argCount = 0;
    it++;
    if (it != l.end()) {
      ExpDesc *e_cache = e;
      ExpDesc args;
      e = &args;
      std::visit(*this, it->data);
      if (error)
        return;
      argCount++;
      for (it++; it != l.end(); it++) {
        fn->expr2nextReg(*e);
        std::visit(*this, it->data);
        if (error)
          return;
        argCount++;
      }
      fn->expr2nextReg(args);
      e = e_cache;
    }
    auto base = e->u.r;
    e->u.s.info = fn->emit_ins(byteCode::AD(opCode::CALL, base, argCount));
    e->u.s.aux = base;
    e->kind = ExpKind::CALL;
    fn->setNextReg(base + 1);
  }

  void defCall(const MALList &l) {
    switch (l.size()) {
    case 0:
      assert(false); // How did we get here?
      break;
    case 1:
    case 2:
      error = std::make_shared<MALError>("Not enough arguments to def!");
      return;
    case 3:
      // This is the good case
      break;
    default:
      error = std::make_shared<MALError>("Too many arguments to def!");
      return;
    }
    assert(l.size() == 3);
    auto it = ++l.begin();
    auto sym = std::get_if<std::shared_ptr<MALSymbol>>(&it->data);
    if (sym == nullptr) {
      error = std::make_shared<MALError>("Var name should be a simple symbol.");
      return;
    }
    it++;

    std::visit(*this, it->data);
    fn->emitGlobalStore((*sym)->symbol, *e);
  }

  void letCall(const MALList &l) {
    Scope sc;
    fn->beginScope(sc);
    auto it = l.begin();
    assert(it != l.end());
    it++;
    if (it == l.end()) {
      *e = ExpDesc();
      return;
    }

    // This is where we actually handle the assignments.
    auto [ptr, end] = std::visit(Iterator{*it}, it->data);
    if (ptr == nullptr) {
      error = std::make_shared<MALError>("argument to let* isn't a sequence");
      return;
    }
    while (ptr != end) {
      auto s = std::get_if<std::shared_ptr<MALSymbol>>(&ptr->data);
      if (!s) {
        error = std::make_shared<MALError>("Unsupported let binding");
        return;
      }
      // If we cared about stack space we, would check if this was shadowing a
      // variable in the same scope. since we look for variables from newest
      // defined to oldest, this shouldn't matter.
      assert(fn->varMap.size() == fn->nVars);
      fn->varMap.push_back((uint16_t)vars.size());
      vars.push_back({(*s)->symbol, 0, 0});

      ptr++;
      if (ptr != end) {
        std::visit(*this, ptr->data);
      } else {
        e->kind = ExpKind::NIL;
      }
      fn->expr2nextReg(*e);
      auto &v = *vars.rbegin();
      v.slot = fn->nVars;
      fn->nVars++;
      ptr++;
    }

    it++;
    if (it != l.end()) {
      std::visit(*this, it->data);

      for (it++; it != l.end(); it++) {
        fn->expr2nextReg(*e);
        std::visit(*this, it->data);
      }
    } else {
      e->kind = ExpKind::NIL;
    }
    // fn->expr2Reg(*e, (reg)sc.nVars);

    fn->endScope();
    // if (e->kind == ExpKind::LOCAL) {
    //   fn->regReserve(1);
    // }
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
  compiler.fn->expr2Reg(e, (reg)r);
  if (compiler.error) {
    state->error = compiler.error;
    return false;
  }
  state->chunk = compiler.fn->getChunk();
  return true;
}
