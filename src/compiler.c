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

typedef enum {
  TYPE_FUNCTION,
  TYPE_SCRIPT,
} FunctionType;

/*
 * State we need to keep track of in the compiler.
 */
typedef struct Compiler {
  struct Compiler *enclosing;
  ObjFunction *function;
  FunctionType type;
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
static Chunk *current_chunk() { return &current_cs->function->chunk; }

/*
 * Init a compiler state.
 */
static void init_compiler(Compiler *compiler, FunctionType type) {
  log_debug("init compiler state");

  compiler->enclosing = current_cs;
  compiler->function = NULL;
  compiler->type = type;
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  compiler->function = new_function();

  current_cs = compiler;

  if (type != TYPE_SCRIPT) {
    current_cs->function->name =
        copy_string(parser.previous.start, parser.previous.length);
  }

  Local *local = &current_cs->locals[current_cs->local_count++];
  local->depth = 0;
  local->name.start = "";
  local->name.length = 0;
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
 * emits loop instruction;
 */
static void emit_loop(int loop_start) {
  emit_byte(OP_LOOP);
  int offset = current_chunk()->count - loop_start + 2;
  if (offset > UINT16_MAX) {
    error("Loop body too large");
  }

  emit_byte((offset >> 8) & 0xff);
  emit_byte(offset & 0xff);
}

/*
 * Emits OP_RETURN.
 */
static void emit_return() {
  emit_byte(OP_NULL);
  emit_byte(OP_RETURN);
}

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
 * emits jump statements.
 */
static int emit_jump(uint8_t instruction) {
  emit_byte(instruction);
  emit_byte(0xff);
  emit_byte(0xff);
  return current_chunk()->count - 2;
}

/*
 * called after executing chunk for cleanup.
 */
static ObjFunction *end_compiler() {
  emit_return();
  ObjFunction *function = current_cs->function;

// some logging .
#ifdef ZSPIE_DEBUG_MODE
  if (!parser.has_error) {
    disassemble_chunk(current_chunk(), function->name != NULL
                                           ? function->name->chars
                                           : "<script>");
  }
#endif // !ZSPIE_DEBUG_MODE
  //
  current_cs = current_cs->enclosing;

  return function;
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
  log_trace("making indentifier_constant");
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
  if (current_cs->scope_depth == 0) {
    return;
  }
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
 * gets the arguments passed to a function call.
 */
static uint8_t argument_list() {
  uint8_t args_count = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (args_count == 255) {
        error("Cannot have more than 255 arguments in a function call.");
      }
      args_count++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
  return args_count;
}

/*
 * Parses function calls.
 */
static void call(bool can_assign) {
  uint8_t args_count = argument_list();
  emit_bytes(OP_CALL, args_count);
}

/*
 * Parses 'blocks'
 */
static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

/*
 * compiles function signature and body
 */
static void function(FunctionType type) {
  Compiler compiler;
  init_compiler(&compiler, type);
  begin_scope();

  consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
  // parameters
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current_cs->function->arity++;
      if (current_cs->function->arity > 255) {
        error_at_current("Cannot have more than 255 function parameters.");
      }
      uint8_t constant = parse_variable("Expected parameter name.");
      define_variable(constant);
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expected ')' after function parameters.");
  consume(TOKEN_LEFT_BRACE, "Expected '{' after function signature.");

  block();
  ObjFunction *function = end_compiler();
  emit_bytes(OP_CONSTANT, make_constant(OBJ_VAL(function)));
}

/*
 * compiler functon declaration.
 */
static void fn_declaration() {
  uint8_t global = parse_variable("Expected function name.");
  mark_initialized();
  function(TYPE_FUNCTION);
  define_variable(global);
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
static void named_variable(Token name, bool can_assign) {
  uint8_t get_op, set_op;
  int arg = resolve_local(current_cs, &name);
  if (arg != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else {
    arg = indentifier_constant(&name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(set_op, (uint8_t)arg);
  } else {
    emit_bytes(get_op, (uint8_t)arg);
  }
}

/*
 * Retrives variables.
 */
static void variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
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
  log_trace("parsing print statement line=%d", parser.current.line);
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
  emit_byte(OP_PRINT);
}

/*
 * Compiles function return statements.
 */
static void return_statement() {
  if (current_cs->type == TYPE_SCRIPT) {
    error("Can't return from top level.");
  }

  if (match(TOKEN_SEMICOLON)) {
    emit_return();
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
    emit_byte(OP_RETURN);
  }
}

/*
 * int patches jump statements.
 */
static void patch_jump(int offset) {
  // -2 for jumping the operand of jump statement.
  int jump = current_chunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  current_chunk()->code[offset] = (jump >> 8) & 0xff;
  current_chunk()->code[offset + 1] = jump & 0xff;
}

/*
 * and logical operator
 */
static void and_(bool can_assign) {
  int end_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  parse_precedence(PREC_AND);
  patch_jump(end_jump);
}

/*
 * or logical operator
 */
static void or_(bool can_assign) {
  int else_jump = emit_jump(OP_JUMP_IF_FALSE);
  int end_jump = emit_jump(OP_JUMP);

  patch_jump(else_jump);
  emit_byte(OP_POP);

  parse_precedence(PREC_OR);
  patch_jump(end_jump);
}

/*
 * parses if statements
 */
static void if_statement() {
  log_trace("parsing if statement line=%d", parser.current.line);
  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression");

  int then_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();

  int else_jump = emit_jump(OP_JUMP);

  patch_jump(then_jump);
  emit_byte(OP_POP);

  // if there is a 'else' token after 'then' block.
  if (match(TOKEN_ELSE)) {
    statement();
  }

  patch_jump(else_jump);
}

/*
 * parses while statements
 */
static void while_statement() {
  int loop_start = current_chunk()->count;
  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' expression");

  int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();
  emit_loop(loop_start);

  patch_jump(exit_jump);
  emit_byte(OP_POP);
}

/*
 * parses for statements
 */
static void for_statement() {
  begin_scope();

  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'");

  if (match(TOKEN_SEMICOLON)) {
    // no initialiser
  } else if (match(TOKEN_LET)) {
    let_declaration();
  } else {
    expression_statement();
  }

  int loop_start = current_chunk()->count;
  int exit_jump = -1;

  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after expression.");

    exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int body_jump = emit_jump(OP_JUMP);
    int increment_start = current_chunk()->count;

    expression();
    emit_byte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after for expression");

    emit_loop(loop_start);
    loop_start = increment_start;
    patch_jump(body_jump);
  }

  statement();
  emit_loop(loop_start);

  if (exit_jump != -1) {
    patch_jump(exit_jump);
    emit_byte(OP_POP);
  }

  end_scope();
}

/*
 * parses statements
 */

static void statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else if (match(TOKEN_IF)) {
    if_statement();
  } else if (match(TOKEN_RETURN)) {
    return_statement();
  } else if (match(TOKEN_WHILE)) {
    while_statement();
  } else if (match(TOKEN_FOR)) {
    for_statement();
  } else {
    expression_statement();
  }
}

/*
 * Parses declarations
 */

static void declaration() {
  if (match(TOKEN_FN)) {
    fn_declaration();
  } else if (match(TOKEN_LET)) {
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
    [TOKEN_LEFT_PAREN] = {grouping, call, PREC_CALL},
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
    [TOKEN_AND] = {NULL, and_, PREC_AND},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NULL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or_, PREC_OR},
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
ObjFunction *compile(const char *source) {
  log_info("compiling source=\n%s", source);

  init_scanner(source);

  Compiler compiler;
  init_compiler(&compiler, TYPE_SCRIPT);

  parser.has_error = false;
  parser.panic_mode = false;

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction *function = end_compiler();
  return parser.has_error ? NULL : function;
}
