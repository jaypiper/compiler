%{
    #include <stdio.h>  
    #include <Node.h>
    Node* root;
    // char* msg;
    extern int syn_correct;
    extern int yychar;
    void yyerror (char const *s) {
        syn_correct = 0;
        // fprintf (stderr, "Error type B at line %d: %s\n", yylineno, s);
        
        // switch_to_newline();
        // yychar = yylex();
    }
    void syn_newline(){
        // switch_to_newline();
        // yychar = yylex();
    }

%}
%locations
%union {
    Node* type_pnode;
}


%token <type_pnode> INT 
%token <type_pnode> FLOAT 
%token <type_pnode> ID 
%token <type_pnode> SEMI COMMA ASSIGNOP RELOP 
%token <type_pnode> PLUS MINUS STAR DIV
%token <type_pnode> AND OR NOT
%token <type_pnode> DOT
%token <type_pnode> TYPE
%token <type_pnode> LP RP LB RB LC RC
%token <type_pnode> STRUCT RETURN IF ELSE WHILE

%type <type_pnode> program ExtDefList ExtDef Specifier ExtDecList  /* High-level Definitions */
%type <type_pnode> FunDec CompSt VarDec StructSpecifier OptTag DefList Tag /* Specifiers */
%type <type_pnode> VarList ParamDec /* Declarators */
%type <type_pnode> Stmt StmtList Exp /* Statements */
%type <type_pnode> Def DecList Dec Args /* Local Definitions */


/* %nonassoc LOWER_THAN_RP */
/* %nonassoc LOWER_THAN_RB */
/* %nonassoc LOWER_THAN_RC */


%right ASSIGNOP
%left OR AND 
%left RELOP 
%left PLUS MINUS
%left STAR DIV
%right NOT
%left DOT
%left LB RB
%left LP RP

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

/* %nonassoc LOWER_THAN_SEMI */
%nonassoc SEMI

%%

/* High-level Definitions */
program: ExtDefList                             { $$ = createNode(@$.first_line, "program", 1, $1); root = $$;}
    ;
ExtDefList:                                     { $$ = NULL;}
    | ExtDef ExtDefList                         { $$ = createNode(@$.first_line, "ExtDefList", 2, $1, $2); }
    ;
ExtDef: Specifier ExtDecList SEMI               { $$ = createNode(@$.first_line, "ExtDef", 3, $1, $2, $3);}
    | Specifier SEMI                            { $$ = createNode(@$.first_line, "ExtDef", 2, $1, $2);}
    | Specifier FunDec CompSt                   { $$ = createNode(@$.first_line, "ExtDef", 3, $1, $2, $3);}
    | Specifier FunDec SEMI                     { $$ = createNode(@$.first_line, "ExtDef", 3, $1, $2, $3);} //加入函数声明
    /* | Specifier ExtDecList error                { fprintf(stderr, "Error type B at line %d: Missing \";\"\n", @1.first_line); syn_newline();}
    | Specifier error                           { fprintf(stderr, "Error type B at line %d: Missing \";\"\n", @1.first_line); syn_newline();}
    | error SEMI                                { fprintf(stderr, "Error type B at line %d: extdef error\n", @1.first_line);} // error2 */
    ;
ExtDecList: VarDec                              { $$ = createNode(@$.first_line, "ExtDecList", 1, $1);}
    | VarDec COMMA ExtDecList                   { $$ = createNode(@$.first_line, "ExtDecList", 3, $1, $2, $3);}
    ;

/* Specifiers */
Specifier: TYPE                                 { $$ = createNode(@$.first_line, "Specifier", 1, $1);}
    | StructSpecifier                           { $$ = createNode(@$.first_line, "Specifier", 1, $1);}
    ;
