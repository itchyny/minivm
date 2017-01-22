#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "node.h"
#include "opcode.h"
#include "vm.h"
#include "func.c"

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
  e->variableslen = 0;
  e->local_variables = NULL;
  e->local_variables_len = 0;
  e->func_pc = 0;
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
#define MK_OP_A(op,a)       ((op)|MK_ARG_A(a))
#define MK_OP_AB(op,a,b)    ((op)|MK_ARG_A(a)|MK_ARG_B(b))

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

typedef struct variable_index {
  bool global;
  int index;
} variable_index;

static variable_index lookup(env* e, char* name, bool set) {
  variable_index vi;
  if (e->local_variables != NULL) {
    vi.global = false;
    vi.index = 0;
    while (e->local_variables[vi.index].name) {
      if (strcmp(e->local_variables[vi.index].name, name) == 0)
        return vi;
      vi.index++;
    }
    if (set) {
      e->local_variables[vi.index].name = name;
      e->local_variables_len = vi.index + 1;
      return vi;
    }
  }
  vi.global = true;
  vi.index = 0;
  while (e->variables[vi.index].name) {
    if (strcmp(e->variables[vi.index].name, name) == 0)
      return vi;
    vi.index++;
  }
  if (set) {
    e->variables[vi.index].name = name;
    e->variableslen = vi.index + 1;
    return vi;
  }
  vi.index = -1;
  return vi;
}

static uint16_t let_args(env* e, node* fargs) {
  uint16_t count = 0;
  variable_index vi;
  if (fargs != NULL) {
    count += let_args(e, fargs->cdr);
    vi = lookup(e, (char*)fargs->car, true);
    addcode(e, MK_OP_A(OP_LET_LOCAL, vi.index)); ++count;
  }
  return count;
}

static uint16_t codegen(env* e, node* n) {
  uint16_t count = 0;
  switch (intn(n->car)) {
    case NODE_FUNCTION: {
      constant_value v; v.lval = e->codesidx + 3;
      variable_index vi = lookup(e, (char*)n->cdr->car, true);
      addcode(e, MK_OP_A(OP_LOAD_LONG, addconstant(e, v))); ++count;
      addcode(e, MK_OP_A(OP_LET, vi.index)); ++count;
      e->local_variables = calloc(128, sizeof(variable));
      e->local_variables_len = 0;
      uint16_t save_func_pc = e->func_pc; e->func_pc = e->codesidx;
      uint16_t index1, index2, index3, index4;
      index1 = addcode(e, OP_JMP); ++count;
      index2 = addcode(e, OP_RET); ++count;
      index3 = addcode(e, OP_ALLOC); ++count;
      index4 = addcode(e, OP_LET_LOCAL); ++count;
      count += let_args(e, n->cdr->cdr->car);
      count += codegen(e, n->cdr->cdr->cdr);
      v.lval = 0;
      addcode(e, MK_OP_A(OP_LOAD_LONG, addconstant(e, v))); ++count;
      addcode(e, MK_OP_A(OP_JMP, - (long)(e->codesidx - e->func_pc))); ++count;
      operand(e, index1, e->codesidx - e->func_pc - 1);
      operand(e, index2, e->local_variables_len + 1);
      operand(e, index3, e->local_variables_len + 1);
      operand(e, index4, e->local_variables_len);
      free(e->local_variables);
      e->local_variables = NULL;
      e->local_variables_len = 0;
      e->func_pc = save_func_pc;
      break;
    }
    case NODE_RETURN:
      count += codegen(e, n->cdr);
      addcode(e, MK_OP_A(OP_JMP, - (long)(e->codesidx - e->func_pc))); ++count;
      break;
    case NODE_STMTS:
      n = n->cdr;
      while (n != NULL) {
        count += codegen(e, n->car);
        n = n->cdr;
      }
      break;
    case NODE_ASSIGN: {
      variable_index vi = lookup(e, (char*)n->cdr->car, true);
      count += codegen(e, n->cdr->cdr);
      addcode(e, MK_OP_A(vi.global ? OP_LET : OP_LET_LOCAL, vi.index)); ++count;
      break;
    }
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
      count += (diff0 = codegen(e, n->cdr->car));
      index = addcode(e, OP_JMP_NOT); ++count;
      count += (diff1 = codegen(e, n->cdr->cdr));
      addcode(e, MK_OP_A(OP_JMP, -(diff0 + diff1 + 2))); ++count;
      operand(e, index, diff1 + 1);
      break;
    }
    case NODE_PRINT:
      count += codegen(e, n->cdr);
      addcode(e, OP_PRINT); ++count;
      break;
    case NODE_FCALL: {
      uint16_t num = 0, op, i;
      variable_index vi;
      vi = lookup(e, (char*)n->cdr->car, false);
      if (vi.index >= 0) {
        op = OP_UFCALL;
        i = vi.index;
      } else {
        op = OP_FCALL;
        for (i = 0; i < sizeof(gfuncs) / sizeof(func); ++i) {
          if (!strcmp(gfuncs[i].name, (char*)n->cdr->car))
            break;
        }
        if (i == sizeof(gfuncs) / sizeof(func)) {
          printf("Unknown function: %s\n", (char*)n->cdr->car);
          exit(1);
        }
      }
      node* m = n->cdr->cdr;
      while (m != NULL) {
        ++num;
        count += codegen(e, m->car);
        m = m->cdr;
      }
      addcode(e, MK_OP_AB(op, i, num)); ++count;
      break;
    }
    case NODE_UNARYOP:
      count += codegen(e, n->cdr->cdr);
      switch (intn(n->cdr->car)) {
        case NOT: addcode(e, OP_UNOT); break;
        case PLUS: addcode(e, OP_UADD); break;
        case MINUS: addcode(e, OP_UMINUS); break;
        default: printf("Unknown unary operator\n"); exit(1);
      };
      ++count;
      break;
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
          default: printf("Unknown binary operator\n"); exit(1);
        }
        ++count;
      }
      break;
    case NODE_BOOL: {
      constant_value v; v.bval = (bool)((intptr_t)n->cdr == 1);
      addcode(e, MK_OP_A(OP_LOAD_BOOL, addconstant(e, v))); ++count;
      break;
    }
    case NODE_LONG: {
      constant_value v; v.lval = atol((char*)n->cdr);
      addcode(e, MK_OP_A(OP_LOAD_LONG, addconstant(e, v))); ++count;
      break;
    }
    case NODE_DOUBLE: {
      constant_value v; v.dval = strtod((char*)n->cdr, NULL);
      addcode(e, MK_OP_A(OP_LOAD_DOUBLE, addconstant(e, v))); ++count;
      break;
    }
    case NODE_IDENTIFIER: {
      variable_index vi;
      vi = lookup(e, (char*)n->cdr, false);
      if (vi.index < 0) {
        printf("Unknown variable: %s\n", (char*)n->cdr);
        exit(1);
      }
      addcode(e, MK_OP_A(vi.global ? OP_LOAD_IDENT : OP_LOAD_LOCAL_IDENT, vi.index)); ++count;
      break;
    }
    default:
      printf("Unknown node %d\n", intn(n->car));
      exit(1);
  }
  return count;
}

