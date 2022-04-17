#include <common.h>
#include <assembly.h>

extern char* insts[];
extern InstType* instType[];
extern int inst_num;
FILE* fp = NULL;

rvInst_t rvInsts[MAX_RV_INST];
int total_rvInst = 0;

varInfo_t varInfo[MAX_VAR_NUM];
int varInfo_idx = 0;
// stacks for function call
funcInfo_t funcInfo[FUNC_STACK_DEPTH];
int stack_idx = 0;

const char* names[] = {
	"$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

regState_t regState[32];
int sreg[] = {8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27}; // s0 - s11
int sregIdx = 0;

int regs[32];
int offset = 0;
static int dst_reg, src1_reg, src2_reg;
static char buf[128];

static uint8_t used_regs[32];
static int used_regs_num = 0;
static int stack_instIdx = 0;
static int regsave_instIdx = 0;
static int areg[] = {10, 11, 12, 13, 14, 15, 16, 17};
static int argnum = 0;
static int paramnum = 0;

void init_riscv(){
	add_noindent_inst(".section .text");
	add_noindent_inst(".globl _start");

	add_noindent_inst("_start:");
	add_normal_inst("mv s0, zero");
	add_normal_inst("li sp, 0x81000000");
	add_normal_inst("jal main");
	add_normal_inst(".word 0x6b");

	add_noindent_inst("syscall:");
	add_normal_inst("mv a7, a0");
	add_normal_inst("mv a0, a1");
	add_normal_inst("mv a1, a2");
	add_normal_inst("mv a2, a3");
	add_normal_inst("ecall");
	add_normal_inst("ret");
}

void init(){
	for(int i = 0; i < 32; i++) regState[i].var_id = -1;
	sregIdx = 0;
	memset(funcInfo, 0, sizeof(funcInfo));
	stack_idx = 0;
	memset(varInfo, 0, sizeof(varInfo));
	varInfo_idx = 0;
  // memset(val_offset, 0, sizeof(val_offset));
	memset(rvInsts, 0, sizeof(rvInsts));
	init_riscv();
}

void set_varinfo(int is_reg, int var_id, int info){
	if(is_reg) varInfo[var_id].reg_id = info;
	else{
		varInfo[var_id].reg_id = 0;
		varInfo[var_id].stack_offset = info;
	}
	varInfo[var_id].valid = 1;
}

void push_stack(int reg_id){ // store the variable bound to reg_id, if exists, into stack
	if(reg_id <= 0) return;
	int var_id = regState[reg_id].var_id;
	if(var_id < 0) return;
	// update varInfo for save_var_id
	set_varinfo(IN_STACK, var_id, offset);
	add_ldst_inst(offset, "sw %s, %%d($sp)", names[reg_id]);
	offset += 8;
}

void fetch_var_from_stack(int var_id, int reg_id){ // move var to reg
	assert(var_id >= 0 && reg_id >=0);
	int offset = varInfo[var_id].stack_offset;
	add_ldst_inst(offset, "lw %s, %%d(sp)", names[reg_id]);
}

int get_reg(Info info){
	// variable is already in a reg
  if(varInfo[info.id].valid && varInfo[info.id].reg_id != 0) {
    return varInfo[info.id].reg_id;
  }
	int select_sreg = sreg[sregIdx];
	regState[select_sreg].is_used = 1;
	sregIdx = sregIdx == 11 ? 0 : sregIdx + 1;
	// save select_sreg into stack
	push_stack(select_sreg);
	// move var to reg
	if(varInfo[info.id].valid) fetch_var_from_stack(info.id, select_sreg);
	set_varinfo(IN_REG, info.id, select_sreg);

	return select_sreg;
}

void inst_label(InstType* inst){
	add_noindent_inst("label%d:", inst->dst.id);
}

void inst_func(InstType* inst){
	for(int i = 0; i < MAX_VAR_NUM; i++) varInfo[i].valid = 0;
	for(int i = 0; i < 32; i++) regState[i].is_used = 0;
	push_funcInfo();
	add_noindent_inst("%s:", inst->name);
	add_stack_inst(1, "addi sp, sp, -%%d");
	push_ra();
	add_savereg_inst();
}

void inst_assign(InstType* inst){
	dst_reg = get_reg(inst->dst);
  if(inst->src1.id >= 0){
    src1_reg = get_reg(inst->src1);
		add_normal_inst("mv %s, %s", names[dst_reg], names[src1_reg]);
  }else{
		add_normal_inst("li %s, %d", names[dst_reg], inst->src1.value);
  }
}

void inst_add(InstType* inst){
	dst_reg = get_reg(inst->dst);
  if(inst->src1.id >= 0 && inst->src2.id >= 0){
    src1_reg = get_reg(inst->src1);
    src2_reg = get_reg(inst->src2);
		add_normal_inst("add %s, %s, %s",  names[dst_reg], names[src1_reg], names[src2_reg]);
  } else if(inst->src1.id >= 0){
    src1_reg = get_reg(inst->src1);
		add_normal_inst("addi %s, %s, %d", names[dst_reg], names[src1_reg], inst->src2.value);
  } else if(inst->src2.id >= 0){
    src2_reg = get_reg(inst->src2);
		add_normal_inst("addi %s, %s, %d", names[dst_reg], names[src2_reg], inst->src1.value);
  } else{
		add_normal_inst("li %s, %d", names[dst_reg], inst->src1.value + inst->src2.value);
  }
}

void inst_sub(InstType* inst){
	dst_reg = get_reg(inst->dst);
  if(inst->src1.id >= 0 && inst->src2.id >= 0){
    src1_reg = get_reg(inst->src1);
    src2_reg = get_reg(inst->src2);
		add_normal_inst("sub %s, %s, %s",  names[dst_reg], names[src1_reg], names[src2_reg]);
  } else if(inst->src1.id >= 0){
    src1_reg = get_reg(inst->src1);
		add_normal_inst("addi %s, %s, %d",  names[dst_reg], names[src1_reg], -inst->src2.value);
  } else if(inst->src2.id >= 0){
    src2_reg = get_reg(inst->src2);
		add_normal_inst("sub %s, zero, %s", names[src2_reg], names[src2_reg]);
		add_normal_inst("addi %s, %s, %d", names[dst_reg], names[src2_reg], inst->src1.value);
  } else{
		add_normal_inst("li %s, %d", names[dst_reg], inst->src1.value - inst->src2.value);
  }
}

void inst_mul(InstType* inst){
	dst_reg = get_reg(inst->dst);
  if(inst->src1.id >= 0 && inst->src2.id >= 0){
    src1_reg = get_reg(inst->src1);
    src2_reg = get_reg(inst->src2);
		add_normal_inst("mul %s, %s, %s", names[dst_reg], names[src1_reg], names[src2_reg]);
  } else if(inst->src1.id >= 0){
    src1_reg = get_reg(inst->src1);
		add_normal_inst("li t0, %d", inst->src2.value);
		add_normal_inst("mul %s, %s, t0", names[dst_reg], names[src1_reg]);
  } else if(inst->src2.id >= 0){
    src2_reg = get_reg(inst->src2);
		add_normal_inst("li t0, %d", inst->src1.value);
		add_normal_inst("mul %s, %s, t0", names[dst_reg], names[src2_reg]);
  } else{
		add_normal_inst("li %s, %d", names[dst_reg], inst->src1.value * inst->src2.value);
  }
}

void inst_div(InstType* inst){
	dst_reg = get_reg(inst->dst);
	if(inst->src1.id >= 0 && inst->src2.id >= 0){
    src1_reg = get_reg(inst->src1);
    src2_reg = get_reg(inst->src2);
		add_normal_inst("div %s, %s, %s", names[dst_reg], names[src1_reg], names[src2_reg]);
  } else if(inst->src1.id >= 0){
    src1_reg = get_reg(inst->src1);
		add_normal_inst("li t0, %d", inst->src2.value);
		add_normal_inst("div %s, %s, t0", names[dst_reg], names[src1_reg]);
  } else if(inst->src2.id >= 0){
    src2_reg = get_reg(inst->src2);
		add_normal_inst("li t0, %d", inst->src1.value);
		add_normal_inst("div %s, t0, %s", names[dst_reg], names[src2_reg]);
  } else{
		add_normal_inst("li %s, %d", names[dst_reg], inst->src1.value / inst->src2.value);
  }
}

void inst_addr(InstType* inst){
	dst_reg = get_reg(inst->dst);
	add_normal_inst("la %s, v%d", names[dst_reg], inst->src1.id);
}

void inst_star(InstType* inst){ // TODO: lstar & rstar
// this is the implementation of lstar(store)
	dst_reg = get_reg(inst->dst);
  if(inst->src1.id >= 0){
    src1_reg = get_reg(inst->src1);
		add_normal_inst("sd %s, 0(%s)", names[src1_reg], names[dst_reg]);
  } else{
		add_normal_inst("li t0, %d", inst->src1.value);
		add_normal_inst("sd t0, 0(%s)", names[dst_reg]);
  }
}

void inst_goto(InstType* inst){
	add_normal_inst("j label%d", inst->dst.id);
}

void inst_if(InstType* inst){
	if(inst->src1.id >= 0) src1_reg = get_reg(inst->src1);
  if(inst->src2.id >= 0) src2_reg = get_reg(inst->src2);
	char tmp_str[MAX_INST_LEN];
  if(strcmp(inst->op, "==") == 0){
		strcpy(tmp_str, "beq ");
  } else if(strcmp(inst->op, "!=") == 0){
    strcpy(tmp_str, "bne ");
  } else if(strcmp(inst->op, ">") == 0){
    strcpy(tmp_str, "bgt ");
  } else if(strcmp(inst->op, "<") == 0){
    strcpy(tmp_str, "blt ");
  } else if(strcmp(inst->op, ">=") == 0){
    strcpy(tmp_str, "bge ");
  } else if(strcmp(inst->op, "<=") == 0){
    strcpy(tmp_str, "ble ");
  } else{
		assert(0);
	}

  if(inst->src1.id >= 0) sprintf(tmp_str + strlen(tmp_str), "%s, ", names[src1_reg]);
  else{
		add_normal_inst("li t0, %d", inst->src1.value);
		sprintf(tmp_str + strlen(tmp_str), "t0, ");
	}

  if(inst->src2.id >= 0){
    sprintf(tmp_str +  strlen(tmp_str), "%s, label%d", names[src2_reg], inst->dst.id);
  } else{
		add_normal_inst("li t1, %d", inst->src2.value);
    sprintf(tmp_str +  strlen(tmp_str), "t1, label%d", inst->dst.id);
  }
	add_normal_inst("%s", tmp_str);
}

static void inst_return(InstType* inst){
	if(inst->dst.id >= 0){
		dst_reg = get_reg(inst->dst);
		add_normal_inst("mv a0, %s", names[dst_reg]);
	} else{
		add_normal_inst("li a0, %d", inst->dst.value);
	}
	update_stack_inst();
	add_recoverreg_inst();
	pop_ra();
	add_normal_inst("addi sp, sp, %d", offset);
	pop_funcInfo();
	add_normal_inst("ret");
	paramnum = 0;
}

static void inst_arg(InstType* inst){
	if(inst->dst.id >= 0){
		dst_reg = get_reg(inst->dst);
		add_normal_inst("mv %s, %s", names[areg[argnum ++]], names[dst_reg]);
	}else{
		add_normal_inst("li %s, %d",  names[areg[argnum ++]], inst->dst.value);
	}
}

void inst_call(InstType* inst){
	add_normal_inst("call %s", inst->name);
	set_varinfo(IN_REG, inst->dst.id, 10);
	argnum = 0;
}

void inst_param(InstType* inst){
	varInfo[inst->dst.id].reg_id = areg[paramnum ++];
}


static void (*instFunc[])(InstType* inst) = {
	[TP_LABEL] 		= inst_label,
	[TP_FUNCT] 		= inst_func,
	[TP_ASSIGN] 	= inst_assign,
	[TP_ADD] 			= inst_add,
	[TP_SUB] 			= inst_sub,
	[TP_MUL] 			= inst_mul,
	[TP_DIV] 			= inst_div,
	[TP_ADDR] 		= inst_addr,
	[TP_STAR] 		= inst_star,
	[TP_GOTO] 		= inst_goto,
	[TP_IF] 			= inst_if,
	[TP_RETURN] 	= inst_return,
	[TP_ARG] 			= inst_arg,
	[TP_CALL] 		= inst_call,
	[TP_PARAM] 		= inst_param,
};


void gen_inst(int id){
	void (*instfunc)(InstType* inst) = instFunc[instType[id]->type];
	instfunc(instType[id]);
}

void print_insts(char* filename){
	FILE* fp = fopen(filename, "w");
	uint32_t bitmap;
	int idx;
	int prev_instnum = 0;
	for(int i = 0; i < total_rvInst; i++){
		if(rvInsts[i].type != NO_INDENT && prev_instnum) fprintf(fp, "	");
		if(rvInsts[i].type == NO_INDENT && rvInsts[i].str[strlen(rvInsts[i].str) - 1] == ':') fprintf(fp, "\n");
		switch(rvInsts[i].type){
			case NO_INDENT:
			case NORMAL_INST:
					fprintf(fp, "%s", rvInsts[i].str); break;
			case ALLOC_STACK:
					fprintf(fp, rvInsts[i].str, rvInsts[i].val1); break;
			case SAVE_REG:
					bitmap = rvInsts[i].val1;
					idx = 0;
					for(int i = 0; i < 32; i++){
						if(bitmap & 1){
							fprintf(fp, "sd %s, %d(sp)\n", names[i], idx * 8);
							idx ++;
						}
						bitmap >>= 1;
					}
					break;
			case RECOVER_REG:
					bitmap = rvInsts[i].val1;
					idx = 0;
					for(int i = 0; i < 32; i++){
						if(bitmap & 1){
							fprintf(fp, "ld %s, %d(sp)\n", names[i], idx * 8);
							idx ++;
						}
						bitmap >>= 1;
					}
					break;
			case LDST_INST:
					fprintf(fp, rvInsts[i].str, rvInsts[i].val1); break;
			default: assert(0);
		}
		if(rvInsts[i].type == NO_INDENT || rvInsts[i].type == NORMAL_INST || rvInsts[i].type == ALLOC_STACK) {
			prev_instnum = 1;
			fprintf(fp, "\n");
		} else{
			prev_instnum = idx;
		}
	}
}

void gen_riscv(char* filename){
  init();
  for(int i = 0; i < inst_num; i++){
    gen_inst(i);
  }
	print_insts(filename);
}
