#ifndef ZSPIE_SCANNER_H_
#define ZSPIE_SCANNER_H_

#include "stdlib.h"

/*
 * All types of tokens in zspie.
 */
typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_SEMICOLON,
  TOKEN_SLASH,
  TOKEN_STAR,

  // One or two character tokens.
  TOKEN_BANG,
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_LESS,
  TOKEN_LESS_EQUAL,

  // Literals.
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,

  // Keywords.
  TOKEN_AND,
  TOKEN_CLASS,
  TOKEN_ELSE,
  TOKEN_FALSE,
  TOKEN_FOR,
  TOKEN_FN,
  TOKEN_IF,
  TOKEN_NULL,
  TOKEN_OR,
  TOKEN_PRINT,
  TOKEN_RETURN,
  TOKEN_SUPER,
  TOKEN_THIS,
  TOKEN_TRUE,
  TOKEN_LET,
  TOKEN_WHILE,

  TOKEN_ERROR,
  TOKEN_EOF
} TokenType;

/*
 * Struct to store info about one token.
 */
typedef struct {
  // type of the token.
  TokenType type;
  // lexem of the token.
  const char *start;
  // lemexe length.
  size_t length;
  // line number in the source string.
  size_t line;
} Token;

/*
 * Initiliases a scanner and its fields.
 */
void init_scanner(const char *source);

/*
 * Scans one token at a time and returns it.
 */
Token scan_token();

#endif // !ZSPIE_SCANNER_H_
