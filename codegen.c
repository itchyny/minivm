#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "opcode.h"

typedef struct scope {
  uint32_t pc;
  uint32_t codesidx;
  uint32_t codeslen;
  uint32_t* codes;
  uint32_t constantsidx;
  uint32_t constantslen;
  intptr_t* constants;
} scope;

static scope* new_scope() {
  scope* s = (scope*)malloc(sizeof(scope));
  if (!s)
    return NULL;
  s->pc = 0;
  s->codesidx = 0;
  s->codeslen = 1024;
  s->codes = calloc(s->codeslen, sizeof(uint32_t));
  s->constantsidx = 0;
  s->constantslen = 128;
  s->constants = calloc(s->constantslen, sizeof(intptr_t));
  return s;
}

static void free_scope(scope* s) {
  free(s->codes);
  free(s);
}

static void addcode(scope* s, uint32_t c) {
  if (s->codesidx == s->codeslen) {
    uint32_t* old_codes = s->codes;
    s->codes = calloc(s->codeslen * 2, sizeof(uint32_t));
    memcpy(s->codes, old_codes, s->codeslen * sizeof(uint32_t));
    s->codeslen *= 2;
    free(old_codes);
  }
  s->codes[s->codesidx++] = c;
}

static uint16_t addconstant(scope* s, intptr_t c) {
  if (s->constantsidx == s->constantslen) {
    intptr_t* old_constants = s->constants;
    s->constants = calloc(s->constantslen * 2, sizeof(intptr_t));
    memcpy(s->constants, old_constants, s->constantslen * sizeof(intptr_t));
    s->constantslen *= 2;
    free(old_constants);
  }
  s->constants[s->constantsidx] = c;
  return s->constantsidx++;
}

#define GET_OPCODE(i)       ((int)(((uint32_t)(i)) & 0xff))
#define GET_ARG_A(i)        ((int)((((uint32_t)(i)) >> 8) & 0xffff))

#define MK_OPCODE(op)       ((op) & 0xff)
#define MK_ARG_A(a)         ((intptr_t)((a) & 0xffff) << 8)
#define MK_OP_A(op,a)       (MK_OPCODE(op)|MK_ARG_A(a))

static void codegen(scope* s, node* n) {
  switch (intn(n->car)) {
    case NODE_STMTS:
      codegen(s, n->cdr->car);
      addcode(s, MK_OPCODE(OP_PRINT_POP));
      codegen(s, n->cdr->cdr);
      addcode(s, MK_OPCODE(OP_PRINT_POP));
      break;
    case NODE_BINOP:
      codegen(s, n->cdr->cdr->car);
      codegen(s, n->cdr->cdr->cdr);
      switch (intn(n->cdr->car)) {
        case PLUS: addcode(s, MK_OPCODE(OP_ADD)); break;
        case MINUS: addcode(s, MK_OPCODE(OP_MINUS)); break;
        case TIMES: addcode(s, MK_OPCODE(OP_TIMES)); break;
        case DIVIDE: addcode(s, MK_OPCODE(OP_DIVIDE)); break;
      }
      break;
    case NODE_DOUBLE:
      addcode(s, MK_OP_A(OP_LOAD_DOUBLE, addconstant(s, (intptr_t)(n->cdr))));
      break;
    case NODE_LONG:
      addcode(s, MK_OP_A(OP_LOAD_LONG, addconstant(s, (intptr_t)(n->cdr))));
      break;
  }
}

static void print_codes(scope* s) {
  int i;
  for (i = 0; i < s->codesidx; i++) {
    switch (GET_OPCODE(s->codes[i])) {
      case OP_ADD: printf("+\n"); break;
      case OP_MINUS: printf("-\n"); break;
      case OP_TIMES: printf("*\n"); break;
      case OP_DIVIDE: printf("/\n"); break;
      case OP_PRINT_POP: printf("print\n"); break;
      case OP_LOAD_LONG: printf("long %ld\n", longn(s->constants[GET_ARG_A(s->codes[i])])); break;
      case OP_LOAD_DOUBLE: printf("double %lf\n", doublen(s->constants[GET_ARG_A(s->codes[i])])); break;
    }
  }
}
