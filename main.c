#include "node.h"
#include "parser.h"
#include "lex.yy.h"
#include "y.tab.h"
int yyparse();

int main(void)
{
  parser_state ps;
  yyscan_t scanner;
  yylex_init(&scanner);
  yylex_init_extra(&ps, &scanner);
  yyset_in(stdin, scanner);
  if (yyparse(scanner)) {
    yylex_destroy(scanner);
    exit(1);
  }
  print_node(yyget_extra(scanner)->node, 0);
  yylex_destroy(scanner);
}
