%{
#define YYDEBUG 1
#include "parser.hpp"
#include "lexer.hpp"
#include "ast.hpp"
#include "xmldumpvisitor.hpp"

#include <cctype>
#include <string>
#include <deque>
#include <stack>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <unistd.h>


static std::stack<std::shared_ptr<Node>> g_stack;
#define FLAG_SHARED (0x1)
#define FLAG_CLASSIC_LOG (0x2)
#define FLAG_STACK_DUMPS (0x4)
#define FLAG_XML_DUMPS (0x8)
static unsigned g_parserFlags = 0x0;

#ifndef WITH_CLASSIC_LOG
# define WITH_CLASSIC_LOG 1
#endif
#ifndef WITH_STACK_DUMPS
# define WITH_STACK_DUMPS 1
#endif
#ifndef WITH_XML_DUMPS
# define WITH_XML_DUMPS 1
#endif


#ifndef NOT_USED
# define NOT_USED(X) ((void)(X))
#endif

#if defined(WITH_STACK_DUMPS) && WITH_STACK_DUMPS
# include <typeinfo>
# define STACK_DUMP() do{\
    if(!(g_parserFlags & FLAG_STACK_DUMPS)) break; \
    decltype(g_stack) copy = g_stack; \
    std::cerr << "== begin stack dump ==" << std::endl; \
    auto i = 0u; \
    while(!copy.empty()) { \
        std::cerr << ">>> " << i++ << "\t" << typeid(*(copy.top().get())).name() << std::endl; \
        copy.pop(); \
    } \
}while(0)
# define POP(S) do{\
    if(!(g_parserFlags & FLAG_STACK_DUMPS)) { (S).pop(); break; } \
    std::cerr << std::endl; \
    std::cerr << "==== popping" << std::endl; \
    xmldump((S).top()); \
    std::cerr << std::endl; \
    STACK_DUMP(); \
    (S).pop(); \
    std::cerr << "...after pop" << std::endl; \
    if((S).empty()) std::cerr << "empty" << std::endl; \
    else xmldump((S).top()); \
}while(0)
#else
# define STACK_DUMP() do{ ((void*) 0); }while(0)
# define POP(S) ((S).pop())
#endif

#define BINARY_OP(OP) do{\
    STACK_DUMP(); \
    auto right = g_stack.top(); \
    POP(g_stack); \
    auto left = g_stack.top(); \
    POP(g_stack); \
    std::shared_ptr<Node> bop(new BinaryOp(OP, left, right)); \
    g_stack.push(bop); \
}while(0)
#define UNARY_OP(OP) do{\
    STACK_DUMP(); \
    auto right = g_stack.top(); \
    POP(g_stack); \
    std::shared_ptr<Node> bop(new UnaryOp(OP, right)); \
    g_stack.push(bop); \
}while(0)

static void xmldump(std::shared_ptr<Node> const& n)
{
#if defined(WITH_XML_DUMPS) && WITH_XML_DUMPS
    if(!(g_parserFlags & FLAG_XML_DUMPS)) return;
    XMLDumpVisitor v;
    n->Accept(&v);
#else
    NOT_USED(n);
#endif
}

static std::shared_ptr<Node> variableMagic(std::string const& varName)
{
    Node* ret;
    if(std::all_of(&varName.c_str()[0], &varName.c_str()[varName.size()], &isdigit)
    ) {
        ret = new Atom(varName);
    } else if(varName[0] == '*') {
        auto expr = g_stack.top();
        POP(g_stack);
        ret = new Atom((varName.c_str() + 1), expr);
    } else {
        ret = new Atom(varName);
    }
    std::shared_ptr<Node> sRet(ret);
    return sRet;
}
 
int yyerror(yyscan_t scanner, const char *msg) {
    // Add error handling routine as needed
    printf("%%%% ERROR %s\n", msg);
}

