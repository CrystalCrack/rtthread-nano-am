#include <rtthread.h>

static int hello() {
  rt_kprintf("Hello RISC-V!\n");
  return 0;
}
INIT_ENV_EXPORT(hello);
