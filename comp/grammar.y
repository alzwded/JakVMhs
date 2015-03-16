%{
#define YYDEBUG 1
#include "parser.hpp"
#include "lexer.hpp"
 
int yyerror(yyscan_t scanner, const char *msg) {
    // Add error handling routine as needed
    printf("%%%% ERROR %s\n", msg);
}

static size_t indent = 0;
static void log(char const* msg, std::string const& value)
{
    printf("<%s:%s> ", msg, value.c_str());
}
static void logEndStatement()
{
    printf("\n");
    for(size_t i = 0; i < indent; ++i) {
        printf(". ");
    }
}
static void logEnd()
{
    printf("<end:program>\n");
}
%}

%code requires {
#include <string>
#include <sstream>
#define YYSTYPE std::string
 
#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif
 
}


%output "parser.cpp"
%defines "parser.hpp"
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
%token TOKEN_FROM "from"
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

%token INTEGER
%token STRING
%token VAR_ID
%token EOL

/*%type atom
%type VAR
%type IDENT
%type JMPLABEL*/
%%

program_for_real : program { logEnd(); } ;

program : program top_level | ;

top_level : shared_clause | sub | EOL ;

shared_clause : "shared" {
                    log("shared", "");
                    ++indent;
                    logEndStatement();
                } variable_decl_list EOL {
                    log("end", "shared");
                    --indent;
                    logEndStatement();
                }
                ;

variable_decl_list : var_decl | variable_decl_list "," var_decl ;

var_decl : VAR_ID {
               log("declare", $1);
               logEndStatement();
           }
           | VAR_ID { 
               log("declare", $1);
               } "=" expression {
               log("save to", $1);
               logEndStatement();
           }
           ;

variable_list : VAR_ID {
                    log("with", $1);
                }
                | variable_list "," VAR_ID {
                    log("with", $3);
                }
                ;

sub : "sub" IDENT {
          log("sub", $2);
      } "(" opt_variable_list ")" EOL {
          ++indent; logEndStatement();
      } clause_list "end" "sub" EOL { 
          log("end", "sub");
          --indent; logEndStatement();
      }
      ;

opt_variable_list : | variable_list ;

clause_list : sub_clauses | clause_list sub_clauses ;

sub_clauses : var_clause | shared_clause | inner_clauses ;

var_clause : "var" { log("var", ""); ++indent; logEndStatement(); } variable_decl_list EOL { log("end", "var"); --indent; logEndStatement(); } ;

inner_clause_list : inner_clauses | inner_clause_list inner_clauses ;

inner_clauses : if_statement | loop_statement | next_statement | return_statement | for_statement | break_statement | assignment_statement | call_statement | empty_statement | label_decl ;

label_decl : INTEGER ":" {
                 log("label", $1);
                 //logEndStatement();
             }
             | VAR_ID ":" { 
                 log("label", $1);
                 //logEndStatement();
             }
             ;

empty_statement : EOL ;

if_statement : "if" {
                   log("if", "");
                   ++indent;
                   logEndStatement();
               } "(" expression ")" {
                   logEndStatement();
               } JMPLABEL "," JMPLABEL EOL {
                   log("true", $7);
                   logEndStatement();
                   log("false", $9);
                   --indent;
                   logEndStatement();
               }
               ;

loop_statement : "loop" EOL { 
                     log("loop", "infinite");
                     ++indent; logEndStatement();
                 } inner_clause_list "end" "loop" EOL {
                     log("end", "loop");
                     --indent; logEndStatement();
                 }
                 | "loop" {
                     log("loop", "with");
                 } "(" expression ")" EOL {
                     ++indent; logEndStatement();
                 } inner_clause_list "end" "loop" EOL {
                     log("end", "loop");
                     --indent; logEndStatement();
                 }
                 ;

for_statement : "for" IDENT "=" IDENT "," IDENT EOL {
                     log("for", $2);
                     log("start", $4);
                     log("end", $6);
                     ++indent; logEndStatement();
                 } inner_clause_list "end" "for" EOL {
                     log("end", "for");
                     --indent; logEndStatement();
                 }
                 ;

