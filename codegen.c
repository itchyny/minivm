#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "node.h"
#include "opcode.h"

enum value_type {
  VT_BOOL,
  VT_LONG,
  VT_DOUBLE,
  VT_FUNC,
};

typedef struct value {
  int type;
  union {
    bool bval;
    long lval;
    double dval;
    struct func* fval;
  };
} value;

#define TO_LONG(val) (\
  val.type == VT_DOUBLE ? (long)val.dval : \
  val.type == VT_LONG ? val.lval : \
  val.type == VT_BOOL ? (val.bval ? 1 : 0) : 0)

#define TO_DOUBLE(val) (\
  val.type == VT_DOUBLE ? val.dval : \
  val.type == VT_LONG ? (double)val.lval : \
  val.type == VT_BOOL ? (val.bval ? 1.0 : 0.0) : 0.0)

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

typedef struct func {
  char* name;
  void (*func)(env*, value*, int);
} func;

static void f_min(env* e, value* values, int len) {
  int i; long l; double d, g;
  value v;
  v.type = VT_LONG;
  for (i = 0; i < len; ++i) {
    if (v.type == VT_LONG) {
      if (values[i].type == VT_LONG || values[i].type == VT_BOOL) {
        l = TO_LONG(values[i]);
        v.lval = i == 0 ? l : l > v.lval ? v.lval : l;
        continue;
      }
      v.type = VT_DOUBLE;
      d = (double)v.lval;
    } else {
      d = v.dval;
    }
    g = TO_DOUBLE(values[i]);
    v.dval = i == 0 ? g : d > g ? g : d;
  }
  e->stack[e->stackidx++] = v;
}

static void f_max(env* e, value* values, int len) {
  int i; long l; double d, g;
  value v;
  v.type = VT_LONG;
  for (i = 0; i < len; ++i) {
    if (v.type == VT_LONG) {
      if (values[i].type == VT_LONG || values[i].type == VT_BOOL) {
        l = TO_LONG(values[i]);
        v.lval = i == 0 ? l : l > v.lval ? l : v.lval;
        continue;
      }
      v.type = VT_DOUBLE;
      d = (double)v.lval;
    } else {
      d = v.dval;
    }
    g = TO_DOUBLE(values[i]);
    v.dval = i == 0 ? g : d > g ? d : g;
  }
  e->stack[e->stackidx++] = v;
}

func gfuncs[] = {
  { "min", f_min },
  { "max", f_max },
};

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

#define GET_OPCODE(i)       ((uint8_t)(i & 0xff))
#define GET_ARG_A(i)        ((int16_t)((i >> 8) & 0xffff))
#define GET_ARG_B(i)        ((int8_t)((i >> 24) & 0xff))

#define MK_ARG_A(a)         ((intptr_t)((a) & 0xffff) << 8)
#define MK_ARG_B(a)         ((intptr_t)((a) & 0xff) << 24)
#define MK_OP_A(op,a)       (op|MK_ARG_A(a))
#define MK_OP_AB(op,a,b)    (op|MK_ARG_A(a)|MK_ARG_B(b))

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

