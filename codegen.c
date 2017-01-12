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
  uint16_t codesidx;
  uint16_t codeslen;
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

static uint16_t addcode(env* e, uint32_t c) {
  if (e->codesidx == e->codeslen) {
    uint32_t* old_codes = e->codes;
    e->codes = calloc(e->codeslen * 2, sizeof(uint32_t));
    memcpy(e->codes, old_codes, e->codeslen * sizeof(uint32_t));
    e->codeslen *= 2;
    free(old_codes);
  }
  e->codes[e->codesidx] = c;
  return e->codesidx++;
}

static void setcode(env* e, uint16_t index, uint32_t c) {
  e->codes[index] = c;
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

static uint16_t codegen(env* e, node* n) {
  uint16_t count = 0;
  switch (intn(n->car)) {
    case NODE_STMTS:
      n = n->cdr;
      while (n != NULL) {
        count += codegen(e, n->car);
        n = n->cdr;
      }
      break;
    case NODE_ASSIGN:
      count += codegen(e, n->cdr->cdr) + 1;
      addcode(e, MK_OP_A(OP_ASSIGN, lookup(e, (char*)n->cdr->car, 1)));
      break;
    case NODE_IF: {
      uint16_t diff0, diff1, index0, index1;
      count += codegen(e, n->cdr->car) + 1;
      index0 = addcode(e, 0);
      count += (diff0 = codegen(e, n->cdr->cdr->car));
      if (n->cdr->cdr->cdr != NULL) {
        index1 = addcode(e, 0);
        count += (diff1 = codegen(e, n->cdr->cdr->cdr)) + 1;
        setcode(e, index1, MK_OP_A(OP_JMP, diff1));
        ++diff0;
      }
      setcode(e, index0, MK_OP_A(OP_JMP_NOT, diff0));
      break;
    }
    case NODE_WHILE: {
      uint16_t diff0, diff1, index;
      index = addcode(e, 0); ++count;
      count += (diff0 = codegen(e, n->cdr->cdr));
      setcode(e, index, MK_OP_A(OP_JMP, diff0));
      count += (diff1 = codegen(e, n->cdr->car));
      addcode(e, MK_OP_A(OP_JMP_IF_BACK, diff0 + diff1 + 1)); ++count;
      break;
    }
    case NODE_PRINT:
      count += codegen(e, n->cdr) + 1;
      addcode(e, OP_PRINT);
      break;
    case NODE_BINOP:
      count += codegen(e, n->cdr->cdr->car);
      count += codegen(e, n->cdr->cdr->cdr);
      switch (intn(n->cdr->car)) {
        case PLUS: addcode(e, OP_ADD); break;
        case MINUS: addcode(e, OP_MINUS); break;
        case TIMES: addcode(e, OP_TIMES); break;
        case DIVIDE: addcode(e, OP_DIVIDE); break;
        case GT: addcode(e, OP_GT); break;
        case GE: addcode(e, OP_GE); break;
        case EQEQ: addcode(e, OP_EQEQ); break;
        case LT: addcode(e, OP_LT); break;
        case LE: addcode(e, OP_LE); break;
      }
      ++count;
      break;
    case NODE_BOOL: {
      constant_value v;
      v.bval = (bool)((intptr_t)n->cdr == 1);
      addcode(e, MK_OP_A(OP_LOAD_BOOL, addconstant(e, v)));
      ++count;
      break;
    }
    case NODE_LONG: {
      constant_value v;
      v.lval = atol((char*)n->cdr);
      addcode(e, MK_OP_A(OP_LOAD_LONG, addconstant(e, v)));
      ++count;
      break;
    }
    case NODE_DOUBLE: {
      constant_value v;
      v.dval = strtod((char*)n->cdr, NULL);
      addcode(e, MK_OP_A(OP_LOAD_DOUBLE, addconstant(e, v)));
      ++count;
      break;
    }
    case NODE_IDENTIFIER: {
      int index = lookup(e, (char*)n->cdr, 0);
      if (index < 0) {
        printf("Unknown variable: %s\n", (char*)n->cdr);
        exit(1);
      }
      addcode(e, MK_OP_A(OP_LOAD_IDENT, index));
      ++count;
      break;
    }
  }
  return count;
}

#define TO_LONG(val) (\
  val.type == VT_DOUBLE ? (long)val.dval : \
  val.type == VT_LONG ? val.lval : \
  val.type == VT_BOOL ? (val.bval ? 1 : 0) : 0)

