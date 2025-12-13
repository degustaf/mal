#include "types.hpp"

#include <cassert>
#include <memory>
#include <sstream>
#include <string>
#include <variant>

template <char start, char end>
static inline std::string toString(std::vector<MALType> data) {
  std::stringstream stream;
  stream << start;
  for (auto &m : data) {
    stream << (std::string)m << " "; // TODO chanege to C++ style cast
  }
  if (stream.str().size() != 1) {
    stream.seekp(-1, stream.cur);
  }
  stream << end;
  return stream.str();
}

MALList::operator std::string() { return toString<'(', ')'>(data); }

MALVector::operator std::string() { return toString<'[', ']'>(data); }

MALMap::operator std::string() {
  std::stringstream stream;
  stream << "{";
  for (auto &m : data) {
    stream << m.first << " ";
    stream << (std::string)m.second << " "; // TODO chanege to C++ style cast
  }
  if (stream.str().size() != 1) {
    stream.seekp(-1, stream.cur);
  }
  stream << "}";
  return stream.str();
}

MALSymbol::operator std::string() const { return symbol; }

MALKeyword::operator std::string() const { return keyword; }

MALString::operator std::string() const { return str; }

MALCFunc::operator std::string() const { return name; }

MALError::operator std::string() const { return msg; }

struct StringVisitor {
  std::string operator()(std::monostate) { return "nil"; }
  std::string operator()(bool b) { return std::to_string(b); }
  std::string operator()(int n) { return std::to_string(n); }
  std::string operator()(double x) { return std::to_string(x); }
  std::string operator()(std::shared_ptr<CallFrame>) {
    assert(false);
    return "";
  }

  template <typename T> std::string operator()(std::shared_ptr<T> t) {
    return *t;
  }
};

MALType::operator std::string() const {
  return std::visit(StringVisitor{}, data);
}
