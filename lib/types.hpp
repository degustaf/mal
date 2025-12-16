#pragma once

#include "mal.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "bytecode.hpp"

struct MALType;

typedef std::monostate MALNil;

struct MALList {
  MALList() : data(){};
  MALList(size_t n) : data() { data.reserve(n); };
  operator std::string();

  inline auto begin() { return data.begin(); };
  inline auto begin() const { return data.begin(); };
  inline auto end() { return data.end(); };
  inline auto end() const { return data.end(); };
  inline auto empty() const { return data.empty(); };
  inline auto size() const { return data.size(); };

  std::vector<MALType> data;
};

struct MALVector {
  MALVector() : data(){};
  MALVector(size_t n) : data() { data.reserve(n); };
  operator std::string();

  inline auto begin() { return data.begin(); };
  inline auto begin() const { return data.begin(); };
  inline auto end() { return data.end(); };
  inline auto end() const { return data.end(); };
  inline auto empty() const { return data.empty(); };
  inline auto size() const { return data.size(); };
  inline const MALType &operator[](size_t n) const { return data[n]; };
  inline MALType &operator[](size_t n) { return data[n]; };

  std::vector<MALType> data;
};

struct MALMap {
  MALMap() : data(){};
  MALMap(size_t n) : data() { data.reserve(n); };
  operator std::string();

  std::unordered_map<std::string, MALType> data;
};

struct MALSymbol {
  MALSymbol(const char *ptr, int len) : symbol(ptr, ptr + len){};
  MALSymbol(const std::string &symbol) : symbol(symbol){};
  operator std::string() const;

  std::string symbol;
};

struct MALKeyword {
  MALKeyword(const char *ptr, int len) : keyword(ptr, ptr + len){};
  MALKeyword(const std::string &keyword) : keyword(keyword){};
  operator std::string() const;

  std::string keyword;
};

struct MALString {
  MALString(const std::string &str) : str(str){};
  MALString(const char *ptr, int len) : str(ptr, ptr + len){};

  operator std::string() const;
  std::string str;
};

struct MALCFunc {
  MALCFunc(CFunction fn, std::string_view name) : fn(fn), name(name){};

  operator std::string() const;

  CFunction fn;
  std::string name;
};

struct MALError {
  MALError(const std::string &msg) : msg(msg){};
  MALError(const char *msg) : msg(msg){};

  operator std::string() const;
  std::string msg;
};

struct CallFrame;

struct MALType {
  operator std::string() const;

  std::variant<MALNil, bool, int, double, std::shared_ptr<MALList>,
               std::shared_ptr<MALVector>, std::shared_ptr<MALMap>,
               std::shared_ptr<MALSymbol>, std::shared_ptr<MALKeyword>,
               std::shared_ptr<MALString>, std::shared_ptr<MALCFunc>,
               std::shared_ptr<CallFrame>>
      data;
};

struct Iterator {
  template <typename T> std::array<const MALType *, 2> operator()(T &) {
    return {nullptr, nullptr};
  }
  std::array<const MALType *, 2>
  operator()(const std::shared_ptr<MALVector> &v) {
    return {&(*v->begin()), &(*v->end())};
  };
  std::array<const MALType *, 2> operator()(const std::shared_ptr<MALList> &l) {
    return {&(*l->begin()), &(*l->end())};
  };
};

struct CallFrame {
  MALType fn;
  std::vector<byteCode>::const_iterator parent_ip;
  ptrdiff_t stack_offset;
};
