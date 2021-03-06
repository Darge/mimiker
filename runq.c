#include <libkern.h>
#include <common.h>
#include <thread.h>
#include <runq.h>

void runq_init(runq_t *rq) {
  memset(rq, 0, sizeof (*rq));

  for (int64_t i = 0; i < RQ_NQS; i++)
    TAILQ_INIT(&rq->rq_queues[i]);
}

void runq_add(runq_t *rq, thread_t *td) {
  uint64_t priority = td->td_priority / RQ_PPQ;
  TAILQ_INSERT_TAIL(&rq->rq_queues[priority], td, td_runq);
}

thread_t *runq_choose(runq_t *rq) {
  for (int64_t i = RQ_NQS - 1; i >= 0; i--) {
    struct rq_head *head = &rq->rq_queues[i];
    thread_t *td = TAILQ_FIRST(head);

    if (td)
      return td;
  }

  return NULL;
}

void runq_remove(runq_t *rq, thread_t *td) {
  uint64_t priority = td->td_priority / RQ_PPQ;
  TAILQ_REMOVE(&rq->rq_queues[priority], td, td_runq);
}

void runq_debug(runq_t *rq) {
    kprintf("enter runq_debug()\n");
    for (int i = 0; i < RQ_NQS; i++) {
        thread_t *td = NULL;
        TAILQ_FOREACH (td, &rq->rq_queues[i], td_runq) {
            kprintf("One thread in runq! %s, priority: %d \n", td->td_name, td->td_priority);
        }
    }
    kprintf("leave runq_debug()\n");
}

#ifdef _KERNELSPACE
int main() {
  thread_t t1;
  t1.td_priority = 3 * RQ_PPQ;
  thread_t t2;
  t2.td_priority = 4 * RQ_PPQ;
  thread_t t3;
  t3.td_priority = 1 * RQ_PPQ;
  thread_t t4;
  t4.td_priority = 5 * RQ_PPQ;

  runq_t runq;

  runq_init(&runq);

  runq_add(&runq, &t1);
  assert(runq_choose(&runq) == &t1);

  runq_add(&runq, &t2);
  assert(runq_choose(&runq) == &t2);

  runq_add(&runq, &t3);
  assert(runq_choose(&runq) == &t2);

  runq_add(&runq, &t4);
  assert(runq_choose(&runq) == &t4);

  runq_remove(&runq, &t4);
  assert(runq_choose(&runq) == &t2);

  runq_remove(&runq, &t3);
  assert(runq_choose(&runq) == &t2);

  runq_remove(&runq, &t2);
  assert(runq_choose(&runq) == &t1);

  runq_remove(&runq, &t1);
  assert(runq_choose(&runq) == NULL);

  return 0;
}
#endif // _KERNELSPACE
