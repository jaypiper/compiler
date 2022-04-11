#include "Node.h"

int syn_correct = 1;
int pre_lineno = -1;
int lex_num = 0;

Node* createTokenNode(int lineno, char* name, char* text, void* val){
    // printf("Token: %d %s %s\n", lineno, name, text);
    Node* p = (Node*)malloc(sizeof(Node));
    assert(p);
    p->lineno = lineno;
    p->name = (char*)malloc(sizeof(char) * (strlen(name)+1));
    p->text = (char*)malloc(sizeof(char) * (strlen(text) + 1));
    strncpy(p->name, name, strlen(name) + 1);
    strncpy(p->text, text, strlen(text) + 1);
    p->child_num = 0;
    p->node_type = is_token;
    p->ival = 0; p->fval = 0;
    // p->true_list = p->false_list = NULL;
    if(val){
        if(strcmp(name, "INT") == 0) p->ival = *((int*)val);
        else if(strcmp(name, "FLOAT") == 0) p->fval = *((float*) val);
        else assert(0);
    }
    // printf("---\n");
    return p;
}

Node* createNode(int lineno, char* name, int num, ...){
    // printf("Node: %d %s\n", lineno, name);
    if(lineno != pre_lineno){
        pre_lineno = lineno;
        lex_num = 1;
    }
    else lex_num ++;
    Node* parent = (Node*)malloc(sizeof(Node));
    assert(parent);
    parent->lineno = lineno;
    parent->name = (char*)malloc(sizeof(char) * (strlen(name)+1));
    strncpy(parent->name, name, strlen(name) + 1);
    parent->child_num = 0;
    parent->node_type = not_token;
    // parent->true_list = parent->false_list = NULL;
    va_list valist;
    va_start(valist, num);
    for(int i = 0; i < num; i++){
        // printf("idx: %d\n", i);
        Node* next = va_arg(valist, Node*);
        // if(!next) continue;
        parent->child[parent->child_num ++] = next;
    }
    va_end(valist);
    // printf("---\n");
    return parent;

}

int more_info(char* name){
    if(strcmp(name, "ID") == 0 || strcmp(name, "TYPE") == 0) return 1;
    if(strcmp(name, "INT") == 0) return 2;
    if(strcmp(name, "FLOAT") == 0) return 3;
    return 0;
}

void preorder_traversal(Node* root, int depth){
    if(!syn_correct) return;
    if(!root) return;
    for(int i = 0; i < depth; i++) printf(" ");
    if(root->node_type == is_token) {
        switch(more_info(root->name)){
            case 0: printf("%s\n", root->name); break;
            case 1: printf("%s: %s\n", root->name, root->text); break;
            case 2: printf("%s: %d\n", root->name, root->ival); break;
            case 3: printf("%s: %f\n", root->name, root->fval); break;
            default: assert(0);
        }
    }
    else printf("%s (%d)\n", root->name, root->lineno);
    for(int i = 0; i < root->child_num; i++){
        preorder_traversal(root->child[i], depth + 1);
    }

}
