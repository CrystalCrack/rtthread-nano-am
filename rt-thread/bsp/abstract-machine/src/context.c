#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <rtthread.h>
#define STACK_SIZE (4096*8)

#define NAMEINIT(key)  [ AM_KEY_##key ] = #key,
static const char *names[] = {
  AM_KEYS(NAMEINIT)
};

void wrapper(void *args){
  rt_ubase_t *stack_top = args;
  void *tentry = (void*)*(stack_top);
  stack_top--;
  void *parameter = (void*)*(stack_top);
  stack_top--;
  void *texit = (void*)*(stack_top);
  ((void (*)(void *))tentry)(parameter);
  ((void (*)(void))texit)();
}


static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:{
      rt_thread_t current = rt_thread_self();
      c = *((Context**)current->user_data);
      // c->mepc += 4;
      break;
    }
    case EVENT_IRQ_TIMER:{
      rt_tick_increase();
      break;
    }
    case EVENT_IRQ_IODEV:{
      AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
      if (ev.keycode == AM_KEY_DOWN) break;
      if (ev.keydown) printf("%s", names[ev.keycode]);
      break;
    }
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  rt_thread_t current = rt_thread_self();
  rt_uint32_t user_data_temp = current->user_data;
  current->user_data = to;
  yield();
  current->user_data = user_data_temp;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  rt_thread_t current = rt_thread_self();
  *((Context**) from) = current->sp;
  rt_uint32_t user_data_temp = current->user_data;
  current->user_data = to;
  yield();
  current->user_data = user_data_temp;
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  //align
  stack_addr = (rt_uint8_t*)((uintptr_t)stack_addr & (~(sizeof(uintptr_t)-1)));


  // init stack,
  // stack sequence: [stack_bottom] context tentry parameter texit [stack_top]
  Area stack;
  stack.start = stack_addr - STACK_SIZE;
  stack.end = stack_addr;
  rt_ubase_t *stack_top = (rt_ubase_t *)(stack.end - sizeof(Context) - 1);
  Context* cp = kcontext(stack, wrapper, (void *)stack_top);
  *(stack_top) = (rt_ubase_t)tentry;
  stack_top--;
  *(stack_top) = (rt_ubase_t)parameter;
  stack_top--;
  *(stack_top) = (rt_ubase_t)texit;
  return (rt_uint8_t *)cp;
}
