%{
#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "state.h"
int yylex();
int yyerror(state*, char const*);
#define YYLEX_PARAM s->scanner
#define cons(a,b) new_cons(s,(a),(b))
#define binop(op,a,b) new_binop(s,(op),(a),(b))
%}

%pure-parser
%parse-param {state *s}
%lex-param {void *scanner}

%union {
  node *node;
}
%token <node> BOOL_LITERAL LONG_LITERAL DOUBLE_LITERAL IDENTIFIER;
%token EQ PLUS MINUS TIMES DIVIDE LPAREN RPAREN PRINT CR
%token IF ELSE ENDIF
%type <node> program statements statement else_opt expression primary

%left PLUS MINUS
%left TIMES DIVIDE

%%

program           : sep_opt statements sep_opt
                    {
                      s->node = $$ = $2;
                    }
                  ;

statements        : statements sep statement
                    {
                      $$ = append($1, cons($3, NULL));
                    }
                  | statement
                    {
                      $$ = cons(nint(NODE_STMTS), cons($1, NULL));
                    }
                  ;

sep               : sep CR
                  | CR
                  ;

sep_opt           : sep
                  |
                  ;

statement         : IDENTIFIER EQ expression
                    {
                      $$ = cons(nint(NODE_ASSIGN), cons($1, $3));
                    }
                  | IF expression sep statements sep else_opt ENDIF
                    {
                      $$ = cons(nint(NODE_IF), cons($2, cons($4, $6)));
                    }
                  | PRINT expression
                    {
                      $$ = cons(nint(NODE_PRINT), $2);
                    }
                  ;

else_opt          :
                    {
                      $$ = NULL;
                    }
                  | ELSE sep statements sep
                    {
                      $$ = $3;
                    }
                  ;

expression        : expression PLUS expression
                    {
                      $$ = binop(PLUS, $1, $3);
                    }
                  | expression MINUS expression
                    {
                      $$ = binop(MINUS, $1, $3);
                    }
                  | expression TIMES expression
                    {
                      $$ = binop(TIMES, $1, $3);
                    }
                  | expression DIVIDE expression
                    {
                      $$ = binop(DIVIDE, $1, $3);
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

primary           : BOOL_LITERAL
                    {
                      $$ = cons(nint(NODE_BOOL), $1);
                    }
                  | LONG_LITERAL
                    {
                      $$ = cons(nint(NODE_LONG), $1);
                    }
                  | DOUBLE_LITERAL
                    {
                      $$ = cons(nint(NODE_DOUBLE), $1);
                    }
                  | IDENTIFIER
                    {
                      $$ = cons(nint(NODE_IDENTIFIER), $1);
                    }
                  ;

%%

int yyerror(state *s, char const *str)
{
    fprintf(stderr, "Error: %s\n", str);
    return 0;
}
