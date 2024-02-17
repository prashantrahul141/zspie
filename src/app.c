#include "external/log.h"
#include "vm.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void repl() {
  log_info("starting up repl");

  char line[1024];
  while (true) {
    printf(">>> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    interpret(line);
  }
}

static char *read_file(const char *filepath) {
  log_info("reading contents from file : %s", filepath);
  FILE *file = fopen(filepath, "rb");

  if (file == NULL) {
    fprintf(stderr, "Couldn't open file : '%s'\n", filepath);
    log_error("Couldn't open file : '%s'\n", filepath);
    exit(74);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char *file_buffer = (char *)malloc(file_size + 1);
  if (file_buffer == NULL) {
    fprintf(stderr, "Couldn't allocate memory for file\n");
    log_error("Couldn't allocate memory for file.");
    exit(74);
  }
  size_t bytes_read = fread(file_buffer, sizeof(char), file_size, file);

  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file : '%s'", filepath);
    log_error("Could not read file : '%s'", filepath);
    exit(74);
  }

  file_buffer[bytes_read] = '\0';
  fclose(file);

  log_debug("read file successfully, length=%d source=\n%s", file_size,
            file_buffer);

  return file_buffer;
}

static void run_file(const char *filepath) {
  log_debug("running from a file : %s", filepath);

  char *source = read_file(filepath);
  InterpretResult result = interpret(source);

  free(source);

  if (result == INTERPRET_COMPILE_ERROR) {
    log_error("Found compile error exiting exit code with 65");
    exit(65);
  }

  if (result == INTERPRET_RUNTIME_ERROR) {
    log_error("Found runtime error exiting exit code with 70");
    exit(70);
  }
}

void handle_cli(size_t argc, const char *argv[]) {
  log_info("Handling cli");
  log_info("arg=%d  argv:", argc);
  for (size_t i = 0; i < argc; i++) {
    log_debug("[ %s ]", argv[i]);
  }

  if (argc == 1) {
    // run repl.
    repl();
  } else if (argc == 2) {
    // run from file.
    run_file(argv[1]);
  } else {
    fprintf(stderr,
            "\033[1mZspie\033[0m - Stack based VM, interpreter, written "
            "completely in C."
            "\n"
            "\n"
            "\033[1mUsage:\033[0m zspie [filepath]"
            "\n"
            "\n"
            "\033[1mOptions\033[0m:"
            "\n"
            "    repl - Run the interpreter without any arguments to "
            "open live repl."
            "\n"
            "    filepath - Provide path to a zpe file to compile and run it."
            "\n");

    exit(64); //
  }
}
