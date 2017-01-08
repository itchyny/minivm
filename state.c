#include <stdio.h>
#include <stdlib.h>
#include "state.h"
#include "node.h"

node_pool* new_node_pool(node_pool* prev) {
  node_pool* np = (node_pool*)malloc(sizeof(node_pool));
  np->idx = 0;
  np->len = 1024;
  np->next = NULL;
  np->nodes = (node*)calloc(np->len, sizeof(node));
  if (prev != NULL) {
    prev->next = np;
  }
  return np;
}

state* new_state() {
  state* s = (state*)malloc(sizeof(state));
  if (s == NULL)
    return NULL;
  s->node = NULL;
  s->scanner = NULL;
  s->current_node_pool = s->top_node_pool = new_node_pool(NULL);
  return s;
}

node* new_node(state* s) {
  if (s->current_node_pool->idx == s->current_node_pool->len) {
    s->current_node_pool = new_node_pool(s->current_node_pool);
  }
  return &s->current_node_pool->nodes[s->current_node_pool->idx++];
}

void free_node_pools(node_pool* np) {
  node_pool* p;
  while (np != NULL) {
    p = np;
    free(p->nodes);
    np = p->next;
    free(p);
  }
}

void free_state(state* s) {
  free_node_pools(s->top_node_pool);
  free(s);
}
