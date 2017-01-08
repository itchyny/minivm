#include "node.h"
#include "state.h"
#include "lex.yy.h"
#include "y.tab.h"
#include "codegen.c"
int yyparse();

int main(void)
{
  state* p = new_state();
  if (p == NULL)
    exit(1);
  yylex_init(&p->scanner);
  yyset_in(stdin, p->scanner);
  if (yyparse(p)) {
    yylex_destroy(p->scanner);
    exit(1);
  }
  print_node(p->node, 0);
  env* e = new_env();
  codegen(e, p->node);
  print_codes(e);
  execute_codes(e);
  free_env(e);
  yylex_destroy(p->scanner);
  free_state(p);
}