static void operand(env* e, uint16_t index, int32_t a) {
  e->codes[index] |= MK_ARG_A(a);
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
      addcode(e, MK_OP_A(OP_LET, lookup(e, (char*)n->cdr->car, 1)));
      break;
    case NODE_IF: {
      int16_t diff0, diff1; uint16_t index0, index1;
      count += codegen(e, n->cdr->car);
      index0 = addcode(e, OP_JMP_NOT); ++count;
      count += (diff0 = codegen(e, n->cdr->cdr->car));
      if (n->cdr->cdr->cdr != NULL) {
        index1 = addcode(e, OP_JMP); ++count;
        count += (diff1 = codegen(e, n->cdr->cdr->cdr));
        operand(e, index1, diff1);
        ++diff0;
      }
      operand(e, index0, diff0);
      break;
    }
    case NODE_WHILE: {
      int16_t diff0, diff1; uint16_t index;
      index = addcode(e, OP_JMP); ++count;
      count += (diff0 = codegen(e, n->cdr->cdr));
      operand(e, index, diff0);
      count += (diff1 = codegen(e, n->cdr->car));
      addcode(e, MK_OP_A(OP_JMP_IF, -diff0 - diff1 - 1)); ++count;
      break;
    }
    case NODE_PRINT:
      count += codegen(e, n->cdr);
      addcode(e, OP_PRINT); ++count;
      break;
    case NODE_FCALL: {
      uint16_t num = 0, i;
      for (i = 0; i < sizeof(gfuncs) / sizeof(func); ++i) {
        if (!strcmp(gfuncs[i].name, (char*)n->cdr->car))
          break;
      }
      if (i == sizeof(gfuncs) / sizeof(func)) {
        printf("Unknown function: %s\n", (char*)n->cdr->car);
        exit(1);
      }
      node* m = n->cdr->cdr;
      while (m != NULL) {
        ++num;
        count += codegen(e, m->car);
        m = m->cdr;
      }
      addcode(e, MK_OP_AB(OP_FCALL, i, num)); ++count;
      break;
    }
    case NODE_BINOP:
      count += codegen(e, n->cdr->cdr->car);
      if (intn(n->cdr->car) == AND) {
        int16_t diff; uint16_t index;
        index = addcode(e, OP_JMP_NOT_KEEP); ++count;
        addcode(e, OP_POP); ++count;
        count += (diff = codegen(e, n->cdr->cdr->cdr));
        operand(e, index, diff + 1);
      } else if (intn(n->cdr->car) == OR) {
        int16_t diff; uint16_t index;
        index = addcode(e, OP_JMP_IF_KEEP); ++count;
        addcode(e, OP_POP); ++count;
        count += (diff = codegen(e, n->cdr->cdr->cdr));
        operand(e, index, diff + 1);
      } else {
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
          default: printf("unknown binary operator\n"); exit(1);
        }
        ++count;
      }
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
  for (i = 0; i < e->codesidx; ++i) {
    /* printf("%d %d %d\n", i, e->stackidx, GET_OPCODE(e->codes[i])); */
    switch (GET_OPCODE(e->codes[i])) {
      case OP_POP:
        --e->stackidx;
        break;
      case OP_LET:
        e->variables[GET_ARG_A(e->codes[i])].value = e->stack[--e->stackidx];
        break;
      case OP_JMP:
        i += GET_ARG_A(e->codes[i]);
        break;
      case OP_JMP_IF:
        if (evaluate_bool(e))
          i += GET_ARG_A(e->codes[i]);
        break;
      case OP_JMP_IF_KEEP:
        if (evaluate_bool(e))
          i += GET_ARG_A(e->codes[i]);
        ++e->stackidx;
        break;
      case OP_JMP_NOT:
        if (!evaluate_bool(e))
          i += GET_ARG_A(e->codes[i]);
        break;
      case OP_JMP_NOT_KEEP:
        if (!evaluate_bool(e))
          i += GET_ARG_A(e->codes[i]);
        ++e->stackidx;
        break;
      case OP_FCALL: {
        int len = GET_ARG_B(e->codes[i]);
        gfuncs[GET_ARG_A(e->codes[i])].func(e, &e->stack[e->stackidx -= len], len);
        break;
      }
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
      case OP_ADD: BINARY_OP(+); break;
      case OP_MINUS: BINARY_OP(-); break;
      case OP_TIMES: BINARY_OP(*); break;
      case OP_DIVIDE: BINARY_OP(/); break;
      case OP_GT: BINARY_OP(>); break;
      case OP_GE: BINARY_OP(>=); break;
      case OP_EQEQ: BINARY_OP(==); break;
      case OP_LT: BINARY_OP(<); break;
      case OP_LE: BINARY_OP(<=); break;
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
      default: printf("unknown opcode %d\n", GET_OPCODE(e->codes[i])); exit(1);
    }
  }
  if (e->stackidx != 0) {
    printf("stack not consumed\n");
    exit(1);
  }
}

static void print_codes(env* e) {
  int i;
  for (i = 0; i < e->codesidx; i++) {
    switch (GET_OPCODE(e->codes[i])) {
      case OP_POP: printf("pop\n"); break;
      case OP_LET: printf("let %s\n", e->variables[GET_ARG_A(e->codes[i])].name); break;
      case OP_JMP: printf("jmp %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP_IF: printf("jmp_if %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP_IF_KEEP: printf("jmp_if_keep %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP_NOT: printf("jmp_not %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP_NOT_KEEP: printf("jmp_not_keep %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_PRINT: printf("print\n"); break;
      case OP_FCALL: printf("fcall %d %d\n", GET_ARG_A(e->codes[i]), GET_ARG_B(e->codes[i])); break;
      case OP_ADD: printf("+\n"); break;
      case OP_MINUS: printf("-\n"); break;
      case OP_TIMES: printf("*\n"); break;
      case OP_DIVIDE: printf("/\n"); break;
      case OP_GT: printf(">\n"); break;
      case OP_GE: printf(">=\n"); break;
      case OP_EQEQ: printf("==\n"); break;
      case OP_LT: printf("<\n"); break;
      case OP_LE: printf("<=\n"); break;
      case OP_LOAD_BOOL:
        if (e->constants[GET_ARG_A(e->codes[i])].bval)
          printf("bool true\n");
        else
          printf("bool false\n");
        break;
      case OP_LOAD_LONG: printf("long %ld\n", e->constants[GET_ARG_A(e->codes[i])].lval); break;
      case OP_LOAD_DOUBLE: printf("double %.9lf\n", e->constants[GET_ARG_A(e->codes[i])].dval); break;
      case OP_LOAD_IDENT: printf("load %s\n", e->variables[GET_ARG_A(e->codes[i])].name); break;
      default: printf("unknown opcode %d\n", GET_OPCODE(e->codes[i])); exit(1);
    }
  }
}
