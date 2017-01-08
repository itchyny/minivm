#include <stdio.h>
#include <stdlib.h>
#include "state.h"

state* new_state() {
  state* s = (state*)malloc(sizeof(state));
  if (s == NULL)
    return NULL;
  s->node = NULL;
  s->scanner = NULL;
  return s;
}

void free_state(state* s) {
  free(s);
}
