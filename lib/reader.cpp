#include "state.hpp"

#include "token.hpp"
#include "types.hpp"

#include <cassert>
#include <cctype>
#include <memory>
#include <string>

Token Scanner::scan(void) {
  auto ret = peek();
  current += ret.length;
  return ret;
}

Token Scanner::peek(void) {
  while (*current && (std::isspace(*current) || *current == ','))
    current++;

  switch (*current) {
  case 0:
    return Token{TokenType::EOFToken, nullptr, 0};
  case '(':
    return Token{TokenType::LeftParen, current, 1};
  case ')':
    return Token{TokenType::RightParen, current, 1};
  case '{':
    return Token{TokenType::LeftBrace, current, 1};
  case '}':
    return Token{TokenType::RightBrace, current, 1};
  case '[':
    return Token{TokenType::LeftBracket, current, 1};
  case ']':
    return Token{TokenType::RightBracket, current, 1};
  case '^':
    return Token{TokenType::Meta, current, 1};
  case '@':
    return Token{TokenType::Deref, current, 1};
  case '~':
    if (current[1] == '@')
      return Token{TokenType::SpliceUnquote, current, 2};
    return Token{TokenType::Unquote, current, 1};
  case '`':
    return Token{TokenType::QuasiQuote, current, 1};
  case '\'':
    return Token{TokenType::Quote, current, 1};
  case ';': {
    // This line is a comment
    int len = 1;
    while (current[len] && current[len] != '\n')
      len++;
    return Token{TokenType::None, current, len + 1};
  }
  case '"': {
    int len = 1;
    while (true) {
      switch (current[len]) {
      case 0:
        return Token{TokenType::EOFToken, nullptr, 0};
      case '"':
        len++;
        return Token{TokenType::String, current, len};
      case '\\':
        len++;
        switch (current[len]) {
        case 0:
          return Token{TokenType::EOFToken, nullptr, 0};
        case '"':
        case '\\':
        case 'n':
        case 'r':
        case 't':
          len++;
          break;
        default:
          return Token{TokenType::BadEscape, nullptr, 0};
        }
        break;
      default:
        len++;
      }
    }
  }
  default: {
    int len = 1;
    while (true) {
      switch (current[len]) {
      case 0:
      case '(':
      case ')':
      case '{':
      case '}':
      case '[':
      case ']':
      case '\'':
      case '"':
      case '`':
      case ',':
      case ';':
        return Token{TokenType::Identifier, current, len};
      default:
        if (std::isspace(current[len]))
          return Token{TokenType::Identifier, current, len};
        len++;
      }
    }
  }
  }
}

static MALType read_form(Scanner &scanner);

template <TokenType closer, typename T>
static inline MALType read_container(Scanner &scanner) {
  auto list = std::make_shared<T>();
  for (auto tok = scanner.peek();
       tok.type != closer && tok.type != TokenType::EOFToken;
       tok = scanner.peek()) {
    auto m = read_form(scanner);
    if (auto err = std::get_if<std::shared_ptr<MALError>>(&m.data); err) {
      return m;
    }
    list->data.push_back(m);
  }
  if (scanner.peek().type == TokenType::EOFToken) {
    return MALType{std::make_shared<MALError>("EOF")};
  }
  scanner.scan(); // Discard the close paren.
  return MALType{list};
}

static MALType read_list(Scanner &scanner) {
  return read_container<TokenType::RightParen, MALList>(scanner);
}

static MALType read_vec(Scanner &scanner) {
  return read_container<TokenType::RightBracket, MALVector>(scanner);
}

static MALType read_map(Scanner &scanner) {
  auto map = std::make_shared<MALMap>();
  for (auto tok = scanner.peek();
       tok.type != TokenType::RightBrace && tok.type != TokenType::EOFToken;
       tok = scanner.peek()) {
    auto key = read_form(scanner);
    if (auto err = std::get_if<std::shared_ptr<MALError>>(&key.data); err) {
      return key;
    }
    auto val = read_form(scanner);
    if (auto err = std::get_if<std::shared_ptr<MALError>>(&val.data); err) {
      return val;
    }
    map->data[key] = val;
  }
  if (scanner.peek().type == TokenType::EOFToken) {
    return MALType{std::make_shared<MALError>("EOF")};
  }
  scanner.scan(); // Discard the close paren.
  return MALType{map};
}

static MALType read_string(Scanner &scanner) {
  auto tok = scanner.scan();
  assert(tok.type == TokenType::String);
  if (tok.length > 1 && tok.start[tok.length - 1] == '"') {
    return MALType{std::make_shared<MALString>(tok.start, tok.length)};
  }
  return MALType{std::make_shared<MALError>("EOF")};
}

static MALType read_macro(Scanner &scanner, const std::string &symbol) {
  scanner.scan(); // pop the '

  auto list = std::make_shared<MALList>();
  list->data.push_back(MALType{std::make_shared<MALSymbol>(symbol)});
  auto m = read_form(scanner);
  if (auto err = std::get_if<std::shared_ptr<MALError>>(&m.data); err) {
    return m;
  }
  list->data.push_back(m);

  return MALType{list};
}

static MALType read_meta(Scanner &scanner) {
  scanner.scan(); // Pop off the ^

  auto list = std::make_shared<MALList>();
  list->data.push_back(MALType{std::make_shared<MALSymbol>("with-meta")});

  auto meta = read_form(scanner);
  if (auto err = std::get_if<std::shared_ptr<MALError>>(&meta.data); err) {
    return meta;
  }

  auto form = read_form(scanner);
  if (auto err = std::get_if<std::shared_ptr<MALError>>(&form.data); err) {
    return form;
  }
  list->data.push_back(form);
  list->data.push_back(meta);

  return MALType{list};
}

static MALType read_atom(Scanner &scanner) {
  auto tok = scanner.scan();
  if (isdigit(tok.start[0])) {
    size_t pos = 0;
    auto n = std::stoi(tok.start, &pos);
    assert(pos <= (size_t)tok.length);
    if (pos == (size_t)tok.length) {
      return MALType{n};
    }
    auto x = std::stod(tok.start, &pos);
    assert(pos <= (size_t)tok.length);
    if (pos == (size_t)tok.length) {
      return MALType{x};
    }
    return MALType{std::make_shared<MALError>("Bad number")};
  }
  return MALType{std::make_shared<MALSymbol>(tok.start, tok.length)};
}

static MALType read_form(Scanner &scanner) {
  auto tok = scanner.peek();
  switch (tok.type) {
  case TokenType::LeftParen:
    scanner.scan(); // pop the Paren off the scanner.
    return read_list(scanner);
  case TokenType::LeftBracket:
    scanner.scan(); // pop the Bracket off the scanner.
    return read_vec(scanner);
  case TokenType::LeftBrace:
    scanner.scan(); // pop the Brace off the scanner.
    return read_map(scanner);
  case TokenType::EOFToken:
    return MALType{std::make_shared<MALError>("EOF")};
  case TokenType::String:
    return read_string(scanner);
  case TokenType::Quote:
    return read_macro(scanner, "quote");
  case TokenType::QuasiQuote:
    return read_macro(scanner, "quasiquote");
  case TokenType::Unquote:
    return read_macro(scanner, "unquote");
  case TokenType::SpliceUnquote:
    return read_macro(scanner, "splice-unquote");
  case TokenType::Deref:
    return read_macro(scanner, "deref");
  case TokenType::Meta:
    return read_meta(scanner);
  default:
    return read_atom(scanner);
  }
  return MALType{};
}

void MALState::read_str(std::string &str) {
  auto scanner = Scanner(str);
  state->value = read_form(scanner);
}
