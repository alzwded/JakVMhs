%{
#define YYDEBUG 1
#include "parser.h"
#include "lexer.h"
 
int yyerror(yyscan_t scanner, const char *msg) {
    // Add error handling routine as needed
    printf("%%%% ERROR %s\n", msg);
}
%}

%code requires {
 
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif
 
}

%union {
    int num;
    char* str;
}

%output "parser.c"
%defines "parser.h"
%define api.pure
%lex-param { yyscan_t scanner }
%parse-param { yyscan_t scanner }

%token TOKEN_SHARED "shared"
%token TOKEN_VAR "var"
%token TOKEN_SUB "sub"
%token TOKEN_CALL "call"
%token TOKEN_RETURN "return"
%token TOKEN_NEXT "next"
%token TOKEN_BREAK "break"
%token TOKEN_LPAREN "("
%token TOKEN_RPAREN ")"
%token TOKEN_COMMA ","
%token TOKEN_END "end"
%token TOKEN_IF "if"
%token TOKEN_LOOP "loop"
%token TOKEN_FOR "for"
%token TOKEN_ASSIGN "="
%token TOKEN_SQLPARENT "["
%token TOKEN_SQRPARENT "]"
%token TOKEN_COLON ":"

%token TOKEN_EQUALS "=="
%token TOKEN_LT "<"
%token TOKEN_LTE "<="
%token TOKEN_GT ">"
%token TOKEN_GTE ">="
%token TOKEN_NOT "!"
%token TOKEN_PLUS "+"
%token TOKEN_MINUS "-"
%token TOKEN_STAR "*"
%token TOKEN_SLASH "/"
%token TOKEN_PERCENT "%"
%token TOKEN_AND "&&"
%token TOKEN_OR "||"
%token TOKEN_XOR "^^"
%token TOKEN_BITAND "&"
%token TOKEN_BITOR "|"
%token TOKEN_BITXOR "^"
%token TOKEN_BITNEG "~"
%token TOKEN_DEREF "@"

%left TOKEN_EQUALS
%nonassoc TOKEN_LT
%nonassoc TOKEN_LTE
%nonassoc TOKEN_GT
%nonassoc TOKEN_GTE
%left TOKEN_NOT
%left TOKEN_PLUS
%left TOKEN_MINUS
%left TOKEN_STAR
%left TOKEN_SLASH
%left TOKEN_PERCENT
%left TOKEN_AND
%left TOKEN_OR
%left TOKEN_XOR
%left TOKEN_BITAND
%left TOKEN_BITOR
%left TOKEN_BITXOR
%left TOKEN_BITNEG
%left TOKEN_DEREF

%token <num> INTEGER
%token <string> STRING
%token <string> VAR_ID
%token EOL

%%

program : program top_level | ;

top_level : shared_clause | sub | EOL ;

shared_clause : "shared" variable_decl_list EOL ;

variable_decl_list : var_decl | variable_decl_list "," var_decl ;

var_decl : VAR_ID | VAR_ID "=" expression ;

variable_list : VAR_ID | variable_list "," VAR_ID ;

sub : "sub" IDENT "(" opt_variable_list ")" EOL clause_list "end" "sub" EOL
      | "sub" IDENT "(" opt_variable_list ")" "end" "sub" EOL
      ;

opt_variable_list : | variable_list ;

clause_list : sub_clauses | clause_list sub_clauses ;

sub_clauses : var_clause | shared_clause | inner_clauses ;

var_clause : "var" variable_decl_list EOL ;

inner_clause_list : inner_clauses | inner_clause_list inner_clauses ;

inner_clauses : if_statement | loop_statement | next_statement | return_statement | for_statement | break_statement | assignment_statement | call_statement | empty_statement | label_decl ;

label_decl : INTEGER ":" | VAR_ID ":" ;

empty_statement : EOL ;

if_statement : "if" "(" expression ")" JMPLABEL "," JMPLABEL EOL ;

