#include "chunk.h"
#include "common.h"
#include "external/log.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#include <stdint.h>
#include <string.h>

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
typedef void (*ParseFn)(bool can_assign);

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

typedef struct {
  Token name;
  int depth;
} Local;

/*
 * State we need to keep track of in the compiler.
 */
typedef struct {
  Local locals[UINT8_COUNT];
  int local_count;
  int scope_depth;
} Compiler;

// Global module level to avoid passing parser around using parameters and
// pointers
Parser parser;
// Compilation state at runtime.
Compiler *current_cs = NULL;
// currently compiling chunk
Chunk *compiling_chunk;

// little helper function.
static Chunk *current_chunk() { return compiling_chunk; }

/*
 * Init a compiler state.
 */
static void init_compiler(Compiler *compiler) {
  log_debug("init compiler state");
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  current_cs = compiler;
}

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
 * increments depth.
 */
static void begin_scope() {
  log_trace("begin scope : line=%d", &parser.current.line);
  current_cs->scope_depth++;
}

/*
 * decrements depth.
 */
static void end_scope() {
  log_trace("end scope : line=%d", &parser.current.line);
  current_cs->scope_depth--;
  // delete local declarations.
  // untill
  // 1. we reach the end of all declarations.
  log_trace("deleting local variables.", &parser.current.line);
  while (current_cs->local_count > 0 &&
         // 2. from the entire scope.
         current_cs->locals[current_cs->local_count - 1].depth >
             current_cs->scope_depth) {
    emit_byte(OP_POP);
    current_cs->local_count--;
  }
}

/*
 * Gets ParseRule for a type of token.
 * @param type - type of token.
 */
static ParseRule *get_rule(TokenType type);

// declaration of all statement parsing functions.
static void expression_statement();
static void print_statement();
static void statement();
static void let_declaration();
static void declaration();

/*
 * Makes a constant
 */
static uint8_t indentifier_constant(Token *token) {
  return make_constant(OBJ_VAL(copy_string(token->start, token->length)));
}

/*
 * Adds locals to current scope
 */
static void add_local(Token name) {
  log_trace("adding token=%.*s at line= to locals.", name.length, name.start,
            name.line);
  if (current_cs->local_count == UINT8_COUNT) {
    error("Too many local variables in current scope.");
    return;
  }

  Local *local = &current_cs->locals[current_cs->local_count++];
  local->name = name;
  // we dont define the depth, it is kindof used as token
  // to mark variables uninitialised.
  local->depth = -1;
}

/*
 * Matches identifier names.
 */
static bool identifier_equal(Token *a, Token *b) {
  log_trace("checking for identifier equality a=%.*s    b=%.*s at line=%d",
            a->length, a->start, b->length, b->start, a->line);
  if (a->length != b->length) {
    return false;
  }
  return memcmp(a->start, b->start, a->length) == 0;
}

/*
 * Resolves a local variable.
 */
static int resolve_local(Compiler *compiler_state, Token *name) {
  log_trace("resolving a local for token=%.*s at line=%d", name->length,
            name->start, name->line);
  for (int i = compiler_state->local_count - 1; i >= 0; i--) {
    Local *local = &compiler_state->locals[i];
    if (identifier_equal(name, &local->name)) {
      if (local->depth == -1) {
        error("Reference to an undefined variable.");
      }
      log_debug("found match for local at index=%d", i);
      return i;
    }
  }

  log_trace("didnt found match for local.");
  return -1;
}

/*
 * local variables.
 */
static void declare_variable() {
  log_trace("declarating a local variable.");
  if (current_cs->scope_depth == 0) {
    log_trace("in global scope returning.");
    return;
  }

  Token *name = &parser.previous;
  // check if the variable already exist in current scope.
  for (int i = current_cs->local_count - 1; i >= 0; i--) {
    Local *local = &current_cs->locals[i];

    // break
    // 1. reached at -1 scope,
    // 2. "local's" index is less than current comp state.
    if (local->depth != -1 && local->depth < current_cs->scope_depth) {
      break;
    }

    // matching identifier.
    if (identifier_equal(name, &local->name)) {
      log_error("variable already exist in the current scope.");
      error("Redeclaration of local variable.");
    }
  }
  add_local(*name);
}

/*
 * parses variable indentifier.
 */
static uint8_t parse_variable(const char *error_message) {
  log_trace("parsing variable with error message=%d", error_message);
  consume(TOKEN_IDENTIFIER, error_message);
  declare_variable();

  // if we're in a scope, we return, we dont care about the
  // index of locals because they are not lookedup by index.
  if (current_cs->scope_depth > 0) {
    return 0;
  }
  return indentifier_constant(&parser.previous);
}

/*
 * This marks the variable as initialised by settings it depth value.
 */
static void mark_initialized() {
  current_cs->locals[current_cs->local_count - 1].depth =
      current_cs->scope_depth;
}

/*
 * Defines a variable
 */
static void define_variable(uint8_t global) {
  log_trace("defining variable global=%c", global);
  if (current_cs->scope_depth > 0) {
    mark_initialized();
    return;
  }
  emit_bytes(OP_DEFINE_GLOBAL, global);
}

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
  bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(can_assign);

  // consuming infix expression.
  while (precedence <= get_rule(parser.current.type)->precedence) {
    advance();
    ParseFn infix_rule = get_rule(parser.previous.type)->infix;
    // call infix expression's parsefn rule.
    infix_rule(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment.");
  }
}

/*
 * Parses binary expressions.
 */
static void binary(bool can_assign) {
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
static void literal(bool can_assign) {
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
static void number(bool can_assign) {
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
static void unary(bool can_assign) {
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
static void grouping(bool can_assign) {
  log_trace("parsing grouping expression");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}
/*
 * Parses string literals.
 */
static void string(bool can_assign) {
  emit_constant(OBJ_VAL(
      copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void synchronize() {
  parser.panic_mode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) {
      return;
    }

    switch (parser.current.type) {
    case TOKEN_CLASS:
    case TOKEN_FN:
    case TOKEN_LET:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;

    default:;
    }
  }

  advance();
}

/*
 * Retrives named variables.
 */
static void named_variable(Token *token, bool can_assign) {
  uint8_t arg = indentifier_constant(token);

  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(OP_SET_GLOBAL, arg);
  } else {
    emit_bytes(OP_GET_GLOBAL, arg);
  }
}

/*
 * Retrives variables.
 */
static void variable(bool can_assign) {
  named_variable(&parser.previous, can_assign);
}

/*
 * parses let variables declaration.
 */
static void let_declaration() {
  uint8_t global = parse_variable("Expected variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NULL);
  }

  consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

  define_variable(global);
}

/*
 * Statement wrapper for expressions.
 */
static void expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
  emit_byte(OP_POP);
}

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
  } else {
    expression_statement();
  }
}

/*
 * Parses declarations
 */

static void declaration() {
  if (match(TOKEN_LET)) {
    let_declaration();
  } else {
    statement();
  }

  if (parser.panic_mode) {
    synchronize();
  }
}

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
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
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

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_EOF, "Expected end of expression.");
  end_compiler();

  return !parser.has_error;
}
