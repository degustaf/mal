#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct MALType;

struct MALList {
  MALList() : data(){};
  operator std::string();

  std::vector<MALType> data;
};

struct MALVector {
  MALVector() : data(){};
  operator std::string();

  std::vector<MALType> data;
};

struct MALMap {
  MALMap() : data(){};
  operator std::string();

  std::unordered_map<std::string, MALType> data;
};

struct MALSymbol {
  MALSymbol(const char *ptr, int len) : symbol(ptr, ptr + len){};
  MALSymbol(const std::string &symbol) : symbol(symbol){};
  operator std::string() const;

  std::string symbol;
};

struct MALString {
  MALString(const std::string &str) : str(str){};
  MALString(const char *ptr, int len) : str(ptr, ptr + len){};

  operator std::string() const;
  std::string str;
};

struct MALError {
  MALError(const std::string &msg) : msg(msg){};

  operator std::string() const;
  std::string msg;
};

typedef std::monostate MALNil;

struct MALType {
  operator std::string() const;

  std::variant<MALNil, bool, int, double, std::shared_ptr<MALList>,
               std::shared_ptr<MALVector>, std::shared_ptr<MALMap>,
               std::shared_ptr<MALSymbol>, std::shared_ptr<MALString>,
               std::shared_ptr<MALError>>
      data;
};