static size_t indent = 0;
static void log(char const* msg, std::string const& value)
{
#if defined(WITH_CLASSIC_LOG) && WITH_CLASSIC_LOG
    if(!(g_parserFlags & FLAG_CLASSIC_LOG)) return;
    printf("<%s:%s> ", msg, value.c_str());
#else
    NOT_USED(msg);
    NOT_USED(value);
#endif
}
static void logEndStatement()
{
#if defined(WITH_CLASSIC_LOG) && WITH_CLASSIC_LOG
    if(!(g_parserFlags & FLAG_CLASSIC_LOG)) return;
    printf("\n");
    for(size_t i = 0; i < indent; ++i) {
        printf(". ");
    }
#endif
}
static void logEnd()
{
#if defined(WITH_CLASSIC_LOG) && WITH_CLASSIC_LOG
    if(!(g_parserFlags & FLAG_CLASSIC_LOG)) return;
    printf("<end:program>\n");
#endif
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
%token TOKEN_NEW "new"
%token TOKEN_FREE "free"
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

/*
%type atom
%type VAR
%type IDENT
%type JMPLABEL
%type DEREFABLE
*/
%%

program_for_real : {
                       std::shared_ptr<Node> prg(new Program());
                       g_stack.push(prg);
                   }
                   program { logEnd(); } ;

program : program top_level 
        | /* EMPTY */
        ;

top_level : shared_clause | sub | EOL ;

shared_clause : "shared" {
                    g_parserFlags |= FLAG_SHARED;
                    log("shared", "");
                    ++indent;
                    logEndStatement();
                } variable_decl_list EOL {
                    log("end", "shared");
                    --indent;
                    logEndStatement();
                    g_parserFlags &= ~FLAG_SHARED;
                }
                ;

variable_decl_list : var_decl {
                         auto n = g_stack.top();
                         POP(g_stack);
                         auto parent = std::dynamic_pointer_cast<ContainerNode>(g_stack.top());
                         parent->Add(n);
                         xmldump(parent);
                     }
                     | variable_decl_list "," var_decl {
                         auto n = g_stack.top();
                         POP(g_stack);
                         auto parent = std::dynamic_pointer_cast<ContainerNode>(g_stack.top());
                         parent->Add(n);
                         xmldump(parent);
                     }
                     ;

var_decl : VAR_ID {
               STACK_DUMP();
               log("declare", $1);
               logEndStatement();
               if(g_parserFlags & FLAG_SHARED) {
                   std::shared_ptr<Empty> empty(new Empty());
                   std::shared_ptr<SharedDecl> shDecl(new SharedDecl($1, empty));
                   g_stack.push(shDecl);
               } else {
                   std::shared_ptr<Empty> empty(new Empty());
                   std::shared_ptr<VarDecl> varDecl(new VarDecl($1, empty));
                   g_stack.push(varDecl);
               }
           }
           | VAR_ID { 
               log("declare", $1);
               } "=" expression {
               log("save to", $1);
               logEndStatement();
               STACK_DUMP();
               if(g_parserFlags & FLAG_SHARED) {
                   std::shared_ptr<Node> expression = g_stack.top();
                   POP(g_stack);
                   std::shared_ptr<SharedDecl> shDecl(new SharedDecl($1, expression));
                   g_stack.push(shDecl);
               } else {
                   std::shared_ptr<Node> expression = g_stack.top();
                   POP(g_stack);
                   std::shared_ptr<VarDecl> varDecl(new VarDecl($1, expression));
                   g_stack.push(varDecl);
               }
           }
           ;

sub : "sub" IDENT {
          log("sub", $2);
          std::shared_ptr<Sub> sub(new Sub($2));
          g_stack.push(sub);
          STACK_DUMP();
      } "(" opt_variable_list ")" EOL {
          ++indent; logEndStatement();
      } clause_list "end" "sub" EOL { 
          log("end", "sub");
          --indent; logEndStatement();
          STACK_DUMP();
          auto n = g_stack.top();
          POP(g_stack);
          STACK_DUMP();
          auto parent = std::dynamic_pointer_cast<ContainerNode>(g_stack.top());
          parent->Add(n);
      }
      ;

variable_list : VAR_ID {
                    log("with", $1);
                    auto parent = std::dynamic_pointer_cast<Sub>(g_stack.top());
                    parent->AddParam($1);
                }
                | variable_list "," VAR_ID {
                    log("with", $3);
                    auto parent = std::dynamic_pointer_cast<Sub>(g_stack.top());
                    parent->AddParam($3);
                }
                ;

opt_variable_list : | variable_list ;

clause_list : { STACK_DUMP(); } sub_clauses | clause_list {STACK_DUMP();} sub_clauses ;

sub_clauses : var_clause | shared_clause | inner_clauses ;

var_clause : "var" {
                 log("var", ""); ++indent; logEndStatement();
             } variable_decl_list EOL {
                 log("end", "var"); --indent; logEndStatement();
             }
             ;

inner_clause_list : inner_clauses | inner_clause_list inner_clauses ;

inner_clauses : label_decl true_inner_clauses {
                   std::string lblName($1);
                   auto n = g_stack.top();
                   POP(g_stack);
                   std::shared_ptr<Labelled> label(new Labelled(lblName, n));
                   //g_stack.push(label);
                   auto parent = std::dynamic_pointer_cast<ContainerNode>(g_stack.top());
                   parent->Add(label);
                }
                | true_inner_clauses {
                   auto n = g_stack.top();
                   POP(g_stack);
                   auto parent = std::dynamic_pointer_cast<ContainerNode>(g_stack.top());
                   parent->Add(n);
                }
                ;

true_inner_clauses : if_statement | loop_statement | next_statement | return_statement | for_statement | break_statement | assignment_statement | call_statement | empty_statement | free_statement ;

label_decl : INTEGER ":" {
                 log("label", $1);
                 $$ = $1;
                 //logEndStatement();
             }
             | VAR_ID ":" { 
                 log("label", $1);
                 $$ = $1;
                 //logEndStatement();
             }
             ;

empty_statement : EOL {
                    std::shared_ptr<Empty> empty(new Empty());
                    g_stack.push(empty);
                }
                ;

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

                   auto operand = g_stack.top();
                   POP(g_stack);
                   std::shared_ptr<If> cond(new If(operand, $7, $9));
                   g_stack.push(cond);
               }
               ;