StructSpecifier: STRUCT OptTag LC DefList RC    { $$ = createNode(@$.first_line, "StructSpecifier", 5, $1, $2, $3, $4, $5);}
    | STRUCT Tag                                { $$ = createNode(@$.first_line, "StructSpecifier", 2, $1, $2);}
    /* | STRUCT error LC DefList RC                { fprintf(stderr, "Error type B at line %d: wrong OptTag before \"{\"\n", @1.first_line);}
    | STRUCT OptTag LC error RC                 { fprintf(stderr, "Error type B at line %d: wrong arguments list between \"(\" and \")\"\n", @1.first_line);}
	| STRUCT error                              { fprintf(stderr, "Error type B at line %d: wrong struct definition\n", @2.first_line);} */
    ;
OptTag:                                         { $$ = NULL;}
    | ID                                        { $$ = createNode(@$.first_line, "OptTag", 1, $1);}
    ;
Tag: ID                                         { $$ = createNode(@$.first_line, "Tag", 1, $1);}
    ;

/* Declarators */
VarDec: ID                                      { $$ = createNode(@$.first_line, "VarDec", 1, $1);}
    | VarDec LB INT RB                          { $$ = createNode(@$.first_line, "VarDec", 4, $1, $2, $3, $4);}
    /* | VarDec LB error RB                        { fprintf(stderr, "Error type B at line %d: The number between \"[\" and \"]\" must be integer\n", @3.first_line);} */
    ;
FunDec: ID LP VarList RP                        { $$ = createNode(@$.first_line, "FunDec", 4, $1, $2, $3, $4);}
    | ID LP RP                                  { $$ = createNode(@$.first_line, "FunDec", 3, $1, $2, $3);}
    /* | ID LP error RP                            { fprintf(stderr, "Error type B at line %d: wrong arguments list between \"(\" and \")\"\n", @3.first_line);}
    | ID error RP                               { fprintf(stderr, "Error type B at line %d: Missing \"(\"\n", @2.first_line);}
    | ID LP VarList error                       { fprintf(stderr, "Error type B at line %d: Missing \")\"\n", @1.first_line);}
    | ID LP error                               { fprintf(stderr, "Error type B at line %d: Missing \")\"\n", @1.first_line);} */
    ;
VarList: ParamDec COMMA VarList                 { $$ = createNode(@$.first_line, "VarList", 3, $1, $2, $3);}
    | ParamDec                                  { $$ = createNode(@$.first_line, "VarList", 1, $1);}
    ;
ParamDec: Specifier VarDec                      { $$ = createNode(@$.first_line, "ParamDec", 2, $1, $2);}
    ;

/* Statements */
CompSt: LC DefList StmtList RC                  { $$ = createNode(@$.first_line, "CompSt", 4, $1, $2, $3, $4);}
    /* | LC error RC                               { fprintf(stderr, "Error type B at line %d: wrong statements between \"{\" and \"}\"\n", @2.first_line); } // in this case, the process in the function will not be checked
    | error RC                                  { fprintf(stderr, "Error type B at line %d: Missing \"{\"\n", @1.first_line); } // in this case, the process in the function will not be checked
    | LC DefList StmtList error                 { fprintf(stderr, "Error type B at line %d: Missing \"}\"\n", @1.first_line); } */
    ;
StmtList:                                       { $$ = NULL;}
    | Stmt StmtList                             { $$ = createNode(@$.first_line, "StmtList", 2, $1, $2);}
    ;
Stmt: Exp SEMI                                  { $$ = createNode(@$.first_line, "Stmt", 2, $1, $2);}
    | CompSt                                    { $$ = createNode(@$.first_line, "Stmt", 1, $1);}
    | RETURN Exp SEMI                           { $$ = createNode(@$.first_line, "Stmt", 3, $1, $2, $3);}
    | IF LP Exp RP Stmt %prec LOWER_THAN_ELSE   { $$ = createNode(@$.first_line, "Stmt", 5, $1, $2, $3, $4, $5);}
    | IF LP Exp RP Stmt ELSE Stmt               { $$ = createNode(@$.first_line, "Stmt", 7, $1, $2, $3, $4, $5, $6, $7);}
    | WHILE LP Exp RP Stmt                      { $$ = createNode(@$.first_line, "Stmt", 5, $1, $2, $3, $4, $5);}
    /* | WHILE LP Exp error Stmt                   { fprintf(stderr, "Error type B at line %d: Missing \")\"\n", @4.first_line);}
    | WHILE LP error RP Stmt                    { fprintf(stderr, "Error type B at line %d: wrong expression between \"(\" and \")\"\n", @4.first_line);}
    | error SEMI                                { fprintf(stderr, "Error type B at line %d: stmt error\n", @1.first_line);} // error2
    | Exp error                                 { fprintf(stderr, "Error type B at line %d: Missing \";\"\n", @1.first_line); syn_newline();}
    | RETURN Exp error                          { fprintf(stderr, "Error type B at line %d: Missing \";\"\n", @1.first_line); syn_newline();} */
    ;

