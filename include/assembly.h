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

enum {NORMAL_INST = 1, NO_INDENT, ALLOC_STACK, SAVE_REG, RECOVER_REG};

typedef struct rvInst{
  char str[MAX_INST_LEN];
  int type;
  int val_num;
  int val1;
  int val2;
}rvInst_t;

#define push_funcInfo() \
  funcInfo[stack_idx].varInfo_idx = varInfo_idx; \
  funcInfo[stack_idx].stack_offset = offset; \
  funcInfo[stack_idx].stack_instIdx = stack_instIdx; \
  funcInfo[stack_idx].regsave_instIdx = regsave_instIdx; \
  offset = 0; \
  stack_idx ++;

#define pop_funcInfo() \
  stack_idx --; \
  varInfo_idx = funcInfo[stack_idx].varInfo_idx; \
  offset = funcInfo[stack_idx].stack_offset; \
  stack_instIdx = funcInfo[stack_idx].stack_instIdx; \
  regsave_instIdx = funcInfo[stack_idx].regsave_instIdx;

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
    total_rvInst ++; \
  } while(0)

#define add_savereg_inst(...) \
  do{ \
    add_inst(SAVE_REG, 0, -1, -1, " "); \
    for(int i = 31; i >= 0; i--){ \
      rvInsts[total_rvInst].val1 = (rvInsts[total_rvInst].val1 << 1) | (regState[i].is_used == 1); \
      regState[i].is_used = 0; \
      offset += 8; \
    } \
    regsave_instIdx = total_rvInst; \
    total_rvInst ++; \
  } while(0)

#define update_stack_inst() \
  rvInsts[stack_instIdx].val1 = offset;

#define add_recoverreg_inst(...) \
  do{ \
    add_inst(RECOVER_REG, 0, -1, -1, " "); \
    rvInsts[total_rvInst].val1 = rvInsts[regsave_instIdx].val1; \
    uint32_t bitmap = rvInsts[total_rvInst].val1; \
    for(int i = 0; i < 32; i++){ \
      if(bitmap & 1) regState[i].is_used = 1; \
      else regState[i].is_used = 0; \
      bitmap >>= 1; \
    } \
  } while(0)

#define push_ra() \
  add_normal_inst("sd ra, 0(sp)"); \
  offset += 8;

#define pop_ra() \
  add_normal_inst("ld ra, 0(sp)");

#define IN_STACK 0
#define IN_REG 1
