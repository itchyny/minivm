%{
#include <stdio.h>
#include "y.tab.h"
int yywrap(void)
{
  return 1;
}
%}

%%

"+"    return PLUS;
"-"    return MINUS;
"*"    return TIMES;
"/"    return DIVIDE;

"\n"   return CR;
[[:blank:]]  ;

[1-9][0-9]* {
    yylval.long_value = atol(yytext);
    return LONG_LITERAL;
}
[0-9]*\.[0-9]* {
    yylval.double_value = strtod(yytext, NULL);
    return DOUBLE_LITERAL;
}

%%
