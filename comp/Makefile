comp.bin: lexer.c parser.c
	g++ -g --std=gnu++11 -o comp.bin parser.c lexer.c

lexer.c: lexer.l
	flex lexer.l

parser.c: grammar.y
	bison -g -v -d -o log grammar.y

clean:
	rm -f parser.output parser.c parser.h parser.dot comp.bin