#define UNARY_OP(op) \
  do { \
    value val = e->stack[--e->stackidx]; \
    if (val.type == VT_DOUBLE) { \
      e->stack[e->stackidx++].dval = op(val.dval); \
      \
    } else { \
      e->stack[e->stackidx].type = VT_LONG; \
      e->stack[e->stackidx++].lval = op(TO_LONG(val)); \
    } \
  } while(0);

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

static void print_codes(env* e) {
  int i;
  for (i = 0; i < e->codesidx; i++) {
    switch (GET_OPCODE(e->codes[i])) {
      case OP_POP: printf("pop\n"); break;
      case OP_LET: printf("let %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_LET_LOCAL: printf("let_local %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP: printf("jmp %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP_IF_KEEP: printf("jmp_if_keep %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP_NOT: printf("jmp_not %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_JMP_NOT_KEEP: printf("jmp_not_keep %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_UFCALL: printf("ufcall %d %d\n", GET_ARG_A(e->codes[i]), GET_ARG_B(e->codes[i])); break;
      case OP_ALLOC: printf("alloc %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_RET: printf("ret %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_PRINT: printf("print\n"); break;
      case OP_FCALL: printf("fcall %d %d\n", GET_ARG_A(e->codes[i]), GET_ARG_B(e->codes[i])); break;
      case OP_UNOT: printf("u!\n"); break;
      case OP_UADD: printf("u+\n"); break;
      case OP_UMINUS: printf("u-\n"); break;
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
      case OP_LOAD_IDENT: printf("load %d\n", GET_ARG_A(e->codes[i])); break;
      case OP_LOAD_LOCAL_IDENT: printf("load_local %d\n", GET_ARG_A(e->codes[i])); break;
      default: printf("Unknown opcode %d\n", GET_OPCODE(e->codes[i])); exit(1);
    }
  }
}
