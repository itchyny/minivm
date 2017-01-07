#ifndef AST_NODE_H
#define AST_NODE_H

#include <stdint.h>

#define nint(x) ((node*)(intptr_t)(x))
#define intn(x) ((int)(intptr_t)(x))
#define nptr(x) ((node*)*(intptr_t*)&(x))
#define doublen(x) (*(double*)&(x))
#define longn(x) (*(long*)&(x))

enum node_type {
  NODE_BINOP,
  NODE_LONG,
  NODE_DOUBLE
};

typedef struct node {
  struct node *car, *cdr;
} node;

#endif
