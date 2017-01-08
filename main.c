#include "node.h"
#include "state.h"
#include "lex.yy.h"
#include "y.tab.h"
#include "codegen.c"
int yyparse();

int main(void)
{
  state* s = new_state();
  if (s == NULL)
    exit(1);
  yylex_init(&s->scanner);
  yyset_in(stdin, s->scanner);
  if (yyparse(s)) {
    yylex_destroy(s->scanner);
    exit(1);
  }
  print_node(s->node, 0);
  env* e = new_env();
  codegen(e, s->node);
  print_codes(e);
  execute_codes(e);
  free_env(e);
  yylex_destroy(s->scanner);
  free_state(s);
}
