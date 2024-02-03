#include "scanner.h"
#include "external/log.h"
#include <string.h>

/*
 * Our scanner struct. holds state of the scanner.
 */
typedef struct {
  // pointer to start of the current scanning token.
  const char *start;
  // pointer to current checking character.
  const char *current;
  // current line number we're scanning.
  size_t line;
} Scanner;

// creating one module global scanner to avoid passing around scanner.
Scanner scanner;

void init_scanner(const char *source) {
  log_debug("initialising scanner..");
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

/*
 * module function to check if reached the end of the source code.
 */
static bool is_at_end() { return *scanner.current == '\0'; }

/*
 * makes a new token from the given type and using scanner's state.
 * @returns token the token created.
 */
static Token make_token(TokenType type) {
  Token token = {.type = type,
                 .start = scanner.start,
                 .length = (int)(scanner.current - scanner.start),
                 .line = scanner.line};

  return token;
}

/*
 * makes a new error token using scanner's state.
 * @param message string message of the token error.
 * @returns token the token created.
 */
static Token error_token(const char *message) {
  Token token = {.type = TOKEN_ERROR,
                 .start = message,
                 .length = strlen(message),
                 .line = scanner.line

  };

  return token;
}

/*
 * Consumes one character form the source code and returns it
 * @returns char the consumed char.
 */
static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

/*
 * Matches current char with the given char, only consumes it if it matches
 * otherwise doesnt consume it.
 * @param char - the matching char.
 * @returns match - if they match or not.
 */
static bool match(char expected) {
  if (is_at_end()) {
    return false;
  }

  if (*scanner.current != expected) {
    return false;
  }

  scanner.current++;
  return true;
}

/*
 * Returns the next character in source without consuming it.
 */
static char peek() { return *scanner.current; }

/*
 * checks if the given char is a number or not
 */
static bool is_digit(char c) { return c >= '0' && c <= '9'; }

/*
 * checks if the given char is ascii alphabetic or not
 */
static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

/*
 * Returns the next + 1character in source without consuming it.
 */
static char peek_next() {
  if (is_at_end()) {
    return '\0';
  }
  return scanner.current[1];
}

/*
 * Skips all whitespaces characters.
 */
static void skip_whitespaces() {
  while (true) {
    char c = peek();
    switch (c) {
    // all whitespace characters
    case ' ':
    case '\t':
    case '\r':
      advance();
      break;

    // newline character
    case '\n':
      scanner.line++;
      advance();
      break;

    // comments
    case '/':
      if (peek_next() == '/') {
        while (peek() != '\n' && !is_at_end()) {
          advance();
        }
      } else {
        return;
      }
      break;

    // non whitespace character.
    default:
      return;
    }
  }
}

/*
 * Scans a whole string.
 * @returns token - the scanned string token sca
 */

static Token scan_string() {
  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n') {
      scanner.line++;
    }
    advance();
  }

  if (is_at_end()) {
    return error_token("Unterminated string.");
  }

  // consuming the closing quote.
  advance();
  return make_token(TOKEN_STRING);
}

/*
 * Scans numbers
 */
static Token scan_number() {
  while (is_digit(peek())) {
    advance();
  }

  // the floating point part.
  if (peek() == '.' && is_digit(peek_next())) {
    while (is_digit(peek())) {
      advance();
    }
  }

  return make_token(TOKEN_NUMBER);
}
/*
 * little utility to check if the current token lexem matches the input string.
 * @param start - start of the strings inside scanner state.
 * @param length - length of the matching strings
 * @param expected - the expected string matching against.
 * @param type - TokenType to return if the matching succeeds
 */
static bool match_keyword(int start, int length, const char *expected,
                          TokenType type) {

  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, expected, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

/*
 * Determines the indentifiers type
 */
static TokenType identifier_type() {
  switch (scanner.start[0]) {
    // and
  case 'a':
    return match_keyword(1, 2, "nd", TOKEN_AND);

    // class
  case 'c':
    return match_keyword(1, 4, "lass", TOKEN_CLASS);

    // else
  case 'e':
    return match_keyword(1, 3, "lse", TOKEN_ELSE);

  // f
  case 'f': {
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      // false
      case 'a':
        return match_keyword(2, 3, "lse", TOKEN_FALSE);

      // for
      case 'o':
        return match_keyword(2, 1, "r", TOKEN_FOR);
      }

      // fn
      return match_keyword(1, 1, "n", TOKEN_FN);
    }
    break;
  }

    // if
  case 'i':
    return match_keyword(1, 1, "f", TOKEN_IF);

    // null
  case 'n':
    return match_keyword(1, 3, "ull", TOKEN_NULL);

    // or
  case 'o':
    return match_keyword(1, 1, "r", TOKEN_OR);

    // print
  case 'p':
    return match_keyword(1, 4, "rint", TOKEN_PRINT);

    // return
  case 'r':
    return match_keyword(1, 5, "eturn", TOKEN_RETURN);

    // super
  case 's':
    return match_keyword(1, 4, "uper", TOKEN_SUPER);

  case 't': {
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'h':
        return match_keyword(2, 2, "is", TOKEN_THIS);
      case 'r':
        return match_keyword(2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  }

    // let
  case 'l':
    return match_keyword(1, 2, "et", TOKEN_LET);

    // while
  case 'w':
    return match_keyword(1, 4, "hile", TOKEN_WHILE);
  }

  // keywords
  return TOKEN_IDENTIFIER;
}

/*
 * Scans identifiers
 */
static Token scan_identifier() {
  while (is_alpha(peek()) || is_digit(peek())) {
    advance();
  }

  return make_token(identifier_type());
}

/*
 * Scans one token at a time and returns it.
 * @returns token the scanned token.
 */
Token scan_token() {
  skip_whitespaces();

  scanner.start = scanner.current;

  // return an EOF token if reached the end of the source string.
  if (is_at_end()) {
    return make_token(TOKEN_EOF);
  }

  // current char.
  char c = advance();

  // indentifiers and keywords.
  if (is_alpha(c)) {
    return scan_identifier();
  }

  // numbers
  if (is_digit(c)) {
    return scan_number();
  }

  switch (c) {
  // (
  case '(':
    return make_token(TOKEN_LEFT_PAREN);

  // )
  case ')':
    return make_token(TOKEN_RIGHT_PAREN);

  // {
  case '{':
    return make_token(TOKEN_LEFT_BRACE);

  // }
  case '}':
    return make_token(TOKEN_RIGHT_BRACE);

  // ;
  case ';':
    return make_token(TOKEN_SEMICOLON);

  // ,
  case ',':
    return make_token(TOKEN_COMMA);

  // .
  case '.':
    return make_token(TOKEN_DOT);

  // -
  case '-':
    return make_token(TOKEN_MINUS);

  // +
  case '+':
    return make_token(TOKEN_PLUS);

  // /
  case '/':
    return make_token(TOKEN_SLASH);

  // *
  case '*':
    return make_token(TOKEN_STAR);

    // !
  case '!':
    // != or =
    return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);

  // =
  case '=':
    // == or =
    return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);

  // <
  case '<':
    // <= or <
    return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);

  // >
  case '>':
    // >= or >
    return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

    // strings!
  case '"':
    return scan_string();
  }

  return error_token("Unexpected character.");
}
