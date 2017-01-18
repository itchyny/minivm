%{
#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "state.h"
int yylex();
int yyerror(state*, char const*);
#define YYLEX_PARAM s->scanner
#define cons(a,b) new_cons(s,(a),(b))
#define uop(op,a) new_uop(s,(op),(a))
#define binop(op,a,b) new_binop(s,(op),(a),(b))
%}

%pure-parser
%parse-param {state *s}
%lex-param {void *scanner}

%union {
  node *node;
}
%token <node> BOOL_LITERAL LONG_LITERAL DOUBLE_LITERAL IDENTIFIER;
%token EQ PLUS MINUS TIMES DIVIDE GT GE EQEQ LT LE
%token LPAREN RPAREN COMMA PRINT CR
%token FUNC RETURN IF ELSEIF ELSE WHILE END
%type <node> program statements statement else_opt expression fargs_opt fargs args_opt args primary

%left OR
%left AND
%nonassoc EQEQ
%left GT GE LT LE
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

statement         : FUNC IDENTIFIER LPAREN fargs_opt RPAREN sep statements sep END
                    {
                      $$ = cons(nint(NODE_FUNCTION), cons($2, cons($4, $7)));
                    }
                  | RETURN expression
                    {
                      $$ = cons(nint(NODE_RETURN), $2);
                    }
                  | IDENTIFIER EQ expression
                    {
                      $$ = cons(nint(NODE_ASSIGN), cons($1, $3));
                    }
                  | IF expression sep statements sep else_opt END
                    {
                      $$ = cons(nint(NODE_IF), cons($2, cons($4, $6)));
                    }
                  | WHILE expression sep statements sep END
                    {
                      $$ = cons(nint(NODE_WHILE), cons($2, $4));
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
                  | ELSEIF expression sep statements sep else_opt
                    {
                      $$ = cons(nint(NODE_IF), cons($2, cons($4, $6)));
                    }
                  | ELSE sep statements sep
                    {
                      $$ = $3;
                    }
                  ;

expression        : IDENTIFIER LPAREN args_opt RPAREN
                    {
                      $$ = cons(nint(NODE_FCALL), cons($1, $3));
                    }
                  | PLUS expression
                    {
                      $$ = uop(PLUS, $2);
                    }
                  | MINUS expression
                    {
                      $$ = uop(MINUS, $2);
                    }
                  | expression OR expression
                    {
                      $$ = binop(OR, $1, $3);
                    }
                  | expression AND expression
                    {
                      $$ = binop(AND, $1, $3);
                    }
                  | expression PLUS expression
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
                  | expression GT expression
                    {
                      $$ = binop(GT, $1, $3);
                    }
                  | expression GE expression
                    {
                      $$ = binop(GE, $1, $3);
                    }
                  | expression EQEQ expression
                    {
                      $$ = binop(EQEQ, $1, $3);
                    }
                  | expression LT expression
                    {
                      $$ = binop(LT, $1, $3);
                    }
                  | expression LE expression
                    {
                      $$ = binop(LE, $1, $3);
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

fargs_opt         :
                    {
                      $$ = NULL;
                    }
                  | fargs
                    {
                      $$ = $1;
                    }
                  ;

fargs             : IDENTIFIER
                    {
                      $$ = cons($1, NULL);
                    }
                  | fargs COMMA IDENTIFIER
                    {
                      $$ = append($1, cons($3, NULL));
                    }
                  ;

args_opt          :
                    {
                      $$ = NULL;
                    }
                  | args
                    {
                      $$ = $1;
                    }
                  ;

args              : expression
                    {
                      $$ = cons($1, NULL);
                    }
                  | args COMMA expression
                    {
                      $$ = append($1, cons($3, NULL));
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
