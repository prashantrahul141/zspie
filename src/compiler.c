#include "chunk.h"
#include "debug.h"
#include "external/log.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

/*
 * Struct to store state of our parser.
 */
typedef struct {
  // previously consumed token.
  Token previous;
  // next token.
  Token current;
  // flag to keep if any parsing errrors.
  bool has_error;
  // flag to see if the parser is in panic mode.
  bool panic_mode;
} Parser;

/*
 * All precedence values in zspie.
 */
typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

/*
 * Short hand for non returning functions.
 */
typedef void (*ParseFn)();

/*
 * Struct to store a parse rule.
 */
typedef struct {
  // function which handles prefix.
  ParseFn prefix;
  // function which handles infix.
  ParseFn infix;
  // precedence value of this parse rule.
  Precedence precedence;
} ParseRule;

// Global module level to avoid passing parser around using parameters and
// pointers
Parser parser;
// currently compiling chunk
Chunk *compiling_chunk;

// little helper function.
static Chunk *current_chunk() { return compiling_chunk; }

/*
 * Zspie core way of reporting parsing errors to users.
 */
static void error_at(Token *token, const char *message) {
  // dont repot if we're already in panic mode.
  if (parser.panic_mode) {
    return;
  }

  // set panic mode.
  parser.panic_mode = true;

  fprintf(stderr, "[line %zu] Error", token->line);
  log_error("[line %zu] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
    log_error(" at end");
  } else if (token->type == TOKEN_ERROR) {
    //
  } else {
    fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    log_error(" at '%.*s'", (int)token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  log_error(": %s\n", message);
  parser.has_error = true;
}

/*
 * Little helper to report error in previous token.
 */
static void error(const char *message) { error_at(&parser.previous, message); }

/*
 * Little helper to report error in current token.
 */
static void error_at_current(const char *message) {
  error_at(&parser.current, message);
}

/*
 * stores current token in previous, consume new token and stores it.
 */
static void advance() {
  log_trace("advancing parser.current=%d, parser.previous=%d", parser.current,
            parser.previous);
  parser.previous = parser.current;
  while (true) {
    parser.current = scan_token();
    log_trace("current token type=%d", parser.current.type);
    if (parser.current.type != TOKEN_ERROR) {
      break;
    }

    error_at_current(parser.current.start);
  }
}

/*
 * Only advance if the given type of token matches the current type of token.
 * otherwise report error message.
 * @param type - TokenType
 * @param message - Error message
 */
static void consume(TokenType type, const char *message) {
  log_trace("consuming type=%d with message=%s", type, message);

  if (parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}

/*
 * Helper function to check if the current token if of given type.
 */
static inline bool check(TokenType type) { return parser.current.type == type; }

/*
 * Helper function to consume token if only it matches the given token.
 * also returns the result of matching
 */
static bool match(TokenType type) {
  if (!check(type)) {
    return false;
  }
  advance();
  return true;
}

/*
 * Writes a byte to current chunk.
 * @param byte - The byte to write.
 */
static void emit_byte(uint8_t byte) {
  log_debug("emiting byte=%d", byte);
  write_chunk(current_chunk(), byte, parser.previous.line);
}

/*
 * little helper to write two bytes in current chunk.
 */
static void emit_bytes(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

/*
 * Emits OP_RETURN.
 */
static void emit_return() { emit_byte(OP_RETURN); }

/*
 * creates a new constant, for the Chunk.
 * @param value of the constant.
 * @return index of the constant in stack.
 */
static uint8_t make_constant(Value value) {
  log_debug("making constant with value=%lf", value);
  size_t constant_index = add_constant_to_chunk(current_chunk(), value);

  if (constant_index > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant_index;
}

/*
 * Emmits OP_CONSTANT
 * @param value - the value to make constant.
 */
static void emit_constant(Value value) {
  emit_bytes(OP_CONSTANT, make_constant(value));
}

/*
 * called after executing chunk for cleanup.
 */
static void end_compiler() {
  emit_return();
// some logging .
#ifdef ZSPIE_DEBUG_MODE
  if (!parser.has_error) {
    disassemble_chunk(current_chunk(), "code");
  }
#endif // !ZSPIE_DEBUG_MODE
}

/*
 * Gets ParseRule for a type of token.
 * @param type - type of token.
 */
static ParseRule *get_rule(TokenType type);

/*
 * Consumes token untill reached token with higher precedence.
 * @param precedence - precedence to consider.
 */
static void parse_precedence(Precedence precedence) {
  log_trace("parsing precedence with precedence=%d", precedence);

  // consuming prefix of the token.
  advance();
  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("Expected expression.");
    return;
  }

  // calling prefix ParseFn.
  prefix_rule();

  // consuming infix expression.
  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();
    ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    // call infix expression's parsefn rule.
    infix_rule();
  }
}

/*
 * Parses binary expressions.
 */
static void binary() {
  log_trace("parsing binary expression");

  TokenType operator_type = parser.previous.type;
  ParseRule *rule = get_rule(operator_type);
  parse_precedence((Precedence)(rule->precedence + 1));

  switch (operator_type) {
  case TOKEN_PLUS:
    emit_byte(OP_ADD);
    break;

  case TOKEN_MINUS:
    emit_byte(OP_SUBTRACT);
    break;

  case TOKEN_STAR:
    emit_byte(OP_MULTIPLY);
    break;

  case TOKEN_SLASH:
    emit_byte(OP_DIVIDE);
    break;

  default:
    return; // unreachable
  }
}

/*
 * Parses literal.
 */
static void literal() {
  log_fatal("parsing literal");
  switch (parser.previous.type) {
  case TOKEN_TRUE:
    log_fatal("matched true");
    emit_byte(OP_TRUE);
    break;

  case TOKEN_FALSE:

    log_fatal("matched false");
    emit_byte(OP_FALSE);
    break;

  case TOKEN_NULL:

    log_fatal("matched null");
    emit_byte(OP_NULL);
    break;

  default:
    return;
  }
}

/*
 * Parses number literals.
 */
static void number() {
  log_trace("parsing number expression");
  double value = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(value));
}

/*
 * Parses expression.
 */
static void expression() {
  log_trace("parsing expression");
  parse_precedence(PREC_ASSIGNMENT);
}

/*
 * Parses unary expression.
 */
static void unary() {
  log_trace("parsing unary expression");

  TokenType operator_type = parser.previous.type;

  parse_precedence(PREC_UNARY);

  switch (operator_type) {
  case TOKEN_BANG_EQUAL:
    emit_bytes(OP_EQUAL, OP_NOT);
    break;

  case TOKEN_EQUAL_EQUAL:
    emit_byte(OP_EQUAL);
    break;

  case TOKEN_GREATER:
    emit_byte(OP_GREATER);
    break;

  case TOKEN_GREATER_EQUAL:
    emit_bytes(OP_LESS, OP_NOT);
    break;

  case TOKEN_LESS:
    emit_byte(OP_LESS);
    break;

  case TOKEN_LESS_EQUAL:
    emit_bytes(OP_GREATER, OP_NOT);
    break;

  case TOKEN_BANG:
    emit_byte(OP_NOT);
    break;

  case TOKEN_MINUS:
    emit_byte(OP_NEGATE);
    break;

  default:
    return; // unreachable.
  }
}

/*
 * parses grouping expression.
 */
static void grouping() {
  log_trace("parsing grouping expression");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}
/*
 * Parses string literals.
 */
static void string() {
  emit_constant(OBJ_VAL(
      copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

// declaration of all statement parsing functions.
static void print_statement();
static void statement();
static void declaration();

/*
 * Parses print statements.
 */
static void print_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
  emit_byte(OP_PRINT);
}

/*
 * parses statements
 */

static void statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  }
}

/*
 * Parses declarations
 */

static void declaration() { statement(); }

/*
 * This stores all the Parse rules for Zspie.
 *
 */
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NULL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_LET] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

// gets rule.
static ParseRule *get_rule(TokenType type) { return &rules[type]; }

/*
 * Top level function to start compiling a source and writing instructions to
 * given chunk.
 * @param source - source strings.
 * @param chunk - pointer to the chunk to write to.
 */
bool compile(const char *source, Chunk *chunk) {
  log_info("compiling source=\n%s", source);

  init_scanner(source);
  compiling_chunk = chunk;

  parser.has_error = false;
  parser.panic_mode = false;

  advance();
  expression();

  consume(TOKEN_EOF, "Expected end of expression.");
  end_compiler();

  return !parser.has_error;
}
