#include "node.h"
#include "parser.h"
#include "lex.yy.h"
#include "y.tab.h"
#include "codegen.c"
int yyparse();

int main(void)
{
  parser_state p;
  p.node = NULL;
  yylex_init(&p.scanner);
  yyset_in(stdin, p.scanner);
  if (yyparse(&p)) {
    yylex_destroy(p.scanner);
    exit(1);
  }
  print_node(p.node, 0);
  scope* s = new_scope();
  codegen(s, p.node);
  print_codes(s);
  free_scope(s);
  yylex_destroy(p.scanner);
}