loop_statement : "loop" EOL { 
                     log("loop", "infinite");
                     ++indent; logEndStatement();
                     std::shared_ptr<Empty> empty(new Empty());
                     std::shared_ptr<Loop> loop(new Loop(empty));
                     g_stack.push(loop);
                 } inner_clause_list "end" "loop" EOL {
                     log("end", "loop");
                     --indent; logEndStatement();
                 }
                 | "loop" {
                     log("loop", "with");
                 } "(" expression ")" EOL {
                     ++indent; logEndStatement();
                     auto cond = g_stack.top();
                     POP(g_stack);
                     std::shared_ptr<Loop> loop(new Loop(cond));
                     g_stack.push(loop);
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
                     auto for_to = variableMagic($6);
                     auto for_from = variableMagic($4);
                     std::shared_ptr<Empty> empty(new Empty());
                     std::shared_ptr<For> loop(new For($2, for_from, for_to, empty));
                     g_stack.push(loop);
                 } inner_clause_list "end" "for" EOL {
                     log("end", "for");
                     --indent; logEndStatement();
                 }
                 ;

next_statement : "next" EOL { log("next", ""); logEndStatement();
                   std::shared_ptr<Empty> empty(nullptr);
                   std::shared_ptr<Next> next(new Next(empty));
                   g_stack.push(next);
               }
               | "next" JMPLABEL EOL { log("next", $2); logEndStatement();
                   auto n = variableMagic($2);
                   std::shared_ptr<Next> next(new Next(n));
                   g_stack.push(next);
               }
               ;

return_statement : "return" EOL { log("return", ""); logEndStatement();
                     std::shared_ptr<Empty> empty(nullptr);
                     std::shared_ptr<Return> ret(new Return(empty));
                     g_stack.push(ret);
                 }
                 | "return" expression EOL { log("return", ""); logEndStatement();
                     auto n = g_stack.top();
                     POP(g_stack);
                     std::shared_ptr<Return> ret(new Return(n));
                     g_stack.push(ret);
                 }
                 ;

break_statement : "break" EOL { log("break", ""); logEndStatement();
                    std::shared_ptr<Empty> empty(nullptr);
                    std::shared_ptr<Break> next(new Break(empty));
                    g_stack.push(next);
                }
                | "break" JMPLABEL EOL { log("break", $2); logEndStatement();
                    auto n = variableMagic($2);
                    std::shared_ptr<Break> next(new Break(n));
                    g_stack.push(next);
                }
                ;

