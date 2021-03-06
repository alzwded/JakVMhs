%{
 
/*
 * Lexer.l file
 * To generate the lexical analyzer run: "flex Lexer.l"
 */
 
#include "parser.hpp"
 
#include <cstdio>
 
%}
 
%option outfile="lexer.cpp" header-file="lexer.hpp"
%option warn nodefault
 
%option reentrant noyywrap never-interactive nounistd
%option bison-bridge
 
 
NUMBER      [0-9]+
HEXNUMBER   [0][x][0-9A-Fa-f]+
OCTNUMBER   [0][0-7]+
VAR_ID      [a-zA-Z_][a-zA-Z_0-9]*

STRING      L?\"(\\.|[^\\"])*\"
 
%%
 
[ \t]*          { /* Skip blanks. */ }
[\n]            { return EOL; }
"new"           { return TOKEN_NEW; }
"free"          { return TOKEN_FREE; }
"shared"        { return TOKEN_SHARED; }
"var"           { return TOKEN_VAR; }
"sub"           { return TOKEN_SUB; }
"call"          { return TOKEN_CALL; }
"return"        { return TOKEN_RETURN; }
"next"          { return TOKEN_NEXT; }
"break"         { return TOKEN_BREAK; }
"from"          { return TOKEN_FROM; }
"("             { return TOKEN_LPAREN; }
")"             { return TOKEN_RPAREN; }
","             { return TOKEN_COMMA; }
"end"           { return TOKEN_END; }
"if"            { return TOKEN_IF; }
"loop"          { return TOKEN_LOOP; }
"for"           { return TOKEN_FOR; }
"=="            { return TOKEN_EQUALS; }
"<="            { return TOKEN_LTE; }
">="            { return TOKEN_GTE; }
[&][&]          { return TOKEN_AND; }
[|][|]          { return TOKEN_OR; }
"^^"            { return TOKEN_XOR; }
[~]             { return TOKEN_BITNEG; }
[<]             { return TOKEN_LT; }
[>]             { return TOKEN_GT; }
[=]             { return TOKEN_ASSIGN; }
[!]             { return TOKEN_NOT; }
[+]             { return TOKEN_PLUS; }
[-]             { return TOKEN_MINUS; }
[*]             { return TOKEN_STAR; }
[/]             { return TOKEN_SLASH; }
[%]             { return TOKEN_PERCENT; }
[[]             { return TOKEN_SQLPARENT; }
[]]             { return TOKEN_SQRPARENT; }
[&]             { return TOKEN_BITAND; }
[|]             { return TOKEN_BITOR; }
"^"             { return TOKEN_BITXOR; }
[@]             { return TOKEN_DEREF; }
[:]             { return TOKEN_COLON; }
{OCTNUMBER}     { int tempx; sscanf(yytext, "%d", &tempx); std::stringstream ss; ss << tempx; *yylval = ss.str(); return INTEGER; }
{HEXNUMBER}     { int tempx; sscanf(yytext, "%d", &tempx); std::stringstream ss; ss << tempx; *yylval = ss.str(); return INTEGER; }
{NUMBER}        { int tempx; sscanf(yytext, "%d", &tempx); std::stringstream ss; ss << tempx; *yylval = ss.str(); return INTEGER; }
{VAR_ID}        { yylval->assign(yytext); return VAR_ID; }
{STRING}        { char* text = strdup(yytext + 1); text[strlen(text) - 1] = '\0'; yylval->assign(text); free(text); return STRING; }
 
.               {  }
 
%%
 
int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s\n",msg); return 0;
}
