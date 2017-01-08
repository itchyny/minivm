#ifndef PARSER_H
#define PARSER_H

typedef struct parser_state {
  struct node* node;
  void* scanner;
} parser_state;

#endif
