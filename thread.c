#include "thread.h"

int thread_create(thread_t* td_ptr, prio_t priority, void (*function_ptr)(void * ) , void *args) {
  // assume args is NULL for now

  return SUCCESS;
}

void thread_exit() {
  while(1); /* Should we sleep? Should we run thread_destroy? */
}


void thread_destroy(thread_t* td_ptr) {
	/* Remove it from the queues */
}

int thread_join(thread_t* td_ptr) {
	/* TODO: Dunno what */
  return 0;
}
