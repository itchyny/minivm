svm: main.c codegen.c node.c y.tab.c lex.yy.c
	cc -o svm main.c node.c y.tab.c lex.yy.c

y.tab.c y.tab.h: parser.y node.c node.h
	yacc -dvy $<

lex.yy.c lex.yy.h: lexer.l
	lex --header-file=lex.yy.h $<

clean:
	rm -f svm y.tab.c y.tab.h y.output lex.yy.c lex.yy.h
