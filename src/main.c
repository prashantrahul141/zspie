#include "app.h"
#include "external/log.h"
#include "vm.h"

int main(int argc, const char *argv[]) {
  log_set_quiet(false);
  init_vm();

  handle_cli(argc, argv);

  free_vm();
  return EXIT_SUCCESS;
}
