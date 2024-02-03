#include "external/log.h"
#include "scanner.h"
#include <stdbool.h>
#include <stdio.h>

void compile(const char *source) {
  log_info("compiling source=\n%s", source);
  init_scanner(source);

  size_t line = -1;
  while (true) {
    Token token = scan_token();
    if (token.line != line) {
      line = token.line;
      log_info("%4zu ", line);
    } else {
      log_info("   | ");
    }

    log_debug("Scanned token type=%d", token.type);

    log_info("%2d '%.*s'", token.type, (int)token.length, token.start);

    if (token.type == TOKEN_EOF) {
      break;
    }
  }
}