loop_statement : "loop" EOL inner_clause_list "end" "loop" EOL
                 | "loop" "(" expression ")" EOL inner_clause_list "end" "loop" EOL
                 ;

for_statement : "for" IDENT "=" IDENT "," IDENT EOL inner_clause_list "end" "for" EOL ;

next_statement : "next" EOL | "next" JMPLABEL EOL ;

return_statement : "return" EOL | "return" expression EOL ;

break_statement : "break" EOL | "break" JMPLABEL EOL ;

assignment_statement : VAR "=" expression EOL ;

call_expression : "call" VAR_ID "(" expression_list ")"
                 | "call" VAR_ID
                 ;
call_statement : call_expression EOL ;

expression_list : expression | expression_list "," expression ;

atom : IDENT ;

/* the point of this project is not to have a Super Awesome Parser (tm)
   but rather to just _have_ a parser. Therefore, I don't care that I
   don't handle operator precedence, because I can use ()'s */
expression : expression1 | "(" expression ")" ;
expression1 : call_expression | numeric_expression ;
numeric_expression   : expression TOKEN_PLUS expression
                     | expression TOKEN_MINUS expression
                     | expression TOKEN_STAR expression
                     | expression TOKEN_SLASH expression
                     | expression TOKEN_PERCENT expression
                     | TOKEN_MINUS expression
                     | expression TOKEN_EQUALS expression
                     | expression TOKEN_AND expression
                     | expression TOKEN_OR expression
                     | expression TOKEN_XOR expression
                     | expression TOKEN_BITAND expression
                     | expression TOKEN_BITOR expression
                     | expression TOKEN_BITXOR expression
                     | expression TOKEN_LTE expression
                     | expression TOKEN_GTE expression
                     | expression TOKEN_LT expression
                     | expression TOKEN_GT expression
                     | TOKEN_BITNEG expression
                     | TOKEN_NOT expression
                     | TOKEN_DEREF VAR
                     | atom
                     ;

IDENT : INTEGER | STRING | VAR ;
JMPLABEL : TOKEN_MINUS | IDENT ;
/*VAR_ID : "[a-zA-Z_][a-zA-Z_0-9]*" ;*/
VAR : VAR_ID | VAR_ID "[" expression "]" ;

%%

#include <stdio.h>
 
int yyparse(yyscan_t scanner);
 
void doparse(const char* expr)
{
    yyscan_t scanner;
    YY_BUFFER_STATE state;
 
    if (yylex_init(&scanner)) {
        // couldn't initialize
        return;
    }
 
    state = yy_scan_string(expr, scanner);
 
    if (yyparse(scanner)) {
        // error parsing
        return;
    }
 
    yy_delete_buffer(state, scanner);
 
    yylex_destroy(scanner);
 
    return;
}

void parse_stdin()
{
    yyscan_t scanner;
    YY_BUFFER_STATE state;
 
    if (yylex_init(&scanner)) {
        // couldn't initialize
        return;
    }

    state = yy_create_buffer(stdin, YY_BUF_SIZE, scanner);
 
    if (yyparse(scanner)) {
        // error parsing
        return;
    }
 
    yy_delete_buffer(state, scanner);
 
    yylex_destroy(scanner);
 
    return;
}

#define L(X) X "\n"

int main(void)
{
    yydebug = 1;
    char test[] = L("shared x = 0, y")
        L("")
        L("sub f(a, b)")
        L("  var c")
        L("  var d = 2")
        L("  var d = 2 + 3")
        L("  c = a")
        //L("       ")
        L("  b = (c + a) * b")
        L("  c = @b")
        L("  c = call f(a, b)")
        L("  call f")
        L("  for c = a, b")
        L("    a = a + 1")
        L("    if(a == (3 + 1)) 10, -")
        L("    next")
        L("10: a = a + 1")
        L("  end for")
        L("return 0")
        L("return")
        L("end sub")
        ;
 
    doparse(test);

    parse_stdin();
 
    return 0;
}
