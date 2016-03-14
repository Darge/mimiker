#include <libkern.h>
#include <context.h>

void ctx_init(ctx_t *ctx, void (*target)(), void *stack, void *gp) {
  bzero(ctx, sizeof(ctx_t));
  ctx->ra = (word_t)target;
  ctx->sp = (word_t)stack;
  ctx->gp = (word_t)gp;
}
int foo() {return 1;}

void ctx_switch(ctx_t *from, ctx_t *to) {
  foo();
  if (!ctx_save(from))
    ctx_load(to);
}
