%{
#include <stdio.h>
#include <stdlib.h>
int yylex();
int yyerror();
%}

%pure-parser
%parse-param {void *scanner}
%lex-param {void *scanner}

%union {
  long long_value;
  double double_value;
}
%token <long_value> LONG_LITERAL;
%token <double_value> DOUBLE_LITERAL;
%token PLUS MINUS TIMES DIVIDE CR
%type <double_value> expression primary

%left PLUS MINUS
%left TIMES DIVIDE

%%

program            : expression CR
                   {
                     printf(">> %lf\n", $1);
                   }
                   program
                   |
                   ;

expression         : expression PLUS expression
                   {
                     $$ = $1 + $3;
                   }
                   | expression MINUS expression
                   {
                     $$ = $1 - $3;
                   }
                   | expression TIMES expression
                   {
                     $$ = $1 * $3;
                   }
                   | expression DIVIDE expression
                   {
                     $$ = $1 / $3;
                   }
                   | primary
                   ;

primary            : DOUBLE_LITERAL
                   ;

%%

int yyerror(void *scanner, char const *str)
{
    fprintf(stderr, "parser error near %s\n", str);
    return 0;
}
