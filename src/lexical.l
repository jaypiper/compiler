%{
    #include <assert.h>
    #include "Node.h"
    #include "syntax.h"

    int yycolumn = 1;
    #define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

    int get_octal(char* s);
    int get_hex(char* s);
    float get_exp(char* s);
    void switch_to_newline();
    extern int syn_correct;
%}
%option yylineno
digit [0-9]
pdigit [1-9]
uint {pdigit}{digit}*
int [+-]?{uint}
intpow [+-]?[0-9]+
oct [0-7]+
hex [0-9A-Fa-f]+
float {digit}+\.{digit}+ 
ext_float {float}|\.{uint}|{uint}\.
letter [_a-zA-Z]
relop >|<|>=|<=|==|!=
%%
{uint}|0                            { int val = atoi(yytext); yylval.type_pnode = createTokenNode(yylineno, "INT", yytext, &val); return INT; }   /* int */
{float}                             { float val = atof(yytext); yylval.type_pnode = createTokenNode(yylineno, "FLOAT", yytext, &val); return FLOAT;} /* float */
{ext_float}[eE]{intpow}             { float val = get_exp(yytext); yylval.type_pnode = createTokenNode(yylineno, "FLOAT", yytext, &val); return FLOAT; }
0{oct}                              { int val = get_octal(yytext); yylval.type_pnode = createTokenNode(yylineno, "INT", yytext, &val); return INT; }
0(x|X){hex}                         { int val = get_hex(yytext); yylval.type_pnode = createTokenNode(yylineno, "INT", yytext, &val); return INT; }
;                                   { yylval.type_pnode = createTokenNode(yylineno, "SEMI", yytext, NULL); return SEMI; }
,                                   { yylval.type_pnode = createTokenNode(yylineno, "COMMA", yytext, NULL); return COMMA; }
{relop}                             { yylval.type_pnode = createTokenNode(yylineno, "RELOP", yytext, NULL); return RELOP; }
=                                   { yylval.type_pnode = createTokenNode(yylineno, "ASSIGNOP", yytext, NULL); return ASSIGNOP; }
"+"                                 { yylval.type_pnode = createTokenNode(yylineno, "PLUS", yytext, NULL); return PLUS; }
"-"                                 { yylval.type_pnode = createTokenNode(yylineno, "MINUS", yytext, NULL); return MINUS; }
"*"                                 { yylval.type_pnode = createTokenNode(yylineno, "STAR", yytext, NULL); return STAR; }
"/"                                 { yylval.type_pnode = createTokenNode(yylineno, "DIV", yytext, NULL); return DIV; }
"&&"                                { yylval.type_pnode = createTokenNode(yylineno, "AND", yytext, NULL); return AND; }
"||"                                { yylval.type_pnode = createTokenNode(yylineno, "OR", yytext, NULL); return OR; }
"."                                 { yylval.type_pnode = createTokenNode(yylineno, "DOT", yytext, NULL); return DOT; }
"!"                                 { yylval.type_pnode = createTokenNode(yylineno, "NOT", yytext, NULL); return NOT; }
"("                                 { yylval.type_pnode = createTokenNode(yylineno, "LP", yytext, NULL); return LP; }
")"                                 { yylval.type_pnode = createTokenNode(yylineno, "RP", yytext, NULL); return RP; }
"["                                 { yylval.type_pnode = createTokenNode(yylineno, "LB", yytext, NULL); return LB; }
"]"                                 { yylval.type_pnode = createTokenNode(yylineno, "RB", yytext, NULL); return RB; }
"{"                                 { yylval.type_pnode = createTokenNode(yylineno, "LC", yytext, NULL); return LC; } 
"}"                                 { yylval.type_pnode = createTokenNode(yylineno, "RC", yytext, NULL); return RC; }
struct                              { yylval.type_pnode = createTokenNode(yylineno, "STRUCT", yytext, NULL); return STRUCT; }
return                              { yylval.type_pnode = createTokenNode(yylineno, "RETURN", yytext, NULL); return RETURN; }
if                                  { yylval.type_pnode = createTokenNode(yylineno, "IF", yytext, NULL); return IF; }
else                                { yylval.type_pnode = createTokenNode(yylineno, "ELSE", yytext, NULL); return ELSE; }
while                               { yylval.type_pnode = createTokenNode(yylineno, "WHILE", yytext, NULL); return WHILE; }
[ \t]                               {}
[\n]                                { yycolumn = 1;}
int|float                           { yylval.type_pnode = createTokenNode(yylineno, "TYPE", yytext, NULL); return TYPE; }
{letter}({digit}|{letter})*         { yylval.type_pnode = createTokenNode(yylineno, "ID", yytext, NULL); return ID; } /* variable*/
"//"                                { char c = input(); while (c != EOF && c != '\n') c = input(); yycolumn = 1;}
"/*"                                { int beg_no = yylineno; char c1 = 0; char c2 = input(); 
                                        while(c2 != EOF&& !(c1 == '*' && c2 == '/')){
                                            c1 = c2; c2 = input();
                                            // printf("%d\n", c2);
                                        }
                                        if(c2 == EOF){
                                            syn_correct = 0;
                                            printf("error type B at line %d: missing corresponding \"*/\"\n", beg_no);
                                        }
                                        yycolumn = 1;
                                    }               
.                                   { syn_correct = 0; printf("error type A at line %d: Mysterious character: \"%s\"\n", yylineno, yytext);}

%%

int get_octal(char* s){
    assert(strlen(s) > 1 && s[0] == '0');
    int num = 0;
    for(int i = 1; i < strlen(s); i++) num = num * 8 + s[i] - '0';
    return num;
}

int get_hex(char* s){
    assert(strlen(s) > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'));
    int num = 0;
    for(int i = 2; i < strlen(s); i++){
        if(s[i] >= 'a') num = num * 16 + s[i] - 'a' + 10;
        else if(s[i] >= 'A') num = num * 16 + s[i] - 'A' + 10;
        else num = num * 16 + s[i] - '0';
    }
    return num;
}

float get_exp(char* s){
     float num;
     sscanf(s, "%f", &num);
     return num;
}

void switch_to_newline(){
    yyless(yyleng);
    char c = input();
    while(c != '\n' && c) c = input();
    yycolumn = 1;
}