next_statement : "next" EOL { log("next", ""); logEndStatement(); }
               | "next" JMPLABEL EOL { log("next", $2); logEndStatement(); }
               ;

return_statement : "return" EOL { log("return", ""); logEndStatement(); }
                 | "return" expression EOL { log("return", ""); logEndStatement(); }
                 ;

break_statement : "break" EOL { log("break", ""); logEndStatement(); }
                | "break" JMPLABEL EOL { log("break", $2); logEndStatement(); }
                ;

assignment_statement : VAR "=" expression EOL {
                           log("save to", $1);
                           logEndStatement();
                       }
                       ;

regular_call_expression : "call" VAR_ID { log("call", $2); } "(" expression_list ")" { log("end", "call"); }
                 | "call" VAR_ID { log("call", $2); log("end", "call"); }
                 ;

call_expression : "from" STRING { log("from", $2); } regular_call_expression
                | regular_call_expression
                ;

call_statement : call_expression EOL { logEndStatement(); } ;

expression_list : { log("with", ""); } expression | expression_list "," { log("with", ""); } expression ;

atom : IDENT ;

/* the point of this project is not to have a Super Awesome Parser (tm)
   but rather to just _have_ a parser. Therefore, I don't care that I
   don't handle operator precedence, because I can use ()'s */
expression : expression1 | "(" expression ")" ;
expression1 : call_expression | numeric_expression ;
numeric_expression   : expression TOKEN_PLUS expression { log("+", ""); }
                     | expression TOKEN_MINUS expression { log("-", ""); }
                     | expression TOKEN_STAR expression { log("*", ""); }
                     | expression TOKEN_SLASH expression { log("/", ""); }
                     | expression TOKEN_PERCENT expression { log("%", ""); }
                     | TOKEN_MINUS expression { log("-", "unary"); }
                     | expression TOKEN_EQUALS expression { log("==", ""); }
                     | expression TOKEN_AND expression { log("&&", ""); }
                     | expression TOKEN_OR expression { log("||", ""); }
                     | expression TOKEN_XOR expression { log("^^", ""); }
                     | expression TOKEN_BITAND expression { log("&", ""); }
                     | expression TOKEN_BITOR expression { log("|", ""); }
                     | expression TOKEN_BITXOR expression { log("^", ""); }
                     | expression TOKEN_LTE expression { log("<=", ""); }
                     | expression TOKEN_GTE expression { log(">=", ""); }
                     | expression TOKEN_LT expression { log("<", ""); }
                     | expression TOKEN_GT expression { log(">", ""); }
                     | TOKEN_BITNEG expression { log("~", ""); }
                     | TOKEN_NOT expression { log("!", ""); }
                     | TOKEN_DEREF VAR { log("@", $2); }
                     | atom { log("atom", $1); }
                     ;

IDENT : INTEGER { $$ = $1; } | STRING { $$ = $1; } | VAR { $$ = $1; } ;
JMPLABEL : TOKEN_MINUS { $$.assign("-"); } | IDENT { $$ = $1; } ;
VAR : VAR_ID { $$ = $1; } | VAR_ID { log("deref", $1); } "[" expression "]" { log("end", "deref"); $$ = std::string("*").append($1); } ;

%%

#include <cstdio>
 
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
        L("  b = (c + a) * (b + 1)")
        L("  c = @b")
        L("  c = call f(a, b)")
        L("  call f")
        L("  for c = a, b")
        L("    a = a + 1")
        L("    if(a == (3 + 1)) 10, -")
        L("    next")
        L("10: a = a + 1")
        L("  end for")
        L("  a = from \"lib\" call utility")
        L("  a = from \"lib\" call utility + call f(41 + a[3 * 5 + 2], call g(0)) + from \"lib\" call utility")
        L("  call f(a[3 + 2])")
        L("  a[0] = 31")
        L("return 0")
        L("return")
        L("end sub")
        ;
 
    doparse(test);

    parse_stdin();
 
    return 0;
}
