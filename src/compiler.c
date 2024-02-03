#include "external/log.h"
#include "scanner.h"
#include <stdbool.h>
#include <stdio.h>

void compile(const char *source) {
  log_debug("compiling source=%s", source);
  init_scanner(source);

  size_t line = -1;
  while (true) {
    Token token = scan_token();
    if (token.line != line) {
      line = token.line;
      printf("%4zu", line);
    } else {
      printf("   | ");
    }

    printf("%2d %*s", token.type, (int)token.length, token.start);

    if (token.type == TOKEN_EOF) {
      break;
    }
  }
}
