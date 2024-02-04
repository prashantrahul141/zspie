#include "app.h"
#include "external/log.h"
#include "vm.h"

int main(int argc, const char *argv[]) {

  // set logging dependent on type of build.
  // ZSPIE_DEBUG_MODE is set in cmake only in debug mode.
#ifdef ZSPIE_DEBUG_MODE
  log_set_quiet(false);
#else
  log_set_quiet(true);
#endif /* if ZSPIE_DEBUG_MODE */

  // initialise zspie VM.
  init_vm();

  // handle command line arguments.
  handle_cli(argc, argv);

  // free vm
  free_vm();

  return EXIT_SUCCESS;
}
