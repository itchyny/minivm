#ifndef STATE_H
#define STATE_H

#include "node.h"

typedef struct node_pool {
  uint16_t idx;
  uint16_t len;
  struct node* nodes;
  struct node_pool* next;
} node_pool;

typedef struct state {
  struct node* node;
  void* scanner;
  node_pool* top_nodes;
  node_pool* current_nodes;
} state;

state* new_state();
node* new_node(state*);
void free_state(state*);

#endif
