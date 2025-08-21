#include <am.h>
#include <rtthread.h>
#include <klib-macros.h>
#include <klib.h>

// #define AM_APPS


#define RT_HW_HEAP_BEGIN heap.start
#define RT_HW_HEAP_END heap.end

#ifdef AM_APPS
#define AM_APPS_HEAP_SIZE 0x1000
Area am_apps_heap = {}, am_apps_data = {}, am_apps_bss = {};
#endif


uint8_t * am_apps_data_content = NULL;

void rt_hw_board_init() {
  // int rt_hw_uart_init(void);
  // rt_hw_uart_init();

#ifdef RT_USING_HEAP
  /* initialize memory system */
  rt_system_heap_init(RT_HW_HEAP_BEGIN, RT_HW_HEAP_END);
#endif

  #ifdef AM_APPS
  uint32_t size = AM_APPS_HEAP_SIZE;
  void *p = NULL;
  for (; p == NULL && size != 0; size /= 2) { p = rt_malloc(size); }
  am_apps_heap = RANGE(p, p + size);
  extern char __am_apps_data_start, __am_apps_data_end;
  extern char __am_apps_bss_start, __am_apps_bss_end;
  am_apps_data = RANGE(&__am_apps_data_start, &__am_apps_data_end);
  am_apps_bss  = RANGE(&__am_apps_bss_start,  &__am_apps_bss_end);
  rt_kprintf("am-apps.data.start = 0x%x, am-apps.data.end = 0x%x\n",
      am_apps_data.start, am_apps_data.end);
  rt_kprintf("am-apps.bss.start = 0x%x, am-apps.bss.end = 0x%x\n",
      am_apps_bss.start, am_apps_bss.end);
  rt_kprintf("am-apps.data.size = %ld, am-apps.bss.size = %ld\n",
      am_apps_data.end - am_apps_data.start, am_apps_bss.end - am_apps_bss.start);

  uint32_t data_size = am_apps_data.end - am_apps_data.start;
  if (data_size != 0) {
    am_apps_data_content = rt_malloc(data_size);
    assert(am_apps_data_content != NULL);
  }
  memcpy(am_apps_data_content, am_apps_data.start, data_size);
  #endif

#ifdef RT_USING_CONSOLE
  /* set console device */
  #ifdef RT_USING_DEVICE
  rt_console_set_device("uart");
  #endif
#endif /* RT_USING_CONSOLE */

#ifdef RT_USING_COMPONENTS_INIT
  rt_components_board_init();
#endif

#ifdef RT_USING_HEAP
  rt_kprintf("heap: [0x%08x - 0x%08x]\n", (rt_ubase_t) RT_HW_HEAP_BEGIN, (rt_ubase_t) RT_HW_HEAP_END);
#endif
  
}

int am_main() {
  ioe_init();
  extern void __am_cte_init();
  __am_cte_init();
  extern int entry(void);
  entry();
  return 0;
}

void rt_hw_console_output(const char *str){
  const char *p = str;
  while (*p != '\0')
  {
    io_write(AM_UART_TX, *p);
    p++;
  }
}
char rt_hw_console_getchar(void)
{
  uint32_t data = io_read(AM_UART_RX).data;
  return data;
}