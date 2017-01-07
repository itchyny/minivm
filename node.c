#include "node.h"

static node* cons(node*, node*);
static node* new_binop(int, node*, node*);
static void print_binop(node*, int);
static void print_node(node*, int);

static node* cons(node* car, node* cdr) {
  node* n = (node*)malloc(sizeof(node));
  n->car = car;
  n->cdr = cdr;
  return n;
}

static node* new_binop(int op, node* lhs, node* rhs) {
  return cons(nint(NODE_BINOP), cons(nint(op), cons(lhs, rhs)));
}

static void print_binop(node* n, int indent) {
  switch (intn(n->car)) {
    case PLUS: printf("+,"); break;
    case MINUS: printf("-,"); break;
    case TIMES: printf("*,"); break;
    case DIVIDE: printf("/,"); break;
  }
  print_node(n->cdr->car, indent + 2);
  printf(",");
  print_node(n->cdr->cdr, indent + 2);
}

static void print_node(node* n, int indent) {
  if (indent != 0)
    printf("\n");
  for (int i = 0; i < indent; i++) {
    printf(" ");
  }
  printf("(%d, ", intn(n->car));
  switch (intn(n->car)) {
    case NODE_BINOP:
      print_binop(n->cdr, indent);
      break;
    case NODE_DOUBLE:
      printf("%lf", doublen(n->cdr));
      break;
    case NODE_LONG:
      printf("%ld", longn(n->cdr));
      break;
  }
  printf(")");
  if (indent == 0)
    printf("\n");
}
