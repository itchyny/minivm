#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "node.h"
#include "opcode.h"

enum value_type {
  VT_BOOL,
  VT_LONG,
  VT_DOUBLE,
};

typedef struct value {
  int type;
  union {
    bool bval;
    long lval;
    double dval;
  };
} value;

typedef struct constant_value {
  union {
    bool bval;
    long lval;
    double dval;
  };
} constant_value;

typedef struct variable {
  char* name;
  value value;
} variable;

typedef struct env {
  uint32_t codesidx;
  uint32_t codeslen;
  uint32_t* codes;
  uint32_t constantsidx;
  uint32_t constantslen;
  constant_value* constants;
  uint32_t stackidx;
  value* stack;
  variable* variables;
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
  e->constants = calloc(e->constantslen, sizeof(constant_value));
  e->stackidx = 0;
  e->stack = NULL;
  e->variables = calloc(128, sizeof(variable));
  return e;
}

static void free_env(env* e) {
  free(e->codes);
  free(e->constants);
  free(e->variables);
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

static uint16_t addconstant(env* e, constant_value v) {
  if (e->constantsidx == e->constantslen) {
    constant_value* old_constants = e->constants;
    e->constants = calloc(e->constantslen * 2, sizeof(constant_value));
    memcpy(e->constants, old_constants, e->constantslen * sizeof(constant_value));
    e->constantslen *= 2;
    free(old_constants);
  }
  e->constants[e->constantsidx] = v;
  return e->constantsidx++;
}

static int lookup(env* e, char* name, char set) {
  int i = 0;
  while (e->variables[i].name) {
    if (strcmp(e->variables[i].name, name) == 0)
      return i;
    ++i;
  }
  if (set) {
    e->variables[i].name = name;
    return i;
  }
  return -1;
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
        n = n->cdr;
      }
      break;
    case NODE_ASSIGN:
      codegen(e, n->cdr->cdr);
      addcode(e, MK_OP_A(OP_ASSIGN, lookup(e, (char*)n->cdr->car, 1)));
      break;
    case NODE_PRINT:
      codegen(e, n->cdr);
      addcode(e, OP_PRINT);
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
    case NODE_BOOL: {
      constant_value v;
      v.bval = (bool)((intptr_t)n->cdr == 1);
      addcode(e, MK_OP_A(OP_LOAD_BOOL, addconstant(e, v)));
      break;
    }
    case NODE_LONG: {
      constant_value v;
      v.lval = atol((char*)n->cdr);
      addcode(e, MK_OP_A(OP_LOAD_LONG, addconstant(e, v)));
      break;
    }
    case NODE_DOUBLE: {
      constant_value v;
      v.dval = strtod((char*)n->cdr, NULL);
      addcode(e, MK_OP_A(OP_LOAD_DOUBLE, addconstant(e, v)));
      break;
    }
    case NODE_IDENTIFIER: {
      int index = lookup(e, (char*)n->cdr, 0);
      if (index < 0) {
        printf("Unknown variable: %s\n", (char*)n->cdr);
        exit(1);
      }
      addcode(e, MK_OP_A(OP_LOAD_IDENT, index));
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
      case OP_ASSIGN:
        e->variables[GET_ARG_A(e->codes[i])].value = e->stack[--e->stackidx];
        break;
      case OP_ADD: BINARY_OP(+); break;
      case OP_MINUS: BINARY_OP(-); break;
      case OP_TIMES: BINARY_OP(*); break;
      case OP_DIVIDE: BINARY_OP(/); break;
      case OP_PRINT: {
          value val = e->stack[--e->stackidx];
          switch (val.type) {
            case VT_BOOL:
              if (val.bval)
                printf("true\n");
              else
                printf("false\n");
              break;
            case VT_LONG: printf("%ld\n", val.lval); break;
            case VT_DOUBLE: printf("%lf\n", val.dval); break;
          }
        }
        break;
      case OP_LOAD_BOOL:
        e->stack[e->stackidx].type = VT_BOOL;
        e->stack[e->stackidx++].bval = e->constants[GET_ARG_A(e->codes[i])].bval;
        break;
      case OP_LOAD_LONG:
        e->stack[e->stackidx].type = VT_LONG;
        e->stack[e->stackidx++].lval = e->constants[GET_ARG_A(e->codes[i])].lval;
        break;
      case OP_LOAD_DOUBLE:
        e->stack[e->stackidx].type = VT_DOUBLE;
        e->stack[e->stackidx++].dval = e->constants[GET_ARG_A(e->codes[i])].dval;
        break;
      case OP_LOAD_IDENT:
        e->stack[e->stackidx++] = e->variables[GET_ARG_A(e->codes[i])].value;
        break;
    }
  }
}

static void print_codes(env* e) {
  int i;
  for (i = 0; i < e->codesidx; i++) {
    switch (GET_OPCODE(e->codes[i])) {
      case OP_ASSIGN: printf("let %s\n", e->variables[GET_ARG_A(e->codes[i])].name); break;
      case OP_ADD: printf("+\n"); break;
      case OP_MINUS: printf("-\n"); break;
      case OP_TIMES: printf("*\n"); break;
      case OP_DIVIDE: printf("/\n"); break;
      case OP_PRINT: printf("print\n"); break;
      case OP_LOAD_BOOL:
        if (e->constants[GET_ARG_A(e->codes[i])].bval)
          printf("bool true\n");
        else
          printf("bool false\n");
        break;
      case OP_LOAD_LONG: printf("long %ld\n", e->constants[GET_ARG_A(e->codes[i])].lval); break;
      case OP_LOAD_DOUBLE: printf("double %lf\n", e->constants[GET_ARG_A(e->codes[i])].dval); break;
    }
  }
}
