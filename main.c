#include "node.h"
#include "lex.yy.h"
#include "y.tab.h"
int yyparse();

int main(void)
{
  yyscan_t scanner;
  yylex_init(&scanner);
  yyset_in(stdin, scanner);
  if (yyparse(scanner)) {
    fprintf(stderr, "Error!\n");
    yylex_destroy(scanner);
    exit(1);
  }
  yylex_destroy(scanner);
}
