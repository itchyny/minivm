#ifndef STATE_H
#define STATE_H

#include "node.h"

typedef struct node_pool {
  uint16_t index;
  uint16_t length;
  struct node* nodes;
  struct node_pool* next_pool;
} node_pool;

typedef struct state {
  struct node* node;
  void* scanner;
  node_pool* top_pool;
  node_pool* current_pool;
} state;

state* new_state();
node* new_node(state*);
void free_state(state*);

#endif