assignment_statement : VAR "=" expression EOL {
                           log("save to", $1);
                           logEndStatement();
                           auto expr = g_stack.top();
                           POP(g_stack);
                           auto var = variableMagic($1);
                           std::shared_ptr<Assignation> ass(new Assignation(var, expr));
                           g_stack.push(ass);
                       }
                     | VAR "=" new_expression EOL {
                           log("save to", $1);
                           logEndStatement();
                           auto expr = g_stack.top();
                           POP(g_stack);
                           auto var = variableMagic($1);
                           std::shared_ptr<Assignation> ass(new Assignation(var, expr));
                           g_stack.push(ass);
                       }
                       ;


new_expression : "new" expression { log("allocate", "");
                   auto n = g_stack.top();
                   POP(g_stack);
                   std::shared_ptr<Allocation> alloc(new Allocation(n));
                   g_stack.push(alloc);
               }
               ;

free_statement : "free" VAR EOL { log("free", $2); logEndStatement();
                   auto var = variableMagic($2);
                   std::shared_ptr<Deallocation> dealloc(new Deallocation(var));
                   g_stack.push(dealloc);
               }
               ;

call_expression : "from" STRING "call" VAR_ID
                {
                    log("from", $2); 
                    log("call", $4);
                    std::shared_ptr<Call> call(new Call($4, $2));
                    g_stack.push(call);
                }  "(" expression_list ")"
                {
                    log("end", "call");
                }

                | "from" STRING "call" VAR_ID
                {
                    log("from", $2); 
                    log("call", $4);
                    std::shared_ptr<Call> call(new Call($4, $2));
                    g_stack.push(call);
                    log("end", "call");
                }

                | "call" VAR_ID
                {
                    log("call", $2);
                    std::shared_ptr<Call> call(new Call($2, ""));
                    g_stack.push(call);
                } "(" expression_list ")"
                {
                    log("end", "call");
                }

                | "call" VAR_ID
                {
                    log("call", $2);
                    std::shared_ptr<Call> call(new Call($2, ""));
                    g_stack.push(call);
                    log("end", "call");
                }
                ;

call_statement : call_expression EOL { logEndStatement(); } ;

expression_list : { log("with", ""); } expression {
                      STACK_DUMP();
                      auto n = g_stack.top();
                      POP(g_stack);
                      auto call = std::dynamic_pointer_cast<Call>(g_stack.top());
                      call->AddParam(n);
                  }
                  | expression_list "," {
                      log("with", "");
                  } expression {
                      STACK_DUMP();
                      auto n = g_stack.top();
                      POP(g_stack);
                      auto call = std::dynamic_pointer_cast<Call>(g_stack.top());
                      call->AddParam(n);
                  }
                  ;

atom : IDENT { 
         STACK_DUMP();
         auto n = variableMagic($1);
         g_stack.push(n);
         STACK_DUMP();
     }
     ;

/* the point of this project is not to have a Super Awesome Parser (tm)
   but rather to just _have_ a parser. Therefore, I don't care that I
   don't handle operator precedence, because I can use ()'s */
expression : expression1 | "(" expression ")" ;
expression1 : call_expression | numeric_expression ;
numeric_expression   : expression TOKEN_PLUS expression { log("+", ""); BINARY_OP("+"); }
                     | expression TOKEN_MINUS expression { log("-", ""); BINARY_OP("-"); }
                     | expression TOKEN_STAR expression { log("*", ""); BINARY_OP("*"); }
                     | expression TOKEN_SLASH expression { log("/", ""); BINARY_OP("/"); }
                     | expression TOKEN_PERCENT expression { log("%", ""); BINARY_OP("%"); }
                     | expression TOKEN_EQUALS expression { log("==", ""); BINARY_OP("=="); }
                     | expression TOKEN_AND expression { log("&&", ""); BINARY_OP("&&"); }
                     | expression TOKEN_OR expression { log("||", ""); BINARY_OP("||"); }
                     | expression TOKEN_XOR expression { log("^^", ""); BINARY_OP("^^"); }
                     | expression TOKEN_BITAND expression { log("&", ""); BINARY_OP("&"); }
                     | expression TOKEN_BITOR expression { log("|", ""); BINARY_OP("|"); }
                     | expression TOKEN_BITXOR expression { log("^", ""); BINARY_OP("^"); }
                     | expression TOKEN_LTE expression { log("<=", ""); BINARY_OP("<="); }
                     | expression TOKEN_GTE expression { log(">=", ""); BINARY_OP(">="); }
                     | expression TOKEN_LT expression { log("<", ""); BINARY_OP("<"); }
                     | expression TOKEN_GT expression { log(">", ""); BINARY_OP(">"); }
                     | TOKEN_MINUS expression { log("-", "unary"); UNARY_OP("-"); }
                     | TOKEN_BITNEG expression { log("~", ""); UNARY_OP("~"); }
                     | TOKEN_NOT expression { log("!", ""); UNARY_OP("!"); }
                     | TOKEN_DEREF DEREFABLE {
                         STACK_DUMP();
                         log("@", $2);
                         auto var = variableMagic($2);
                         std::shared_ptr<Node> ref(new RefVar(var));
                         g_stack.push(ref);
                     }
                     | atom { log("atom", $1); }
                     ;

