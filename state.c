#include <stdio.h>
#include <stdlib.h>
#include "state.h"
#include "node.h"

node_pool* new_node_pool(node_pool* prev) {
  node_pool* np = (node_pool*)malloc(sizeof(node_pool));
  np->index = 0;
  np->length = 1024;
  np->next_pool = NULL;
  np->nodes = (node*)calloc(np->length, sizeof(node));
  if (prev != NULL) {
    prev->next_pool = np;
  }
  return np;
}

state* new_state() {
  state* s = (state*)malloc(sizeof(state));
  if (s == NULL)
    return NULL;
  s->node = NULL;
  s->scanner = NULL;
  s->current_pool = s->top_pool = new_node_pool(NULL);
  return s;
}

node* new_node(state* s) {
  if (s->current_pool->index == s->current_pool->length) {
    s->current_pool = new_node_pool(s->current_pool);
  }
  return &s->current_pool->nodes[s->current_pool->index++];
}

void free_node_pools(node_pool* np) {
  node_pool* p;
  while (np != NULL) {
    p = np;
    free(p->nodes);
    np = p->next_pool;
    free(p);
  }
}

void free_state(state* s) {
  free_node_pools(s->top_pool);
  free(s);
}
