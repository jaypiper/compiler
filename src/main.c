#include <stdio.h>
#include "Node.h"
#include "syntax.h"
#include "semantic.h"
#include "assembly.h"
extern FILE* yyin;
extern Node* root;
extern int syn_correct;
extern int yyparse (void);
void gen_intercode(Node* root);

int main(int argc, char** argv){
    if(argc > 1){
        if(!(yyin = fopen(argv[1], "r"))){
            perror(argv[1]);
            return 1;
        }

    }
    yyparse();
    // preorder_traversal(root, 0);
    // semantic_check(root);
    assert(syn_correct);
    gen_intercode(root);
    print_mips(argv[2]);
    return 0;
}
