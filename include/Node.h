#ifndef NODE_H
#define NODE_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
// #include "intercode.h"

#define MAX_CHILD_NUM 8
enum NODE_TYPE{is_token = 1, not_token};

typedef struct NODE{
    int lineno;
    char* name;
    char* text;
    int ival;
    float fval;
    int node_type;
    struct NODE* child[MAX_CHILD_NUM]; 
    int child_num;  
    // Ilist* true_list;
    // Ilist* false_list;
}Node;

Node* createTokenNode(int lineno, char* name, char* text, void* val);

Node* createNode(int lineno, char* name, int num, ...);

void preorder_traversal(Node* root, int depth);

#endif