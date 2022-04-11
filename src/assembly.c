#include "assembly.h"

extern char* insts[];
extern InstType* instType[];
extern int inst_num;
FILE* fp = NULL;
const char* names[] = {
	"$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", 
    "$t0",   "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", //8 -15
	"$s0",   "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", //16-23
	"$t8",   "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};

reg_list* head;
reg_list* tail;
int regs[32];
int val_offset[4096];
int val_reg[4096];
int offset = 0;
void init(){
    head = malloc(sizeof(reg_list));
    tail = head;
    reg_list* tem = head;
    for(int i = 8; i <= 23; i++){
       reg_list* new_reg = malloc(sizeof(reg_list));
       new_reg->id = i;
       new_reg->next = NULL;
       tem->next = new_reg;
       tem = new_reg;
       tail = new_reg;
    }
    memset(val_offset, 0, sizeof(val_offset));
}


void print_mips(char* filename){
    init();
    fp = fopen(filename, "w");
    assert(fp);
    fprintf(fp, ".data\n");
    fprintf(fp, "_promp: .asciiz \"Enter an integer:\".");
    fprintf(fp, "_ret: .asciiz \"\\n\"\n");
    fprintf(fp, ".globl main\n");
    fprintf(fp, ".text\n");

    fprintf(fp, "read:\n");
    fprintf(fp, "   li $v0, 4\n");
    fprintf(fp, "   la $a0, _prompt\n");
    fprintf(fp, "   syscall\n");
    fprintf(fp, "   li $v0, 5\n");  //read int
    fprintf(fp, "   syscall\n");
    fprintf(fp, "   jr $ra\n\n");

    fprintf(fp, "write:\n");
    fprintf(fp, "   li $v0, 1\n"); //print int
    fprintf(fp, "   syscall\n");
    fprintf(fp, "   li $v0, 4\n"); //print string
    fprintf(fp, "   la $a0, _ret\n");
    fprintf(fp, "   syscall\n");
    fprintf(fp, "   move $v0, $0\n");
    fprintf(fp, "   jr $ra\n");
    
    for(int i = 0; i < inst_num; i++){
        // printf("%d %d\n", i, inst_num);
        // fprintf(fp, "%s\n", insts[i]);
        gen_inst(i);
    }
}



int get_reg(Info info){
    if(val_reg[info.id] != 0) {
        // printf("%d\n", val_reg[info.id]);
        return val_reg[info.id];
    }
    reg_list* selected = head->next;
    assert(selected);
    int ret = selected->id;
    offset -= 4;
    val_offset[selected->var_id] = offset;
    fprintf(fp, "   sw %s, %d($sp)\n", names[selected->id], offset);
    val_reg[selected->var_id] = 0;
    if(val_offset[info.id] != 0)
        fprintf(fp, "   lw %s, %d($sp)\n", names[selected->id], val_offset[info.id]);
    selected->var_id = info.id;
    val_reg[info.id] = selected->id;
    tail->next = selected;
    head->next = selected->next;
    selected->next = NULL;
    tail = selected;
    // printf("%d\n", ret);
    return ret;
}


void gen_inst(int id){
    int dst_reg, src1_reg, src2_reg;
    char buf[128];
    switch(instType[id]->type){
        case TP_LABEL: 
            fprintf(fp, "label%d\n", instType[id]->dst.value);
            break;
        case TP_FUNCT:
            fprintf(fp, "%s:\n", instType[id]->name);
            fprintf(fp, "   subu $sp, $sp\n"); //push sp
            fprintf(fp, "   sw $fp, 0($sp)\n");
            fprintf(fp, "   move $fp, $sp\n");
            fprintf(fp, "   subu $sp, $sp, %d\n", STACK_SIZE);
            offset = 0;
            // funcSave();
            break;
        case TP_ASSIGN:
            dst_reg = get_reg(instType[id]->dst);
            if(instType[id]->src1.is_id){
                src1_reg = get_reg(instType[id]->src1);
                fprintf(fp, "   move %s, %s\n", names[dst_reg], names[src1_reg]);
            }
            else{
                fprintf(fp, "   li %s, %d\n", names[dst_reg], instType[id]->src1.value);
            }
            break;
        case TP_ADD:
            dst_reg = get_reg(instType[id]->dst);
            if(instType[id]->src1.is_id && instType[id]->src2.is_id){
                src1_reg = get_reg(instType[id]->src1);
                src2_reg = get_reg(instType[id]->src2);
                fprintf(fp, "   add %s, %s, %s\n", names[dst_reg], names[src1_reg], names[src2_reg]);
            }
            else if(instType[id]->src1.is_id && ! instType[id]->src2.is_id){
                src1_reg = get_reg(instType[id]->src1);
                fprintf(fp, "   addi %s, %s, %d\n", names[dst_reg], names[src1_reg], instType[id]->src2.value);
            }
            else if(instType[id]->src2.is_id){
                src2_reg = get_reg(instType[id]->src2);
                fprintf(fp, "   addi %s, %s, %d\n", names[dst_reg], names[src2_reg], instType[id]->src1.value);
            }
            else{
                fprintf(fp, "   li %s, %d\n", names[dst_reg], instType[id]->src1.value + instType[id]->src2.value);
            }
            break;
        case TP_SUB:
            dst_reg = get_reg(instType[id]->dst);
            if(instType[id]->src1.is_id && instType[id]->src2.is_id){
                src1_reg = get_reg(instType[id]->src1);
                src2_reg = get_reg(instType[id]->src2);
                fprintf(fp, "   sub %s, %s, %s\n", names[dst_reg], names[src1_reg], names[src2_reg]);
            }
            else if(instType[id]->src1.is_id && ! instType[id]->src2.is_id){
                src1_reg = get_reg(instType[id]->src1);
                fprintf(fp, "   sub %s, %s, %d\n", names[dst_reg], names[src1_reg], instType[id]->src2.value);
            }
            else{
                fprintf(fp, "   li %s, %d\n", names[dst_reg], instType[id]->src1.value - instType[id]->src2.value);
            }
            break;
        case TP_MUL:
            dst_reg = get_reg(instType[id]->dst);
            if(instType[id]->src1.is_id && instType[id]->src2.is_id){
                src1_reg = get_reg(instType[id]->src1);
                src2_reg = get_reg(instType[id]->src2);
                fprintf(fp, "   mul %s, %s, %s\n", names[dst_reg], names[src1_reg], names[src2_reg]);
            }
            else if(instType[id]->src1.is_id && ! instType[id]->src2.is_id){
                src1_reg = get_reg(instType[id]->src1);
                fprintf(fp, "   mul %s, %s, %d\n", names[dst_reg], names[src1_reg], instType[id]->src2.value);
            }
            else if(instType[id]->src2.is_id){
                src2_reg = get_reg(instType[id]->src2);
                fprintf(fp, "   mul %s, %s, %d\n", names[dst_reg], names[src2_reg], instType[id]->src1.value);
            }
            else{
                fprintf(fp, "   li %s, %d\n", names[dst_reg], instType[id]->src1.value * instType[id]->src2.value);
            }
            break;
        case TP_DIV:
            dst_reg = get_reg(instType[id]->dst);
            if(instType[id]->src1.is_id && instType[id]->src2.is_id){
                src1_reg = get_reg(instType[id]->src1);
                src2_reg = get_reg(instType[id]->src2);
                fprintf(fp, "   div %s, %s, %s\n", names[dst_reg], names[src1_reg], names[src2_reg]);
            }
            else if(instType[id]->src1.is_id && ! instType[id]->src2.is_id){
                src1_reg = get_reg(instType[id]->src1);
                fprintf(fp, "   div %s, %s, %d\n", names[dst_reg], names[src1_reg], instType[id]->src2.value);
            }
            else{
                fprintf(fp, "   li %s, %d\n", names[dst_reg], instType[id]->src1.value / instType[id]->src2.value);
            }
            break;
        case TP_ADDR:
            dst_reg = get_reg(instType[id]->dst);
            src1_reg = get_reg(instType[id]->src1);
            fprintf(fp, "   li %s, 0(%s)\n", names[dst_reg], names[src1_reg]);
            break;
        case TP_STAR:
            dst_reg = get_reg(instType[id]->dst);
            if(instType[id]->src1.is_id){
                src1_reg = get_reg(instType[id]->src1);
                fprintf(fp, "   sw %s, 0(%s)\n", names[src1_reg], names[dst_reg]);
            }
            else{
                fprintf(fp, "   sw %d, 0(%s)\n", instType[id]->src1.value, names[dst_reg]);
            }
            break;
        case TP_GOTO:
            fprintf(fp, "   j label%d\n", instType[id]->dst.id);
            break;
        case TP_IF:
            if(instType[id]->src1.is_id)
                src1_reg = get_reg(instType[id]->src1);
            if(instType[id]->src2.is_id)
                src2_reg = get_reg(instType[id]->src2);
            if(strcmp(instType[id]->op, "==") == 0){
                fprintf(fp, "   beq ");
            } else if(strcmp(instType[id]->op, "!=") == 0){
                fprintf(fp, "   bne ");
            } else if(strcmp(instType[id]->op, ">") == 0){
                fprintf(fp, "   bgt ");
            } else if(strcmp(instType[id]->op, "<") == 0){
                fprintf(fp, "   blt ");
            } else if(strcmp(instType[id]->op, ">=") == 0){
                fprintf(fp, "   bge ");
            } else if(strcmp(instType[id]->op, "<=") == 0){
                fprintf(fp, "   ble ");
            }

            if(instType[id]->src1.is_id){
                // src1_reg = get_reg(instType[id]->src1);
                fprintf(fp, "%s, ", names[src1_reg]);
            }
            else{
                fprintf(fp, "%d, ", instType[id]->src1.value);
            }
            if(instType[id]->src2.is_id){
                // src2_reg = get_reg(instType[id]->src2);
                fprintf(fp, "%s, label%d\n", names[src2_reg], instType[id]->dst.value);
            }
            else{
                fprintf(fp, "%d, label%d\n", instType[id]->src2.value, instType[id]->dst.value);
            }
            break;
        case TP_RETURN:
            if(instType[id]->dst.is_id){
                dst_reg = get_reg(instType[id]->dst);
                fprintf(fp, "   move $v0, %s\n", names[dst_reg]);
            }
            else{
                fprintf(fp, "   move $v0, %d\n", instType[id]->dst.value);
            }
            fprintf(fp, "   addi $sp, $sp, %d\n", STACK_SIZE);
            fprintf(fp, "   lw $fp, 0($sp)\n");
            fprintf(fp, "   addi $sp, $sp, 4\n");
            fprintf(fp, "   jr $ra\n\n");
            break;
        case TP_ARG:
            break;
        case TP_CALL:
            break;
        case TP_PARAM:
            break;
        case TP_READ:
            fprintf(fp, "   subu $sp, $sp, 4\n");
            fprintf(fp, "   sw $ra, 0($sp)\n");
            fprintf(fp, "   jal read\n");
            fprintf(fp, "   lw $ra, 0($sp)\n");
            fprintf(fp, "   addi $sp, $sp, 4\n");
            break;
        case TP_WRITE:
            fprintf(fp, "   subu $sp, $sp, 4\n");
            fprintf(fp, "   sw $ra, 0($sp)\n");
            fprintf(fp, "   jal write\n");
            fprintf(fp, "   lw $ra, 0($sp)\n");
            fprintf(fp, "   addi $sp, $sp, 4\n");
            break;
    }
    
}