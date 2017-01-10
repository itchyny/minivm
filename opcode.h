#ifndef OPCODE_H
#define OPCODE_H

enum OP_CODE {
  OP_ASSIGN,
  OP_JMP_NOT,
  OP_PRINT,
  OP_ADD,
  OP_MINUS,
  OP_TIMES,
  OP_DIVIDE,
  OP_LOAD_BOOL,
  OP_LOAD_LONG,
  OP_LOAD_DOUBLE,
  OP_LOAD_IDENT,
};

#endif
