%{
#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "parser.h"
int yylex();
int yyerror();
parser_state* yyget_extra();
%}

%pure-parser
%parse-param {void *scanner}
%lex-param {void *scanner}

%union {
  node *node;
  long long_value;
  double double_value;
}
%token <long_value> LONG_LITERAL;
%token <double_value> DOUBLE_LITERAL;
%token PLUS MINUS TIMES DIVIDE LPAREN RPAREN CR
%type <node> program statements expression primary

%left PLUS MINUS
%left TIMES DIVIDE

%%

program           : sep_opt statements sep_opt
                    {
                      yyget_extra(scanner)->node = $$ = $2;
                    }
                  ;

statements        : statements sep expression
                    {
                      $$ = cons(nint(NODE_STMTS), cons($1, $3));
                    }
                  | expression
                    {
                      $$ = $1;
                    }
                  ;

sep               : sep CR
                  | CR
                  ;

sep_opt           : sep
                  |
                  ;

expression        : expression PLUS expression
                    {
                      $$ = new_binop(PLUS, $1, $3);
                    }
                  | expression MINUS expression
                    {
                      $$ = new_binop(MINUS, $1, $3);
                    }
                  | expression TIMES expression
                    {
                      $$ = new_binop(TIMES, $1, $3);
                    }
                  | expression DIVIDE expression
                    {
                      $$ = new_binop(DIVIDE, $1, $3);
                    }
                  | LPAREN expression RPAREN
                    {
                      $$ = $2;
                    }
                  | primary
                    {
                      $$ = $1;
                    }
                  ;

primary           : LONG_LITERAL
                    {
                      $$ = cons(nint(NODE_LONG), nptr($1));
                    }
                  | DOUBLE_LITERAL
                    {
                      $$ = cons(nint(NODE_DOUBLE), nptr($1));
                    }
                  ;

%%

int yyerror(void *scanner, char const *str)
{
    fprintf(stderr, "Error: %s\n", str);
    return 0;
}
