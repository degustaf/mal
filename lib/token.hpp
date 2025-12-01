#pragma once

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
  Scanner(const std::string &str) : data(str), current(data.data()) {}
  Token scan(void);
  Token peek(void);

private:
  const std::string &data;
  const char *current;
};
