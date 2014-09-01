comp.bin: lexer.cpp parser.cpp
	g++ -g --std=gnu++11 -o comp.bin parser.cpp lexer.cpp

lexer.cpp: lexer.l
	flex lexer.l

parser.cpp: grammar.y
	bison -g -v -d -o log grammar.y

clean:
	rm -f parser.output parser.c* parser.h* parser.dot comp.bin lexer.c* lexer.h*
