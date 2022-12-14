%{
#include <cstdio>
#include <assembler/assembler.h>

#define YYDEBUG 1

extern "C" FILE *yyin;
extern "C" int yylex (void);

extern char *yytext;
extern size_t ASM_LINE_NO;

void yyerror(const char *s)
{
    fprintf(stderr, "(line_no:%lu): error: %s: '%s'\n", ASM_LINE_NO, s, yytext);
}

%}

/* declare tokens */
%token REG
%token NUM
%token IMM_LITERAL
%token SPACE
%token FUNCTION_KEYW
%token NUM_KEYW
%token IDENTIFIER
%token B_BEGIN
%token COLON
%token B_END

%token OPC_R4_R4 
%token OPC_R8 
%token OPC_I8 
%token OPC 

%%

program:
    declaration |
    program declaration |
    %empty;

declaration:
    function |
    num;

function:
    FUNCTION_KEYW IDENTIFIER { k3s::AsmEncoder::DeclareAndDefineFunction(yytext); } B_BEGIN instructions B_END { k3s::AsmEncoder::CheckLabelsResolved(); };

num:
    NUM_KEYW IDENTIFIER { k3s::AsmEncoder::DeclareNum(yytext); } NUM { k3s::AsmEncoder::DefineNum(yytext); }
instructions:
    instruction_or_label instructions |
    instruction_or_label;

IMM:
    IDENTIFIER { $$ = k3s::AsmEncoder::TryResolveName(yytext); } |
    IMM_LITERAL { $$ = $1; };

instruction_or_label:
    instruction |
    label;

label:
    IDENTIFIER { k3s::AsmEncoder::DefineLabel(yytext); } COLON;

instruction:
    OPC_R4_R4 REG REG {
        k3s::AsmEncoder::Encode<8, 4, 4>($1, $2, $3);
    } |
    OPC_R8 REG {
        k3s::AsmEncoder::Encode<8, 8>($1, $2);
    } |
    OPC_I8 IMM {
        k3s::AsmEncoder::Encode<8, 8>($1, $2);
    } |
    OPC {
        k3s::AsmEncoder::Encode<8>($1);
    } ;

%%

namespace k3s {

AsmEncoder ENCODER;

int AsmEncoder::Process(FILE *file)
{
    yydebug = 0;
    yyin = file;
    return yyparse();
}

} // namespace k3s
