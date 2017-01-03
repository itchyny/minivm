%{
#include <stdio.h>
#include "y.tab.h"
#define YY_DECL int yylex(YYSTYPE *yylval, void *yyscanner)
int yywrap(yyscan_t yyscanner)
{
  return 1;
}
%}
%option reentrant

%%

"+"    return PLUS;
"-"    return MINUS;
"*"    return TIMES;
"/"    return DIVIDE;

\r\n?|\n   return CR;
[[:blank:]]  ;

[1-9][0-9]* {
    yylval->long_value = atol(yytext);
    return LONG_LITERAL;
}
[0-9]*\.[0-9]* {
    yylval->double_value = strtod(yytext, NULL);
    return DOUBLE_LITERAL;
}

%%
