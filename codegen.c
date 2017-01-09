#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "opcode.h"

enum value_type {
  VT_LONG,
  VT_DOUBLE,
};

typedef struct value {
  int type;
  union {
    long lval;
    double dval;
  };
} value;

typedef struct env {
  uint32_t codesidx;
  uint32_t codeslen;
  uint32_t* codes;
  uint32_t constantsidx;
  uint32_t constantslen;
  intptr_t* constants;
  uint32_t stackidx;
  value* stack;
} env;

static env* new_env() {
  env* e = (env*)malloc(sizeof(env));
  if (!e)
    return NULL;
  e->codesidx = 0;
  e->codeslen = 1024;
  e->codes = calloc(e->codeslen, sizeof(uint32_t));
  e->constantsidx = 0;
  e->constantslen = 128;
  e->constants = calloc(e->constantslen, sizeof(intptr_t));
  e->stackidx = 0;
  e->stack = NULL;
  return e;
}

static void free_env(env* e) {
  free(e->codes);
  free(e);
}

static void addcode(env* e, uint32_t c) {
  if (e->codesidx == e->codeslen) {
    uint32_t* old_codes = e->codes;
    e->codes = calloc(e->codeslen * 2, sizeof(uint32_t));
    memcpy(e->codes, old_codes, e->codeslen * sizeof(uint32_t));
    e->codeslen *= 2;
    free(old_codes);
  }
  e->codes[e->codesidx++] = c;
}

static uint16_t addconstant(env* e, intptr_t c) {
  if (e->constantsidx == e->constantslen) {
    intptr_t* old_constants = e->constants;
    e->constants = calloc(e->constantslen * 2, sizeof(intptr_t));
    memcpy(e->constants, old_constants, e->constantslen * sizeof(intptr_t));
    e->constantslen *= 2;
    free(old_constants);
  }
  e->constants[e->constantsidx] = c;
  return e->constantsidx++;
}

#define GET_OPCODE(i)       ((int)(((uint32_t)(i)) & 0xff))
#define GET_ARG_A(i)        ((int)((((uint32_t)(i)) >> 8) & 0xffffff))

#define MK_ARG_A(a)         ((intptr_t)((a) & 0xffffff) << 8)
#define MK_OP_A(op,a)       (op|MK_ARG_A(a))

static void codegen(env* e, node* n) {
  switch (intn(n->car)) {
    case NODE_STMTS:
      n = n->cdr;
      while (n != NULL) {
        codegen(e, n->car);
        addcode(e, OP_PRINT_POP);
        n = n->cdr;
      }
      break;
    case NODE_BINOP:
      codegen(e, n->cdr->cdr->car);
      codegen(e, n->cdr->cdr->cdr);
      switch (intn(n->cdr->car)) {
        case PLUS: addcode(e, OP_ADD); break;
        case MINUS: addcode(e, OP_MINUS); break;
        case TIMES: addcode(e, OP_TIMES); break;
        case DIVIDE: addcode(e, OP_DIVIDE); break;
      }
      break;
    case NODE_LONG: {
      long l = atol((char*)n->cdr);
      printf("NODE_LONG: %s %ld\n", (char*)n->cdr, l);
      addcode(e, MK_OP_A(OP_LOAD_LONG, addconstant(e, (intptr_t)(nptr(l)))));
      break;
    }
    case NODE_DOUBLE: {
      double d = strtod((char*)n->cdr, NULL);
      printf("NODE_DOUBLE: %s %lf\n", (char*)n->cdr, d);
      addcode(e, MK_OP_A(OP_LOAD_DOUBLE, addconstant(e, (intptr_t)(nptr(d)))));
      break;
    }
  }
}

#define BINARY_OP(op) \
  do { \
    value rhs = e->stack[--e->stackidx]; \
    value lhs = e->stack[--e->stackidx]; \
    switch (lhs.type) { \
      case VT_LONG: \
        switch (rhs.type) { \
          case VT_LONG: \
            e->stack[e->stackidx++].lval = lhs.lval op rhs.lval; \
            break; \
          case VT_DOUBLE: \
            e->stack[e->stackidx].type = VT_DOUBLE; \
            e->stack[e->stackidx++].dval = (double)lhs.lval op rhs.dval; \
            break; \
        } \
        break; \
      case VT_DOUBLE: \
        switch (rhs.type) { \
          case VT_LONG: \
            e->stack[e->stackidx++].dval = lhs.dval op (double)rhs.lval; \
            break; \
          case VT_DOUBLE: \
            e->stack[e->stackidx++].dval = lhs.dval op rhs.dval; \
            break; \
        } \
        break; \
    } \
  } while(0);

static void execute_codes(env* e) {
  int i;
  e->stack = calloc(1024, sizeof(value));
  for (i = 0; i < e->codesidx; i++) {
    switch (GET_OPCODE(e->codes[i])) {
      case OP_ADD: BINARY_OP(+); break;
      case OP_MINUS: BINARY_OP(-); break;
      case OP_TIMES: BINARY_OP(*); break;
      case OP_DIVIDE: BINARY_OP(/); break;
      case OP_PRINT_POP: {
          value val = e->stack[--e->stackidx];
          switch (val.type) {
            case VT_LONG: printf("%ld\n", val.lval); break;
            case VT_DOUBLE: printf("%lf\n", val.dval); break;
          }
        }
        break;
      case OP_LOAD_LONG:
        e->stack[e->stackidx].type = VT_LONG;
        e->stack[e->stackidx++].lval = longn(e->constants[GET_ARG_A(e->codes[i])]);
        break;
      case OP_LOAD_DOUBLE:
        e->stack[e->stackidx].type = VT_DOUBLE;
        e->stack[e->stackidx++].dval = doublen(e->constants[GET_ARG_A(e->codes[i])]);
        break;
    }
  }
}

static void print_codes(env* e) {
  int i;
  for (i = 0; i < e->codesidx; i++) {
    switch (GET_OPCODE(e->codes[i])) {
      case OP_ADD: printf("+\n"); break;
      case OP_MINUS: printf("-\n"); break;
      case OP_TIMES: printf("*\n"); break;
      case OP_DIVIDE: printf("/\n"); break;
      case OP_PRINT_POP: printf("print\n"); break;
      case OP_LOAD_LONG: printf("long %ld\n", longn(e->constants[GET_ARG_A(e->codes[i])])); break;
      case OP_LOAD_DOUBLE: printf("double %lf\n", doublen(e->constants[GET_ARG_A(e->codes[i])])); break;
    }
  }
}
