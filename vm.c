#include <stdio.h>
#include <stdbool.h>
#include "node.h"
#include "opcode.h"
#include "vm.h"

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
  int i, j, offset = e->variableslen - 1; value v;
  e->stack = calloc(1024, sizeof(value));
  for (i = 0; i < e->codesidx; ++i) {
    /* printf("\n"); */
    /* printf("%d %d %d\n", i, e->stackidx, GET_OPCODE(e->codes[i])); */
    /* for (j = 0; j < 10; j++) { */
    /*   printf("%ld ", e->stack[j].lval); */
    /* } */
    /* printf("\n"); */
    /* for (j = 0; j < 10; j++) { */
    /*   printf("%ld ", e->variables[j].value.lval); */
    /* } */
    /* printf("\n"); */
    switch (GET_OPCODE(e->codes[i])) {
      case OP_POP:
        --e->stackidx;
        break;
      case OP_DUP:
        e->stack[e->stackidx] = e->stack[e->stackidx - 1];
        ++e->stackidx;
        break;
      case OP_LET:
        e->variables[GET_ARG_A(e->codes[i])].value = e->stack[--e->stackidx];
        break;
      case OP_LET_LOCAL:
        e->variables[offset - GET_ARG_A(e->codes[i])].value = e->stack[--e->stackidx];
        break;
      case OP_JMP:
        i += GET_ARG_A(e->codes[i]);
        break;
      case OP_JMP_IF:
        if (evaluate_bool(e))
          i += GET_ARG_A(e->codes[i]);
        break;
      case OP_JMP_IFNOT:
        if (!evaluate_bool(e))
          i += GET_ARG_A(e->codes[i]);
        break;
      case OP_UFCALL:
        e->stack[e->stackidx++].lval = i;
        i = e->variables[GET_ARG_A(e->codes[i])].value.lval;
        break;
      case OP_ALLOC:
        offset += GET_ARG_A(e->codes[i]);
        break;
      case OP_RET:
        i = e->variables[(offset -= GET_ARG_A(e->codes[i])) + 1].value.lval;
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
      case OP_UNOT: {
        bool b = !TO_BOOL(e->stack[e->stackidx - 1]);
        e->stack[e->stackidx - 1].type = VT_BOOL;
        e->stack[e->stackidx - 1].bval = b;
        break;
      }
      case OP_UADD: UNARY_OP(+); break;
      case OP_UMINUS: UNARY_OP(-); break;
      case OP_ADD: BINARY_OP(+); break;
      case OP_MINUS: BINARY_OP(-); break;
      case OP_TIMES: BINARY_OP(*); break;
      case OP_DIVIDE: BINARY_OP(/); break;
      case OP_IADD: IBINARY_OP(+); break;
      case OP_IMINUS: IBINARY_OP(-); break;
      case OP_GT: LOGICAL_BINARY_OP(>); break;
      case OP_GE: LOGICAL_BINARY_OP(>=); break;
      case OP_EQEQ: LOGICAL_BINARY_OP(==); break;
      case OP_NEQ: LOGICAL_BINARY_OP(!=); break;
      case OP_LT: LOGICAL_BINARY_OP(<); break;
      case OP_LE: LOGICAL_BINARY_OP(<=); break;
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
      case OP_LOAD_LOCAL_IDENT:
        e->stack[e->stackidx++] = e->variables[offset - GET_ARG_A(e->codes[i])].value;
        break;
      default: printf("Unknown opcode %d\n", GET_OPCODE(e->codes[i])); exit(1);
    }
  }
  if (e->stackidx != 0) {
    printf("stack not consumed\n");
    exit(1);
  }
}
