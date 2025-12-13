#pragma once

#include "types.hpp"

#include <memory>
#include <string>

enum struct TokenType {
  None,
  LeftParen,
  RightParen,
  LeftBrace,
  RightBrace,
  LeftBracket,
  RightBracket,
  Meta,
  Deref,
  Quote,
  QuasiQuote,
  Unquote,
  SpliceUnquote,
  NIL,

  String,
  Identifier,

  EOFToken,
  BadEscape
};

struct Token {
  TokenType type;
  const char *start;
  int length;
};

struct Scanner {
  Scanner(const std::string &str)
      : error(nullptr), data(str), current(data.data()) {}
  Token scan(void);
  Token peek(void);

  std::shared_ptr<MALError> error;

private:
  const std::string &data;
  const char *current;
};
