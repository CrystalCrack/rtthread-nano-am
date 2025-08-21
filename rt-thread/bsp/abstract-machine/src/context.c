#include <am.h>
#include <rtthread.h>
#include <klib-macros.h>
#include <stddef.h> // for offsetof
#define STACK_SIZE (4096 * 8)

// 调试开关，可以通过注释/取消注释来控制调试输出
// #define DEBUG_CONTEXT_SWITCH

#ifdef DEBUG_CONTEXT_SWITCH
#define DEBUG_PRINT(fmt, ...) rt_kprintf("[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) \
  do                          \
  {                           \
  } while (0)
#endif

#define NAMEINIT(key) [AM_KEY_##key] = #key,
static const char *names[] = {
    AM_KEYS(NAMEINIT)};

void wrapper(void *args)
{
  rt_ubase_t *stack_top = args;
  void *tentry = (void *)*(stack_top);
  stack_top--;
  void *parameter = (void *)*(stack_top);
  stack_top--;
  void *texit = (void *)*(stack_top);
  DEBUG_PRINT("[WRAPPER] tentry: %p(%p), parameter: %p(%p), texit: %p(%p)\n", tentry, stack_top+2, parameter, stack_top+1, texit, stack_top);
  ((void (*)(void *))tentry)(parameter);
  ((void (*)(void))texit)();
}

static Context *ev_handler(Event e, Context *c)
{
  switch (e.event)
  {
  case EVENT_YIELD:
  {
    rt_thread_t current = rt_thread_self();

    if(current->user_data){
      Context **from = (Context **)current->user_data;
      *from = c;
    }
    c = (Context *)current->sp;

    break;
  }
  case EVENT_IRQ_TIMER:
  {
    rt_tick_increase();
    break;
  }
  case EVENT_IRQ_IODEV:
  {
    AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
    if (ev.keycode == AM_KEY_DOWN)
      break;
    if (ev.keydown)
      rt_kprintf("%s", names[ev.keycode]);
    break;
  }
  default:
    rt_kprintf("Unhandled event ID = %d\n", e.event);
    RT_ASSERT(0);
  }
  return c;
}

void __am_cte_init()
{
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to)
{
  #ifdef DEBUG_CONTEXT_SWITCH
  rt_thread_t current = rt_thread_self();
  // to是指向线程sp字段的地址，需要通过偏移计算出线程指针
  rt_thread_t to_thread = (rt_thread_t)((char *)to - offsetof(struct rt_thread, sp));
  DEBUG_PRINT("[SWITCH_TO] current: %s(sp:%08x) -> to: %s(sp:%08x)\n",
              current ? current->name : "NULL",
              current ? current->sp : 0,
              to_thread ? to_thread->name : "NULL",
              to_thread ? to_thread->sp : 0);
  #endif
  yield();
  DEBUG_PRINT("[SWITCH_TO] back: %s\n",
              current ? current->name : "NULL");
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to)
{
  rt_thread_t current = rt_thread_self();

  #ifdef DEBUG_CONTEXT_SWITCH
  // from和to是指向线程sp字段的地址，需要通过偏移计算出线程指针
  // sp字段在线程结构体中的偏移可以通过container_of宏计算
  rt_thread_t from_thread = (rt_thread_t)((char *)from - offsetof(struct rt_thread, sp));
  rt_thread_t to_thread = (rt_thread_t)((char *)to - offsetof(struct rt_thread, sp));
  DEBUG_PRINT("[SWITCH] from: %s(sp:%08x) -> to: %s(sp:%08x)\n",
              from_thread ? from_thread->name : "NULL",
              from_thread ? from_thread->sp : 0,
              to_thread ? to_thread->name : "NULL",
              to_thread ? to_thread->sp : 0);
  #endif

  rt_uint32_t user_data_temp = current->user_data;
  current->user_data = from;
  yield();

  DEBUG_PRINT("[SWITCH] back: %s\n",
              current ? current->name : "NULL");

  current->user_data = user_data_temp;
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread)
{
  RT_ASSERT(0);
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit)
{
  // align
  rt_uint8_t *aligned_stack_addr;
  aligned_stack_addr = (rt_uint8_t *)((uintptr_t)stack_addr & (~(sizeof(uintptr_t) - 1)));

  // init stack,
  // stack sequence: [stack_bottom] context tentry parameter texit [stack_top]
  Area stack;
  stack.start = aligned_stack_addr - STACK_SIZE;
  stack.end = aligned_stack_addr;

  rt_ubase_t *stack_top = (rt_ubase_t *)(stack.end - sizeof(Context)) - 1;
  Context *cp = kcontext(stack, wrapper, (void *)stack_top);
  *(stack_top) = (rt_ubase_t)tentry;
  stack_top--;
  *(stack_top) = (rt_ubase_t)parameter;
  stack_top--;
  *(stack_top) = (rt_ubase_t)texit;
  DEBUG_PRINT("[STACK_INIT] tentry: %p(%p), parameter: %p(%p), texit: %p(%p)\n", tentry, stack_top+2, parameter, stack_top+1, texit, stack_top);
  return (rt_uint8_t *)cp;
}
