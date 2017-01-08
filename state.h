#ifndef STATE_H
#define STATE_H

typedef struct state {
  struct node* node;
  void* scanner;
} state;

state* new_state();
void free_state(state*);

#endif
