#include <libkern.h>
#include <sched.h>
#include <runq.h>
#include <context.h>
#include <clock.h>
#include <thread.h>
#include <callout.h>
#include <interrupts.h>
#include <mips.h>
#include <critical_section.h>

static runq_t runq;
static thread_t *td_sched;
static callout_t sched_callout;


//////////// RANDOM
static unsigned long int next = 1;
static int rand() {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % 32768;
}
static void srand(unsigned int seed) {
    next = seed;
}

static void do_random_stuff() {
    for (int i = 0; i < rand()%5; i++) {
        thread_t *td;
        td = runq_choose(&runq);
        if (td){
            runq_remove(&runq, td);
            runq_add(&runq, td);
        }
    }
}
//////////// RANDOM

void sched_init() {
  runq_init(&runq);
  srand(123);
}

void sched_add(thread_t *td) {
  cs_enter();

  log("Add '%s' {%p} thread to scheduler", td->td_name, td);
  runq_add(&runq, td);

  runq_debug(&runq);

  cs_leave();
}

static bool sched_activate = false;

void sched_yield(bool add_to_runq) {
  thread_t *td = thread_self();
  assert(td != td_sched);

  callout_stop(&sched_callout);
  if (add_to_runq)
    runq_add(&runq, td);
  sched_activate = false;
  cs_leave();
  /* Scheduler shouldn't trouble us between these two instructions
     because it's deactivated and not in the callout. */
  thread_switch_to(td_sched);
  cs_leave();
}

static void sched_wakeup() {
  assert(during_intr_handler());

  thread_t *td = thread_self();
  assert(td != td_sched);

  callout_stop(&sched_callout);
  runq_add(&runq, td);
  sched_activate = true;
}

void sched_resume() {
  assert(during_intr_handler());

  if (!sched_activate)
    return;

  sched_activate = false;
  mips32_bc_c0(C0_STATUS, SR_EXL);
  // intr_enable();
  cs_leave();
  /* Scheduler shouldn't trouble us between these two instructions
     because it's deactivated and not in the callout. */
  thread_switch_to(td_sched);
}

noreturn void sched_run(size_t quantum) {
  td_sched = thread_self();

  while (true) {
    thread_t *td;

    //while (!(td = runq_choose(&runq)));
    while (true) {
        cs_enter();
        runq_debug(&runq);
        do_random_stuff();
        runq_debug(&runq);


        td = runq_choose(&runq);
        if (!td) {
            cs_leave();
            continue;
        }

        break;
    }

    runq_remove(&runq, td);
    callout_setup(&sched_callout, clock_get_ms() + quantum, sched_wakeup, NULL);
    thread_switch_to(td);
  }
}

#ifdef _KERNELSPACE

static void demo_thread_1() {
  while (true) {
    uint32_t now = clock_get_ms();
    kprintf("[%8ld] Running '%s' thread.\n", now, thread_self()->td_name);
    while (clock_get_ms() < now + 20);
  }
}

static void demo_thread_2() {
  kprintf("Running '%s' thread. Let's yield!\n", thread_self()->td_name);
  sched_yield(true);
  demo_thread_1();
}

int main() {
  sched_init();

  thread_t *t1 = thread_create("t1", demo_thread_1);
  thread_t *t2 = thread_create("t2", demo_thread_1);
  thread_t *t3 = thread_create("t3", demo_thread_1);
  thread_t *t4 = thread_create("t4", demo_thread_2);
  thread_t *t5 = thread_create("t5", demo_thread_2);

  sched_add(t1);
  sched_add(t2);
  sched_add(t3);
  sched_add(t4);
  sched_add(t5);

  sched_run(1);
}

#endif // _KERNELSPACE
