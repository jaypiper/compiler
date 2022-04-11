#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "Node.h"
#include <assert.h>
#include <stdio.h>
// #define DEBUG

#define HASH_SZ 0x3fff
#define STACK_SZ 100
#define TYPE_INT 1
#define TYPE_FLOAT 2
#define IS_STRUCTURE 1
#define NOT_STRUCTURE 0

enum Type_ {BASIC=1, ARRAY, FUNC, STRUCT_VAR, STRUCTURE};

typedef struct STACK{
    struct SYMBOL* s[STACK_SZ];
    int sz;
}Stack;

typedef struct SYMBOL{ //符号名和类型，存放在hash表中指针
    char* name;
    struct SYMBOL_TYPE* type;
    int depth;
    struct SYMBOL* slot_next;
    struct SYMBOL* slot_pre;
    struct SYMBOL* depth_next;
}Symbol;

typedef struct PARA_LIST{ //structure内部定义(;)和function参数列表(,)
    struct SYMBOL* para;
    struct PARA_LIST* next;
    struct PARA_LIST* tail;
}Plist;

typedef struct SYMBOL_TYPE{
    enum Type_ type;
    // int depth;
    union{
        /* Type-Basic */
        struct{
            int basic_type;
        };
        /* Type-Array */
        struct{
            int sz;  //无用
            struct SYMBOL_TYPE* next;
            // struct SYMBOL_TYPE* tail; //并非指向最后一个节点，也是指向最后一个维度，只记录在头节点中，中间节点无效, tail->next = basic_type
        };
        /* Type-Func */
        struct{
            struct SYMBOL_TYPE* ret_type;
            struct PARA_LIST* func_para;
        };
        /*struct_var*/
        // struct{
        //     struct SYMBOL* struct_type;
        // };
        /*structure and struct_var*/
        struct{
            struct PARA_LIST* struct_para;
        };
    };
}Stype;

int semantic_check(Node* root);

#endif