DEREFABLE : INTEGER { $$ = $1; } | VAR { $$ = $1; }
IDENT : INTEGER { $$ = $1; } | STRING { $$ = $1; } | VAR { $$ = $1; } ;
JMPLABEL : TOKEN_MINUS { $$.assign("-"); } | IDENT { $$ = $1; } ;
VAR : VAR_ID { $$ = $1; } | VAR_ID { log("deref", $1); } "[" expression "]" { log("end", "deref"); $$ = std::string("*").append($1); } ;

%%

#include <cstdio>
 
int yyparse(yyscan_t scanner);
 
void parse_string(const char* expr)
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

void parse_stream(FILE* f)
{
    yyscan_t scanner;
    YY_BUFFER_STATE state;
 
    if (yylex_init(&scanner)) {
        // couldn't initialize
        return;
    } else {
        yyset_in(f, scanner);
    }

    state = yy_create_buffer(f, YY_BUF_SIZE, scanner);
 
    if (yyparse(scanner)) {
        // error parsing
        return;
    }
 
    yy_delete_buffer(state, scanner);
 
    yylex_destroy(scanner);
 
    return;
}

#define L(X) X "\n"
#include <fstream>

static char const* optString = "hdo:x:";

void usage(char const* imgname)
{
    std::cerr << "Usage: imgname [-" << optString << "] files..." << std::endl;
    exit(255);
}

int main(int argc, char* argv[])
{
    char c;
    std::string outputFname("");
    bool xml(true);

    yydebug = 0;

    while((c = getopt(argc, argv, optString)) != -1) {
        switch(c) {
        case 'd':
            yydebug = 1;
            g_parserFlags |= FLAG_CLASSIC_LOG | FLAG_STACK_DUMPS | FLAG_XML_DUMPS;
            break;
        case 'o':
            outputFname.assign(optarg);
            xml = false;
            break;
        case 'x':
            outputFname.assign(optarg);
            xml = true;
            break;
        case '?':
            if(optopt == 'o' || optopt == 'x') {
                outputFname.assign("");
                break;
            }
            /*FALLTHROUGH*/
        case 'h':
            usage(argv[0]);
            break;
        default:
            abort();
        }
    }

    if(optind >= argc) {
        usage(argv[0]);
    }

    if(outputFname == "-") {
        outputFname.assign("");
    }

    for(auto index = optind; index < argc; ++index) {
        if(strcmp(argv[index], "-") == 0) {
            parse_stream(stdin);
            continue;
        }

        FILE* f = fopen(argv[index], "r");

        if(!f) {
            std::cerr << "failed to open " << argv[index] << std::endl;
            exit(1);
        }

        parse_stream(f);

        fclose(f);
    }

    std::ofstream ff;
    if(!outputFname.empty()) {
        ff.open(outputFname, std::ios::out);
        if(!ff.good()) {
            std::cerr << "failed to open " << outputFname << std::endl;
            exit(1);
        }
    }
    std::ostream& f = (outputFname.empty()) ? std::cout : ff;

    Visitor* v;
    if(xml) {
        v = new XMLDumpVisitor(f);
    } else {
        std::cerr << "not implemented" << std::endl;
        exit(2);
    }

    while(!g_stack.empty()) {
        g_stack.top()->Accept(v);
        g_stack.pop();
    }

    if(ff.is_open()) ff.close();

    delete v;
 
    return 0;
}
