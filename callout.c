#include "callout.h"
#include "libkern.h"

// The stuff in callout.h is just external interface. Here, we will do some magic.

#define NUMBER_OF_CALLOUT_BUCKETS 5

/*
  Every event is inside one of NUMBER_OF_CALLOUT_BUCKETS buckets.
  The buckets is a cyclic list, but we implement it as an array,
  allowing us to access random elements in constant time.

  TODO:
  - Change from unsigned int to some proper type. But if it's unsigned,
    make sure you don't decrease a zero.

  - long longs (nor int64_t) work: "Undefined reference to '__divdi3'"".
    https://gcc.gnu.org/onlinedocs/gcc/Link-Options.html, so I had
    to change sbintime_t from int64_t to int.
    Do we have <stdint.h>? smallclib doesn't have it.
*/

TAILQ_HEAD(callout_head, callout);


typedef struct callout_internal {
  struct callout_head heads[NUMBER_OF_CALLOUT_BUCKETS];
  unsigned int current_position; /* Which of these heads is at current time. */
  unsigned int uptime; /* Number of ticks since the system started */
} callout_internal_t;

static callout_internal_t ci;


void callout_init() {
  memset(&ci, 0, sizeof ci);

  for (int i = 0; i < NUMBER_OF_CALLOUT_BUCKETS; i++)
    TAILQ_INIT(&ci.heads[i]);
}

void callout_setup(struct callout *handle, sbintime_t time, timeout_t fn, void *arg) {
  memset(handle, 0, sizeof(struct callout));

  int index = (time + ci.uptime) % NUMBER_OF_CALLOUT_BUCKETS;

  handle->c_time = time + ci.uptime;
  handle->c_func = fn;
  handle->c_arg = arg;
  handle->index = index;
  callout_pending(handle);

  log("Inserting into index: %d, because current_position = %d, time = %d, uptime = %d", index, ci.current_position, time, ci.uptime);
  TAILQ_INSERT_TAIL(&ci.heads[index], handle, c_link);
}

void callout_stop(callout_t *handle) {
  TAILQ_REMOVE(&ci.heads[handle->index], handle, c_link);
}

/* 
  If the time of an event comes, execute the callout function and delete it from the list. 
  Returns true if an element was deleted, false otherwise.
*/
bool process_element(struct callout_head* head, struct callout* element) {
  if (element->c_time == ci.uptime) {
    callout_active(element);
    callout_not_pending(element);

    element->c_func(element->c_arg);

    TAILQ_REMOVE(head, element, c_link);

    callout_not_active(element);

    return true;
  }

  if (element->c_time < ci.uptime) {
    panic("%s", "The time of a callout is smaller than uptime.");
  }

  return false;
}

/*
  This function makes a tick takes the next bucket and deals with its contents.
  If we want to run through several buckets at once, just run
  this function many times.
*/
void callout_process(sbintime_t now) {
  ci.current_position = (ci.current_position + 1) % NUMBER_OF_CALLOUT_BUCKETS;
  ci.uptime++;

  struct callout_head* head = &ci.heads[ci.current_position];
  struct callout* current;

  TAILQ_FOREACH(current, head, c_link) {
    // Deal with the next element if the currrent one is not the tail.
    bool element_deleted;
    do {
      element_deleted = false;

      if (current != TAILQ_LAST(head, callout_head)) {
        struct callout* next = TAILQ_NEXT(current, c_link);
        element_deleted = process_element(head, next);
      }
    } while (element_deleted);
  }

  // Deal with the first element
  if (!TAILQ_EMPTY(head)) {
    struct callout* first = TAILQ_FIRST(head);
    //log("Trying to process the head");
    process_element(head, first);
  }
}
