%{
#include <stdio.h>
#include "node.h"
#include "state.h"
#include "y.tab.h"
#define YY_DECL int yylex(YYSTYPE *yylval, void *yyscanner)
int yywrap(yyscan_t yyscanner)
{
  return 1;
}
%}
%option reentrant

%%

"="    return EQ;
"+"    return PLUS;
"-"    return MINUS;
"*"    return TIMES;
"/"    return DIVIDE;
"("    return LPAREN;
")"    return RPAREN;
"print"    return PRINT;

\r\n?|\n   return CR;
[[:blank:]]  ;

"if"       return IF;
"else"     return ELSE;
"endif"    return ENDIF;

"true" {
  yylval->node = (node*)(1);
  return BOOL_LITERAL;
}
"false" {
  yylval->node = (node*)(0);
  return BOOL_LITERAL;
}
[1-9][0-9]*|0 {
  yylval->node = (node*)(yytext);
  return LONG_LITERAL;
}
[0-9]+\.[0-9]+ {
  yylval->node = (node*)(yytext);
  return DOUBLE_LITERAL;
}
[A-Za-z][A-Za-z0-9]* {
  char* s = malloc(yyleng + 1); // TODO: free
  strcpy(s, yytext);
  yylval->node = (node*)s;
  return IDENTIFIER;
}

%%