#define TO_DOUBLE(val) (\
  val.type == VT_DOUBLE ? val.dval : \
  val.type == VT_LONG ? (double)val.lval : \
  val.type == VT_BOOL ? (val.bval ? 1.0 : 0.0) : 0.0)

#define BINARY_OP(op) \
  do { \
    value rhs = e->stack[--e->stackidx]; \
    value lhs = e->stack[--e->stackidx]; \
    if (lhs.type == VT_DOUBLE || rhs.type == VT_DOUBLE) { \
      e->stack[e->stackidx].type = VT_DOUBLE; \
      e->stack[e->stackidx++].dval = TO_DOUBLE(lhs) op TO_DOUBLE(rhs); \
      \
    } else { \
      e->stack[e->stackidx].type = VT_LONG; \
      e->stack[e->stackidx++].lval = TO_LONG(lhs) op TO_LONG(rhs); \
    } \
  } while(0);

inline static bool evaluate_bool(env* e) {
  value v = e->stack[--e->stackidx];
  switch (v.type) {
    case VT_BOOL: return v.bval;
    case VT_LONG: return v.lval != 0.0;
    case VT_DOUBLE: return v.dval != 0;
  }
  return true;
}

static void execute_codes(env* e) {
  int i; value v;
  e->stack = calloc(1024, sizeof(value));
  for (i = 0; i < e->codesidx; i++) {
    switch (GET_OPCODE(e->codes[i])) {
      case OP_ASSIGN:
        e->variables[GET_ARG_A(e->codes[i])].value = e->stack[--e->stackidx];
        break;
      case OP_JMP:
        i += GET_ARG_A(e->codes[i]);
        break;
      case OP_JMP_IF_BACK:
        if (evaluate_bool(e))
          i -= GET_ARG_A(e->codes[i]);
        break;
      case OP_JMP_NOT:
        if (!evaluate_bool(e))
          i += GET_ARG_A(e->codes[i]);
        break;
      case OP_ADD: BINARY_OP(+); break;
      case OP_MINUS: BINARY_OP(-); break;
      case OP_TIMES: BINARY_OP(*); break;
      case OP_DIVIDE: BINARY_OP(/); break;
      case OP_GT: BINARY_OP(>); break;
      case OP_GE: BINARY_OP(>=); break;
      case OP_EQEQ: BINARY_OP(==); break;
      case OP_LT: BINARY_OP(<); break;
      case OP_LE: BINARY_OP(<=); break;
      case OP_PRINT:
        v = e->stack[--e->stackidx];
        switch (v.type) {
          case VT_BOOL:
            if (v.bval)
              printf("true\n");
            else
              printf("false\n");
            break;
          case VT_LONG: printf("%ld\n", v.lval); break;
          case VT_DOUBLE: printf("%.9lf\n", v.dval); break;
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
      case OP_JMP: printf("jmp %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP_IF_BACK: printf("jmp_if_back %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP_NOT: printf("jmp_not %d\n", GET_ARG_A(e->codes[i])); break;
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
      case OP_LOAD_DOUBLE: printf("double %.9lf\n", e->constants[GET_ARG_A(e->codes[i])].dval); break;
      case OP_LOAD_IDENT: printf("load %s\n", e->variables[GET_ARG_A(e->codes[i])].name); break;
    }
  }
}
