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
  int i, offset = e->variableslen - 1; value v;
  e->stack = calloc(1024, sizeof(value));
  for (i = 0; i < e->codesidx; ++i) {
    /* printf("%d %d %d\n", i, e->stackidx, GET_OPCODE(e->codes[i])); */
    switch (GET_OPCODE(e->codes[i])) {
      case OP_POP:
        --e->stackidx;
        break;
      case OP_LET:
        e->variables[offset - GET_ARG_A(e->codes[i])].value = e->stack[--e->stackidx];
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
      case OP_SAVEPC:
        e->stack[e->stackidx++].lval = i;
        break;
      case OP_UNSAVEPC:
        i = e->variables[offset + 1].value.lval + 1; // todo: index...
        break;
      case OP_ALLOC:
        offset += GET_ARG_A(e->codes[i]);
        break;
      case OP_UNALLOC:
        offset -= GET_ARG_A(e->codes[i]);
        break;
      case OP_FCALL: {
        int len = GET_ARG_B(e->codes[i]);
        gfuncs[GET_ARG_A(e->codes[i])].func(e, &e->stack[e->stackidx -= len], len);
        break;
      }
      case OP_UFCALL:
        i = e->variables[offset - GET_ARG_A(e->codes[i])].value.fval;
        break;
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
      case OP_UADD: UNARY_OP(+); break;
      case OP_UMINUS: UNARY_OP(-); break;
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
