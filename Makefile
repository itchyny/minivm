svm: y.tab.c lex.yy.c
	cc -o svm y.tab.c lex.yy.c

y.tab.c: parser.y
	yacc -dv $<

lex.yy.c: lexer.l
	lex $<
