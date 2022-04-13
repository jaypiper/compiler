#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <intercode.h>
// #define MAX_INST_WIDTH 1024
#define STACK_SIZE 128
void gen_inst(int id);
void print_mips(char* filename);
int get_reg(Info info);
typedef struct regList{
    int id;
    int var_id;
    struct regList* next;
}reg_list;