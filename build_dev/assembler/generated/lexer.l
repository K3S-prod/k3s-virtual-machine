%{
    #include "interpreter/generated/opcodes.h"
    #include <cstdlib>
    #include <cstdio>
    #include <cassert>
    #include "grammar.l.hpp"

    size_t ASM_LINE_NO = 1;
    extern "C" int yylex(void);
    void UnknownTokenHandle() {
        printf("(line_no:%lu): Unknown token: %s\n", ASM_LINE_NO, yytext);
        exit (-1);
    }
%}

doubl [0-9]+(\\.[0-9]+)? 

integ [+-]?[0-9]+

%%
[#].+$          {}
[#]$            {}
[\n]            { ASM_LINE_NO ++; }

".def"          { return FUNCTION_KEYW; }
".num"          { return NUM_KEYW; }

[{]             { return B_BEGIN; }
[:]             { return COLON; }
[}]             { return B_END; }

{doubl}         { return NUM; }

[ ]+            {}
{integ}         {
                    yylval = atoi(yytext);
                    //printf("int(%d)\n", yylval);
                    return IMM_LITERAL;
                }
[r]{integ}      {
                    yylval = atoi(++yytext);
                    //printf("reg(%d)\n", yylval);
                    return REG;
                }

[j][u][m][p] {
    yylval = static_cast<int>(k3s::Opcode::JUMP);
    return(OPC_I8);
}
[b][l][e] {
    yylval = static_cast<int>(k3s::Opcode::BLE);
    return(OPC_I8);
}
[c][a][l][l] {
    yylval = static_cast<int>(k3s::Opcode::CALL);
    return(OPC);
}
[r][e][t] {
    yylval = static_cast<int>(k3s::Opcode::RET);
    return(OPC);
}
[l][d][a][i] {
    yylval = static_cast<int>(k3s::Opcode::LDAI);
    return(OPC_I8);
}
[l][d][a] {
    yylval = static_cast<int>(k3s::Opcode::LDA);
    return(OPC_R8);
}
[s][t][a] {
    yylval = static_cast<int>(k3s::Opcode::STA);
    return(OPC_R8);
}
[m][o][v] {
    yylval = static_cast<int>(k3s::Opcode::MOV);
    return(OPC_R4_R4);
}
[a][d][d] {
    yylval = static_cast<int>(k3s::Opcode::ADD);
    return(OPC_R4_R4);
}
[s][u][b] {
    yylval = static_cast<int>(k3s::Opcode::SUB);
    return(OPC_R4_R4);
}
[d][i][v] {
    yylval = static_cast<int>(k3s::Opcode::DIV);
    return(OPC_R4_R4);
}
[m][u][l] {
    yylval = static_cast<int>(k3s::Opcode::MUL);
    return(OPC_R4_R4);
}
[a][d][d][2] {
    yylval = static_cast<int>(k3s::Opcode::ADD2);
    return(OPC_R8);
}
[s][u][b][2] {
    yylval = static_cast<int>(k3s::Opcode::SUB2);
    return(OPC_R8);
}
[d][i][v][2] {
    yylval = static_cast<int>(k3s::Opcode::DIV2);
    return(OPC_R8);
}
[m][u][l][2] {
    yylval = static_cast<int>(k3s::Opcode::MUL2);
    return(OPC_R8);
}
[d][e][c][a] {
    yylval = static_cast<int>(k3s::Opcode::DECA);
    return(OPC);
}
[n][e][w][a][r][r] {
    yylval = static_cast<int>(k3s::Opcode::NEWARR);
    return(OPC_R8);
}
[s][e][t][e][l][e][m] {
    yylval = static_cast<int>(k3s::Opcode::SETELEM);
    return(OPC_R4_R4);
}
[g][e][t][e][l][e][m] {
    yylval = static_cast<int>(k3s::Opcode::GETELEM);
    return(OPC_R4_R4);
}
[d][u][m][p] {
    yylval = static_cast<int>(k3s::Opcode::DUMP);
    return(OPC_R8);
}
[d][u][m][p][a] {
    yylval = static_cast<int>(k3s::Opcode::DUMPA);
    return(OPC);
}
[g][e][t][a][r][g][0] {
    yylval = static_cast<int>(k3s::Opcode::GETARG0);
    return(OPC_R8);
}
[s][e][t][r][e][t][0] {
    yylval = static_cast<int>(k3s::Opcode::SETRET0);
    return(OPC_R8);
}
[s][e][t][a][r][g][0] {
    yylval = static_cast<int>(k3s::Opcode::SETARG0);
    return(OPC_R8);
}
[g][e][t][r][e][t][0] {
    yylval = static_cast<int>(k3s::Opcode::GETRET0);
    return(OPC_R8);
}

[E][O][F] {
    yylval = 0;
    printf("EOF(%d)\n", yylval);
    return EOF;
}

[a-zA-Z_][a-zA-Z0-9_]*  { return IDENTIFIER; }

. {
    UnknownTokenHandle();
}

%%
