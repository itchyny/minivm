#ifndef NODE_H
#define NODE_H

#include <stdint.h>

#define nint(x) ((node*)(intptr_t)(x))
#define intn(x) ((int)(intptr_t)(x))

enum node_type {
  NODE_FUNCTION,
  NODE_RETURN,
  NODE_STMTS,
  NODE_ASSIGN,
  NODE_IF,
  NODE_WHILE,
  NODE_BREAK,
  NODE_CONTINUE,
  NODE_PRINT,
  NODE_FCALL,
  NODE_UNARYOP,
  NODE_BINOP,
  NODE_BOOL,
  NODE_LONG,
  NODE_DOUBLE,
  NODE_IDENTIFIER,
};

typedef struct node {
  struct node *car, *cdr;
} node;

node* new_cons();
node* append(node*, node*);
node* new_uop();
node* new_binop();
void print_node(node*, int);

#endif
