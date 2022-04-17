#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <intercode.h>
// #define MAX_INST_WIDTH 1024
#define STACK_SIZE 128
void gen_inst(int id);
void gen_riscv(char* filename);
int get_reg(Info info);

typedef struct regList{
  int id;
  int var_id;
  struct regList* next;
}reg_list;

typedef struct regState{
  int var_id;
  int is_used;
}regState_t;

typedef struct varInfo{
  int valid;
  int reg_id;
  int stack_offset;
}varInfo_t;

typedef struct funcInfo{
  int varInfo_idx;
  int stack_offset;
  int stack_instIdx;    // instIdx for last function
  int regsave_instIdx;
}funcInfo_t;

#define MAX_INST_LEN 32
#define MAX_RV_INST 4096
#define MAX_VAR_NUM 4096

#define FUNC_STACK_DEPTH 64

enum {NORMAL_INST = 1, NO_INDENT, ALLOC_STACK, SAVE_REG, RECOVER_REG, LDST_INST};

typedef struct rvInst{
  char str[MAX_INST_LEN];
  int type;
  int val_num;
  int val1;
  int val2;
}rvInst_t;

#define add_inst(_type, _val_num, _val1, _val2, ...) \
  sprintf(rvInsts[total_rvInst].str,  __VA_ARGS__); \
  rvInsts[total_rvInst].type = _type; \
  rvInsts[total_rvInst].val_num = _val_num; \
  rvInsts[total_rvInst].val1 = _val1; \
  rvInsts[total_rvInst].val2 = _val2;

#define add_normal_inst(...) \
  do{ \
    add_inst(NORMAL_INST, 0, -1, -1, __VA_ARGS__); \
    total_rvInst ++; \
  } while(0)

#define add_noindent_inst(...) \
  do{ \
    add_inst(NO_INDENT, 0, -1, -1, __VA_ARGS__); \
    total_rvInst ++; \
  } while(0)

#define add_stack_inst(val_num, ...) \
  do{ \
    add_inst(ALLOC_STACK, val_num, -1, -1, __VA_ARGS__); \
    stack_instIdx = total_rvInst; \
    stack_inst[stack_num ++] = total_rvInst; \
    total_rvInst ++; \
  } while(0)

#define add_stack_pop_inst(...) \
  do{ \
    add_inst(ALLOC_STACK, 1, -1, -1, __VA_ARGS__); \
    stack_inst[stack_num ++] = total_rvInst; \
    total_rvInst ++; \
  } while(0)

#define add_savereg_inst(...) \
  do{ \
    add_inst(SAVE_REG, 0, -1, -1, " "); \
    regsave_instIdx = total_rvInst; \
    rvInsts[regsave_instIdx].val1 = 0; \
    total_rvInst ++; \
  } while(0)

#define update_stack_inst() \
  do{ \
    for(int i = 0; i < stack_num; i++) \
      rvInsts[stack_inst[i]].val1 = offset + stackVar_sz; \
  } while(0)

#define add_recoverreg_inst(...) \
  do{ \
    add_inst(RECOVER_REG, 0, -1, -1, " "); \
    int save_num = 0; \
    rvInsts[total_rvInst].val1 = 0; \
    for(int i = 0; i < 32; i++){ \
      if(regState[i].is_used == 1){ \
        save_num ++; \
        rvInsts[total_rvInst].val1 = rvInsts[total_rvInst].val1 | (1 << i); \
      } \
    } \
    if(save_num * 8 > stackVar_sz){ \
      rvInsts[regsave_instIdx].val1 = rvInsts[total_rvInst].val1; \
      stackVar_sz = save_num * 8; \
      update_ldst_offset(regsave_instIdx + 1, total_rvInst, stackVar_sz); \
    } \
    total_rvInst ++; \
  } while(0)

#define add_ldst_inst(offset, ...) \
  do{ \
    add_inst(LDST_INST, 1, -1, -1, __VA_ARGS__); \
    rvInsts[total_rvInst].val1 = offset; \
    total_rvInst ++; \
  } while(0)

#define update_ldst_offset(start, end, increment) \
  do{ \
    for(int i = start; i < end; i++){ \
      if(rvInsts[i].type == LDST_INST){ \
        rvInsts[i].val1 += increment; \
      } \
    } \
  } while(0)


#define IN_STACK 0
#define IN_REG 1