/* Local Definitions */
DefList:                                        { $$ = NULL;}
    | Def DefList                               { $$ = createNode(@$.first_line, "DefList", 2, $1, $2);}
    ;
Def: Specifier DecList SEMI                     { $$ = createNode(@$.first_line, "Def", 3, $1, $2, $3);}
    /* | Specifier DecList error                   { fprintf(stderr, "Error type B at line %d: Missing \";\"\n", @1.first_line); syn_newline();}
    | Specifier error SEMI                      { fprintf(stderr, "Error type B at line %d: No definition\n", @2.first_line);}
    | error SEMI                                { fprintf(stderr, "Error type B at line %d: def error\n", @1.first_line);} // error2 */
    ;
DecList: Dec                                    { $$ = createNode(@$.first_line, "DecList", 1, $1);}
    | Dec COMMA DecList                         { $$ = createNode(@$.first_line, "DecList", 3, $1, $2, $3);}
    /* | error                                     { fprintf(stderr, "Error type B at line %d: Syntax error\n", @1.first_line); syn_newline();} */
    ;
Dec: VarDec                                     { $$ = createNode(@$.first_line, "Dec", 1, $1);}
    | VarDec ASSIGNOP Exp                       { $$ = createNode(@$.first_line, "Dec", 3, $1, $2, $3);}
    ;

/* Expressions */
Exp: Exp ASSIGNOP Exp                           { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp RELOP Exp                             { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp PLUS Exp                              { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp MINUS Exp                             { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp STAR Exp                              { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp DIV Exp                               { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | LP Exp RP                                 { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | MINUS Exp                                 { $$ = createNode(@$.first_line, "Exp", 2, $1, $2);}
    | NOT Exp                                   { $$ = createNode(@$.first_line, "Exp", 2, $1, $2);}
    | ID LP Args RP                             { $$ = createNode(@$.first_line, "Exp", 4, $1, $2, $3, $4);}
    | ID LP RP                                  { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp LB Exp RB                             { $$ = createNode(@$.first_line, "Exp", 4, $1, $2, $3, $4);}
    | Exp DOT ID                                { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | ID                                        { $$ = createNode(@$.first_line, "Exp", 1, $1);}
    | INT                                       { $$ = createNode(@$.first_line, "Exp", 1, $1);}
    | FLOAT                                     { $$ = createNode(@$.first_line, "Exp", 1, $1);}
    | Exp AND Exp                               { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    | Exp OR Exp                                { $$ = createNode(@$.first_line, "Exp", 3, $1, $2, $3);}
    /* | LP error RP                               { fprintf(stderr, "Error type B at line %d: wrong expression between \"(\" and \")\"\n", @2.first_line); } //error
    | ID LP error RP                            { fprintf(stderr, "Error type B at line %d: wrong arguments list between \"(\" and \")\"\n", @3.first_line); } //error
    | Exp LB error RB                           { fprintf(stderr, "Error type B at line %d: wrong expression between \"[\" and \"]\"\n", @3.first_line);}
    | Exp error Exp                             { fprintf(stderr, "Error type B at line %d: undefined operation\n", @2.first_line);} */
    ;
Args: Exp COMMA Args                            { $$ = createNode(@$.first_line, "Args", 3, $1, $2, $3);}
    | Exp                                       { $$ = createNode(@$.first_line, "Args", 1, $1);}
    ;

%%
