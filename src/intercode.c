#include <intercode.h>
#include <common.h>

static void intercode_init();
static  void symTable_insert(Entry* _entry);
static Entry* _find(char* name);
static void program(Node* root);
static void ExtDefList(Node* root);
static void ExtDef(Node* root);
static void ExtDecList(Node* root, Vtype* _type);
static Vtype* Specifier(Node* root);
static Vtype* StructSpecifier(Node* root);
static char* Tag(Node* root);
static Entry* VarDec(Node* root, Vtype* _type);
static Entry* FunDec(Node* root, Vtype* ret_type);
static Vlist* VarList(Node* root);
static Entry* ParamDec(Node* root);
static void CompSt(Node* root , Vtype* func_ret);
static void StmtList(Node* root , Vtype* func_ret);
static Ilist* Stmt(Node* root , Vtype* func_ret);
static Vlist* DefList(Node* root, int _offset);
static Vlist* Def(Node* root, int _offset);
static Vlist* DecList(Node* root, Vtype* _type, int _offset);
static Entry* Dec(Node* root, Vtype* _type);
static Eret* Exp(Node* root);
static void Args(Node* root);
static Vtype* TYPE_token(Node* root);
static void backpatch(Ilist* tflist, int label_id);
static int gen_label();
static Ilist* gen_goto();
static Ilist* ilist_merge(Ilist* _ilist1, Ilist* _ilist2);
static Eret* addr_process(Eret* _exp);


// static Entry* symTable = NULL;
static Entry* symTable[MAX_SYM_NUM];
static int sym_num = 0;
static int depth = 0;
// static int tmp_symId = MAX_SYM_NUM;

static symStack_t symStack[MAX_STACK_DEPTH];

static int new_tmpSym(){
	return sym_num ++;
}

static void push_symStack(){
	symStack[depth].sym_num = sym_num;
	// symStack[depth].tmp_symId = tmp_symId;
	depth ++;
}

static void pop_symStack(){
	depth --;
	assert(depth >= 0);
	sym_num = symStack[depth].sym_num;
	// tmp_symId = symStack[depth].tmp_symId;
}

static Vtype* alloc_int_type(int val){
	Vtype* ret = malloc(sizeof(Vtype));
	ret->type = TP_INT;
	ret->width = 4;
	ret->basic_int = val;
	return ret;
}

InstType* instType[MAX_INST_NUM];
int inst_num = 0;
int entry_num = 0;
// int var_id = 0;
int label_num = 0;

static inline void print_oprand(FILE* fp, Info* info){
	if(info->id >= 0) fprintf(fp, "v%d",info->id);
	else fprintf(fp, "#%d", info->value);
}

void print_intercode(char* filename){
#ifdef DISPLAY_INTERCODE
	FILE* fp = fopen(filename, "w");
	for(int i = 0; i < inst_num; i++){
		switch(instType[i]->type){
			case INST_LABEL :
					fprintf(fp, "LABEL l%d :", instType[i]->dst.id);
					break;
			case INST_FUNC :
					fprintf(fp, "FUNCTION %s :", instType[i]->name);
					break;
			case INST_ASSIGN :
					fprintf(fp, "v%d := ", instType[i]->dst.id);
					print_oprand(fp, &instType[i]->src1);
					break;
			case INST_ADD :
			case INST_SUB :
			case INST_MUL :
			case INST_DIV :
					fprintf(fp, "v%d := ", instType[i]->dst.id);
					print_oprand(fp, &instType[i]->src1);
					fprintf(fp, " %s ", instType[i]->op);
					print_oprand(fp, &instType[i]->src2);
					break;
			case INST_ADDR : assert(0);
			case INST_STAR :
					fprintf(fp, "*v%d := ", instType[i]->dst.id);
					print_oprand(fp, &instType[i]->src1);
					break;
			case INST_GOTO :
					fprintf(fp, "GOTO l%d", instType[i]->dst.id);
					break;
			case INST_IF :
					fprintf(fp, "IF ");
					print_oprand(fp, &instType[i]->src1);
					fprintf(fp, " %s ", instType[i]->op);
					print_oprand(fp, &instType[i]->src2);
					fprintf(fp, " GOTO l%d", instType[i]->dst.id);
					break;
			case INST_RETURN :
					fprintf(fp, "RETURN ");
					print_oprand(fp, &instType[i]->dst);
					break;
			case INST_ARG :
					fprintf(fp, "ARG ");
					print_oprand(fp, &instType[i]->dst);
					break;
			case INST_CALL :
					fprintf(fp, "v%d = CALL %s", instType[i]->dst.id, instType[i]->name);
					break;
			case INST_PARAM :
					fprintf(fp, "PARAM v%d", instType[i]->dst.id);
					break;
			default: assert(0);
		}
		fprintf(fp, "\n");
	}
#endif
}

void gen_intercode(Node* root, char* file){
	intercode_init();
	program(root);
	print_intercode(file);
}

void intercode_init(){
	memset(symTable, 0, sizeof(symTable));
	sym_num = 0;
	memset(symStack, 0, sizeof(symStack));
	depth = 0;
	// add builtin syscall func. syscall(no, val1, val2, val3)
	Vtype* sys_type = malloc(sizeof(Vtype));
	sys_type->type = TP_FUNC;
	sys_type->ret_type = alloc_int_type(0);
	static char* param_names[] = {"no", "val1", "val2", "val3"};

	Vlist* sys_param = NULL;
	Vlist* cur_param = NULL;
	for(int i = 0; i < 4; i++){
		Vlist* param_i = malloc(sizeof(Vlist));
		if(!sys_param) sys_param = param_i;
		param_i->name = param_names[i];
		Vtype* type_i = alloc_int_type(0);
		param_i->var_type = type_i;
		if(cur_param)cur_param->next = param_i;
		param_i->pre = cur_param;
		cur_param = param_i;
	}

	sys_type->func_para = sys_param;

	Entry* sys_entry = malloc(sizeof(Entry));
	sys_entry->type = sys_type;
	sys_entry->name = "syscall";
	symTable_insert(sys_entry);
}

static void symTable_insert(Entry* _entry){
	_entry->id = sym_num;
	symTable[sym_num ++] = _entry;
}

Entry* _find(char* name){ //在符号表中查找名为name的符号，并将其返回
	for(int i = 0; i < sym_num; i++){
		if(symTable[i] && strcmp(symTable[i]->name, name) == 0)
			return symTable[i];
	}
	assert(0);
}


void program(Node* root){
	ExtDefList(root->child[0]);
}

void ExtDefList(Node* root){
	if(!root) return;
	ExtDef(root->child[0]);
	ExtDefList(root->child[1]);
}

void ExtDef(Node* root){ // 不会出现全局变量的定义
	Vtype* _type = Specifier(root->child[0]);
	if(strcmp(root->child[1]->name, "ExtDecList") == 0){ //该情况不会出现
		ExtDecList(root->child[1], _type);
	}
	else if(strcmp(root->child[1]->name, "SEMI") == 0) return;
	else if(strcmp(root->child[2]->name, "CompSt") == 0){  // Specifier FunDec CompSt
		Entry* _entry = FunDec(root->child[1], _type);
		InstType* tp = malloc(sizeof(InstType));
		instType[inst_num++] = tp;
		tp->type = INST_FUNC;
		tp->name = _entry->name;
		for(Vlist* iter = _entry->type->func_para; iter; iter = iter->next){  //加入函数参数信息
			tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;
			tp->type = INST_PARAM;
			tp->dst.id = iter->var_id;
		}
		CompSt(root->child[2], _type);
		pop_symStack();
	}
	else assert(0); //不允许出现函数声明，因此直接去掉
}

void ExtDecList(Node* root, Vtype* _type){ //不会进入该函数
	assert(0);
	VarDec(root->child[0], _type);
	if(root->child_num > 1) ExtDecList(root->child[2], _type);
}

Vtype* Specifier(Node* root){ // Done: 获取类型
	if(strcmp(root->child[0]->name, "TYPE") == 0) return TYPE_token(root->child[0]);
	else if(strcmp(root->child[0]->name, "StructSpecifier") == 0) return StructSpecifier(root->child[0]);
	assert(0);
}

Vtype* StructSpecifier(Node* root){ //Done
	if(!root->child[1] || strcmp(root->child[1]->name, "OptTag") == 0){
		char* _name = Tag(root->child[1]);
		
		Vlist* _deflist = DefList(root->child[3], 0); 

		Vtype* _type = (Vtype*)malloc(sizeof(Vtype));
		_type->type = TP_STRUCT_DEF;
		_type->_vlist = _deflist;
		if(!_deflist) _type->width = 0;
		else _type->width = _deflist->followed_sz + _deflist->var_type->width;

		if(_name){ //插入structure类型
			Entry* _struct_entry = (Entry*)malloc(sizeof(Entry));
			_struct_entry->type = _type;
			_struct_entry->name = _name;
			symTable_insert(_struct_entry);
		}
		return _type;
	}
	else if(strcmp(root->child[1]->name, "Tag") == 0) { // 需要去符号表中查找类型id, 先定义后使用，因此保证能查找到
		char* _name = Tag(root->child[1]);
		assert(_name);
		Entry* _entry = _find(_name);
		Vtype* _type = _entry->type;
		return _type;
	}
	else assert(0);
}

char* Tag(Node* root){ //Done: OptTag and Tag
	if(!root) return NULL;
	return root->child[0]->text;
}

Entry* VarDec(Node* root, Vtype* _type){ //Done
	Entry* _entry;
	if(strcmp(root->child[0]->name, "ID") == 0){
		_entry = (Entry*)malloc(sizeof(Entry));
		_entry->name = root->child[0]->text;
		_entry->type = _type;
		// _entry->var_id = var_id ++;
	}
	else if(strcmp(root->child[0]->name, "VarDec") == 0){
		Vtype* new_head = (Vtype*)malloc(sizeof(Vtype));
		new_head->type = TP_ARRAY;
		new_head->sz = root->child[2]->ival;
		new_head->next = _type;
		new_head->ele_width = _type->width;
		new_head->width = new_head->ele_width * new_head->sz;
		_entry = VarDec(root->child[0], new_head);
	}
	else assert(0);
	return _entry;
}

Entry* FunDec(Node* root, Vtype* ret_type){ // get the func variable info FunDec -> ID LP VarList RP | ID LP RP
	Vtype* _type = malloc(sizeof(Vtype));
	_type->type = INST_FUNC;
	_type->ret_type = ret_type;
	_type->func_para = NULL;
	Entry* _entry = malloc(sizeof(Entry));
	_entry->name = root->child[0]->text;
	_entry->type = _type;
	symTable_insert(_entry);

	push_symStack();  // pop in ExtDef
	if(strcmp(root->child[2]->name, "VarList") == 0) _type->func_para = VarList(root->child[2]);
	return _entry;
}

Vlist* VarList(Node* root){ //Done 函数参数定义
	Entry* _entry = ParamDec(root->child[0]);

	Vlist* _vlist = malloc(sizeof(Vlist));
	_vlist->name = _entry->name;
	_vlist->var_type = _entry->type;
	_vlist->next = _vlist->pre = NULL;
	_vlist->var_id = _entry->id;
	
	if(root->child_num > 1){
		Vlist* _surfix = VarList(root->child[2]);
		_vlist->next = _surfix;
		if(_surfix) _surfix->pre = _vlist;
	}
	return _vlist;
}

Entry* ParamDec(Node* root){ //Done 函数某一类型参数定义
	Vtype* _type = Specifier(root->child[0]);
	Entry* _entry = VarDec(root->child[1], _type);
	symTable_insert(_entry);
	return _entry;
}

void CompSt(Node* root , Vtype* func_ret){ //Done 出现在函数定义 以及 stmt中
	push_symStack();
	DefList(root->child[1], -1);
	StmtList(root->child[2], func_ret);
	pop_symStack();
}

void StmtList(Node* root , Vtype* func_ret){ //Done
	if(!root) return;
	Ilist* _ilist = Stmt(root->child[0], func_ret);
	if(_ilist){
		backpatch(_ilist, gen_label());
	}

	StmtList(root->child[1], func_ret);
}

Ilist* Stmt(Node* root , Vtype* func_ret){ //需要实现控制语句
	
	if(root->child_num == 2) {
		Exp(root->child[0]);
		return NULL;
	}
	else if(root->child_num == 1) {
		CompSt(root->child[0], func_ret);
		return NULL;
	}
	else if(root->child_num == 3){ 
		Eret* _exp = Exp(root->child[1]);
		InstType* tp = malloc(sizeof(InstType));
		instType[inst_num++] = tp;
		tp->type = INST_RETURN;

		if(_exp->_type == EXP_ADDR) _exp = addr_process(_exp);
		
		if(_exp->_type == EXP_INT) {
			tp->dst.id = -1;
			tp->dst.value = _exp->ival;
		}
		else if(_exp->_type == EXP_FLOAT) ;//sprintf(buf, "RETURN #%f\n", _exp->fval);
		// else if(_exp->_type == EXP_ADDR) sprintf(buf, "RETURN *v%d\n", _exp->var_id);
		else {
			tp->dst.id = _exp->var_id;
		}
		return NULL;
	}
	else if(root->child_num == 5){
		if(strcmp(root->child[0]->name, "IF") == 0){
			
			Eret* _exp = Exp(root->child[2]);
			backpatch(_exp->truelist, gen_label());
			Ilist* surfix = Stmt(root->child[4], func_ret); 
			return ilist_merge(_exp->falselist, surfix);
		}
		else{
			int label1 = gen_label();
			Eret* _exp = Exp(root->child[2]);
			int label2 = gen_label();
			Ilist* _ilist = Stmt(root->child[4], func_ret);
			backpatch(_ilist, label1);
			backpatch(_exp->truelist, label2);
			return _exp->falselist;
			//书上这里还有一条goto,但我感觉不太需要？
		}

	}
	else if(root->child_num == 7){ // if label1: _exp goto else 
		
		Eret* _exp = Exp(root->child[2]);

		int label1 = gen_label();
		Ilist* _ilist1 = Stmt(root->child[4], func_ret);
		Ilist* _n = NULL;
		if(inst_num > 0 && instType[inst_num - 1]->type != INST_RETURN)
			_n = gen_goto();

		int label2 = gen_label();
		Ilist* _ilist2 = Stmt(root->child[6], func_ret);
		
		backpatch(_exp->truelist, label1);
		backpatch(_exp->falselist, label2);

		return ilist_merge(_ilist1, ilist_merge(_n, _ilist2));

	}
	else assert(0);
}

Vlist* DefList(Node* root, int _offset){ //Done 返回定义的Vlist, 在结构体定义中记录offset，其他定义中记录var_id

	if(!root) return NULL;
	Vlist* prefix = Def(root->child[0], _offset);

	if(_offset == -1) return DefList(root->child[1], -1); //最后Compst中得到NULL, 非结构体变量
	
	Vlist* surfix = DefList(root->child[1], _offset + prefix->var_type->width + prefix->followed_sz);
	Vlist* tail = prefix;
	int tem_sz = 0;
	if(surfix) tem_sz = surfix->followed_sz + surfix->var_type->width;
	while(tail->next != NULL) {
		tail->followed_sz += tem_sz;
		tail = tail->next;
	}
	tail->next = surfix;

	if(surfix) surfix->pre = tail;

	return prefix;
}

Vlist* Def(Node* root, int _offset){
	Vtype* _type = Specifier(root->child[0]);
	return DecList(root->child[1], _type, _offset);
}

Vlist* DecList(Node* root, Vtype* _type, int _offset){
	Entry* _entry = Dec(root->child[0], _type);
	Vlist* _vlist = (Vlist*)malloc(sizeof(Vlist));
	_vlist->name = _entry->name;
	_vlist->var_type = _type;
	_vlist->offset = _offset;
	
	_vlist->next = NULL;
	_vlist->followed_sz = 0;

	if(root->child_num == 3) {
		Vlist* follow = DecList(root->child[2], _type, _offset + _type->width);
		_vlist->next = follow;
		if(follow) {
			follow->pre = _vlist;
			_vlist->followed_sz = follow->followed_sz + follow->var_type->width;
		}
	}
	return _vlist;
}

Entry* Dec(Node* root, Vtype* _type){ // 变量定义; 将定义的变量插入表中，如果赋值，则varid记录了存储该变量值的临时变量id
	Entry* _entry = VarDec(root->child[0], _type);
	symTable_insert(_entry);
	if(root->child_num > 1) {
		Eret* _exp = Exp(root->child[2]);
		InstType* tp = malloc(sizeof(InstType));
		instType[inst_num++] = tp;
		tp->type = INST_ASSIGN;

		tp->dst.id = _entry->id;
		if(_exp->_type == TP_INT) {
			tp->src1.id = -1;
			tp->src1.value = _exp->ival;
		}
		else if(_exp->_type == TP_FLOAT) ; //sprintf(buf, "v%d := #%f", _entry->var_id, _exp->fval);
		else {
			tp->src1.id = _exp->var_id;
		}
	}
	else if(_entry->type->type == TP_ARRAY || _entry->type->type == TP_STRUCT_DEF){

		InstType* tp = malloc(sizeof(InstType));
		instType[inst_num++] = tp;
		tp->type = INST_DEC;
		tp->dst.id = _entry->id;
		tp->dst.value = _entry->type->width; //不会出现struct

		if(_entry->type->type == TP_ARRAY) ;
		if(_type->type == TP_STRUCT_DEF) ;
		
	}
	return _entry; //struct内部不能赋值
}

Eret* Exp(Node* root){ //
	if(root->child_num == 1){
		Eret* _exp = malloc(sizeof(Eret));
		if(strcmp(root->child[0]->name, "ID") == 0){
			Entry* _entry = _find(root->child[0]->text);
			assert(_entry);
			_exp->name = _entry->name;
			_exp->var_id = _entry->id;
			_exp->_type = EXP_VAR;
			_exp->_vtype = _entry->type;
			return _exp;
		}
		
		if(strcmp(root->child[0]->name, "INT") == 0) {
			_exp->_type = EXP_INT;
			_exp->ival = root->child[0]->ival;
		}
		else if(strcmp(root->child[0]->name, "FLOAT") == 0) {
			_exp->_type = EXP_FLOAT;
			_exp->fval = root->child[0]->fval;
		}
		return _exp;
	}
	else if(root->child_num == 2){
		Eret* _entry = Exp(root->child[1]);
		if(strcmp(root->child[0]->name, "MINUS") == 0){
			if(_entry->_type == EXP_ADDR) _entry = addr_process(_entry);
			if(_entry->_type == EXP_INT) _entry->ival = - _entry->ival;
			else if(_entry->_type == EXP_FLOAT) _entry->fval = - _entry->fval;
			else{
				InstType* tp = malloc(sizeof(InstType));
				instType[inst_num++] = tp;

				tp->type = INST_SUB;
				tp->op = "-";
				tp->dst.id = new_tmpSym();
				tp->src1.id = -1;
				tp->src1.value = 0;
				tp->src2.id = _entry->var_id;
			}

			return _entry;
		}
		if(strcmp(root->child[0]->name, "NOT") == 0){  //实验要求中好像没提NOT的指令
			Eret* _exp = Exp(root->child[1]);
			Ilist* tem = _exp->truelist;
			_exp->truelist = _exp->falselist;
			_exp->falselist = tem;
			return _exp;
		} 
	}
	else if(root->child_num == 3){
		if(strcmp(root->child[1]->name, "ASSIGNOP") == 0){
			
			Eret* _exp1 = Exp(root->child[0]);
			
			Eret* _exp2 = Exp(root->child[2]);
			
			InstType* tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;

			if(_exp2->_type == EXP_ADDR) _exp2 = addr_process(_exp2);
			if(_exp1->_type == EXP_ADDR){
				tp->type = INST_STAR;
				tp->dst.id = _exp1->var_id;
				if(_exp2->_type == EXP_INT) {
					// sprintf(buf, "*v%d := %d", _exp1->var_id, _exp2->ival);
					tp->src1.id = -1;
					tp->src1.value = _exp2->ival;
				}
				else if(_exp2->_type == EXP_FLOAT) ; //sprintf(buf, "*v%d := #%f", _exp1->var_id, _exp2->fval);
				else {
					tp->src1.id = _exp2->var_id;
				}
			}
			else{
				tp->type = INST_ASSIGN;
				tp->dst.id = _exp1->var_id;
				if(_exp2->_type == EXP_INT) {
					tp->src1.id = -1;
					tp->src1.value = _exp2->ival;
				}
				else if(_exp2->_type == EXP_FLOAT) ; //sprintf(buf, "v%d := #%f", _exp1->var_id, _exp2->fval);
				// else if(_exp2->_type == EXP_ADDR) sprintf(buf, "v%d := *v%d", _exp1->var_id, _exp2->var_id);
				else {
					tp->src1.id = _exp2->var_id;
				}
			}
			// insts[inst_num++] = buf;
			return _exp1;
		}
		else if(strcmp(root->child[1]->name, "AND") == 0){

			Eret* _exp1 = Exp(root->child[0]);
			
			InstType* tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;

			backpatch(_exp1->truelist, label_num);
			tp->type = INST_LABEL;
			tp->dst.value = label_num ++ ;

			Eret* _exp2 = Exp(root->child[2]);

			_exp1->truelist = _exp2->truelist;
			_exp1->falselist->tail->next = _exp2->falselist;
			_exp1->falselist->tail = _exp2->falselist->tail;

			return _exp1;
		}
		else if(strcmp(root->child[1]->name, "OR") == 0){
			Eret* _exp1 = Exp(root->child[0]);
			InstType* tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;

			backpatch(_exp1->falselist, label_num);
			tp->type = INST_LABEL;
			tp->dst.id = label_num;

			Eret* _exp2 = Exp(root->child[2]);

			_exp1->falselist = _exp2->falselist;
			_exp1->truelist->tail->next = _exp2->truelist;
			_exp1->truelist->tail = _exp2->truelist->tail;
			return _exp1;
		}
		else if(strcmp(root->child[1]->name, "RELOP") == 0){
			Eret* _exp1 = Exp(root->child[0]);
			Eret* _exp2 = Exp(root->child[2]);
			_exp1->truelist = malloc(sizeof(Ilist));
			_exp1->falselist = malloc(sizeof(Ilist));
			_exp1->truelist->tail = _exp1->truelist;
			_exp1->truelist->next = NULL;
			_exp1->truelist->inst_id = inst_num;

			InstType* tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;
			tp->type = INST_IF;
			if(_exp1->_type == EXP_ADDR) _exp1 = addr_process(_exp1);
			if(_exp2->_type == EXP_ADDR) _exp2 = addr_process(_exp2);
			// if(_exp1->_type == EXP_ADDR) sprintf(buf, "IF *v%d %s ", _exp1->var_id, root->child[1]->text);
			if(_exp1->_type == EXP_INT) {
				tp->src1.id = -1;
				tp->src1.value = _exp1->ival;
			}
			else if(_exp1->_type == EXP_FLOAT) ; //sprintf(buf, "IF #%f %s ", _exp1->fval, root->child[1]->text);
			else {
				tp->src1.id = _exp1->var_id;
			}
			tp->op = root->child[1]->text;
			if(_exp2->_type == EXP_INT) {
				tp->src2.id = -1;
				tp->src2.value = _exp2->ival;
			}
			else if(_exp2->_type == EXP_FLOAT) ; //sprintf(buf+strlen(buf), "#%f GOTO ", _exp2->fval);
			// else if(_exp2->_type == EXP_ADDR) sprintf(buf+strlen(buf), "*v%d GOTO ", _exp2->var_id);
			else {
				tp->src2.id = _exp2->var_id;
			}
			_exp1->falselist->tail = _exp1->falselist;
			_exp1->falselist->next = NULL;
			_exp1->falselist->inst_id = inst_num;

			tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;
			tp->type = INST_GOTO;

			return _exp1;
		}
		else if(strcmp(root->child[1]->name, "PLUS") == 0 || strcmp(root->child[1]->name, "MINUS") == 0 
			 || strcmp(root->child[1]->name, "STAR") == 0 || strcmp(root->child[1]->name, "DIV") == 0){
			Eret* _entry1 = Exp(root->child[0]);
			Eret* _entry2 = Exp(root->child[2]);
			if(_entry1->_type == EXP_ADDR) _entry1 = addr_process(_entry1);
			if(_entry2->_type == EXP_ADDR) _entry2 = addr_process(_entry2);

			assert(_entry1->_type != EXP_FLOAT && _entry2->_type != EXP_FLOAT);

			if(_entry1->_type == EXP_INT && _entry2->_type == EXP_INT){
				if(strcmp(root->child[1]->name, "PLUS") == 0) _entry1->ival += _entry2->ival;
				else if(strcmp(root->child[1]->name, "MINUS") == 0) _entry1->ival -= _entry2->ival;
				else if(strcmp(root->child[1]->name, "STAR") == 0) _entry1->ival *= _entry2->ival;
				else if(strcmp(root->child[1]->name, "DIV") == 0) _entry1->ival /= _entry2->ival;
				return _entry1;
			}
			Eret* ret = malloc(sizeof(Eret));
			ret->_type = EXP_VAR;
			ret->var_id = new_tmpSym();

			InstType* tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;

			tp->dst.id = ret->var_id;
			if(_entry1->_type == EXP_INT) {
				tp->src1.id = -1;
				tp->src1.value = _entry1->ival;
			}
			else if(_entry1->_type == EXP_FLOAT) ; // sprintf(buf+strlen(buf), "#%f", _entry1->fval);
			// else if(_entry1->_type == EXP_ADDR) sprintf(buf+strlen(buf), "*v%d", _entry1->var_id);
			else {
				tp->src1.id = _entry1->var_id;
			}

			if(strcmp(root->child[1]->name, "PLUS") == 0) {
				tp->type = INST_ADD;
				tp->op = "+";
			}
			else if(strcmp(root->child[1]->name, "MINUS") == 0) {
				tp->type = INST_SUB;
				tp->op = "-";
			}
			else if(strcmp(root->child[1]->name, "STAR") == 0) {
				tp->type = INST_MUL;
				tp->op = "*";
			}
			else if(strcmp(root->child[1]->name, "DIV") == 0) {
				tp->type = INST_DIV;
				tp->op = "/";
			}
			else assert(0);
			if(_entry2->_type == EXP_INT) {
				tp->src2.id = -1;
				tp->src2.value = _entry2->ival;
			}
			else if(_entry2->_type == EXP_FLOAT) ; //sprintf(buf+strlen(buf), "#%f", _entry2->fval);
			// else if(_entry2->_type == EXP_ADDR) sprintf(buf+strlen(buf), "*v%d", _entry2->var_id);
			else {
				tp->src2.id = _entry2->var_id;
			}
			return ret;
		}
		else if(strcmp(root->child[0]->name, "LP") == 0) {
			return Exp(root->child[1]);
		}
		
		else if(strcmp(root->child[0]->name, "ID") == 0){ // ID LP RP
			Eret* _exp = malloc(sizeof(Eret));
			_exp->var_id = new_tmpSym();
			_exp->_type = EXP_VAR;

			InstType* tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;

		
			tp->type = INST_CALL;
			tp->dst.id = _exp->var_id;
			tp->name = root->child[0]->text;
			return _exp;
		}
		else if(strcmp(root->child[1]->name, "DOT") == 0){ //lab4不会出现
			assert(0);
			Eret* _exp = Exp(root->child[0]); //开头位置存储在变量中

			Eret* ret_exp = malloc(sizeof(Eret));
			ret_exp->_type = EXP_ADDR;
			ret_exp->var_id = new_tmpSym();

			int offset = 0;
			for(Vlist* iter = _exp->_vtype->_vlist; iter; iter = iter->next){
				assert(iter && iter->name);
				if(strcmp(iter->name, root->child[2]->text) == 0){
					offset = iter->offset;
					ret_exp->_vtype = iter->var_type;
					break;
				}
			}
				
			
			char* buf = malloc(MAX_INST_WIDTH);
			sprintf(buf, "v%d := v%d + #%d", ret_exp->var_id, _exp->var_id, offset);
			return ret_exp;
			
		}
		else assert(0);
	}
	else if(root->child_num == 4){
		if(strcmp(root->child[1]->name, "LP") == 0){
			Entry* _entry = _find(root->child[0]->text);
			Eret* _exp = malloc(sizeof(Eret));
			_exp->var_id = new_tmpSym();
			
			Args(root->child[2]);

			InstType* tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;

			tp->type = INST_CALL;
			tp->dst.id = _exp->var_id;
			tp->name = _entry->name;

			_exp->_type = EXP_VAR;

			return _exp;
		}
		if(strcmp(root->child[1]->name, "LB") == 0){
			assert(strcmp(root->child[2]->child[0]->name, "INT") == 0);

			if(strcmp(root->child[0]->child[0]->name, "ID") == 0){
				Entry* _entry = _find(root->child[0]->child[0]->text);
				assert(_entry->type->type == TP_ARRAY);
				Eret* _exp = malloc(sizeof(Eret));
				_exp->_type = EXP_ADDR;
				_exp->var_id = new_tmpSym();
				_exp->_vtype = _entry->type->next; //存储array中元素的类型

				InstType* tp = malloc(sizeof(InstType));
				instType[inst_num++] = tp;
				tp->type = INST_ADD;
				tp->dst.id = _exp->var_id;
				tp->src1.id = _entry->id;
				tp->src2.id = -1;
				tp->src2.value = _entry->type->ele_width * root->child[2]->child[0]->ival;
				
				return _exp;
			}
			Eret* _exp = Exp(root->child[0]);
			_exp->_vtype = _exp->_vtype->next;

			InstType* tp = malloc(sizeof(InstType));
			instType[inst_num++] = tp;
			tp->type = INST_ADD;
			tp->dst.id = _exp->var_id;
			tp->src1.id = _exp->var_id;
			tp->src2.id = -1;
			tp->src2.value = _exp->_vtype->ele_width * root->child[2]->child[0]->ival;
				

			return _exp;
		}
	}
	assert(0);
}

static void Args(Node* root){ 

	Eret* _exp = Exp(root->child[0]);

	InstType* tp = malloc(sizeof(InstType));
	instType[inst_num++] = tp;

	tp->type = INST_ARG;
	if(_exp->_type == EXP_INT) {
		tp->dst.id = -1;
		tp->dst.value = _exp->ival;
	}
	else if(_exp->_type == EXP_FLOAT) ; //sprintf(buf, "ARG #%f", _exp->fval);
	else if(_exp->_type == EXP_VAR) {
		tp->dst.id = _exp->var_id;
	}
	else { //参数为地址时暂时不处理
		// sprintf(buf, "ARG &v%d", _exp->var_id);
	}

	if(root->child_num > 1) Args(root->child[2]);

}

Vtype* TYPE_token(Node* root){  //DONE: 返回TYPE的类型
	Vtype* _type = (Vtype*)malloc(sizeof(Vtype));
	if(strcmp(root->text, "int") == 0) {
		_type->type = TP_INT;
		_type->basic_int = atoi(root->text);  // TODO: type node has value ?
	}
	else if(strcmp(root->text, "float") == 0) {
		_type->type = TP_FLOAT;
		_type->basic_float = atof(root->text);
	}
	else {
		printf("wrong type: %s\n", root->text);
		assert(0);
	}
	_type->width = 4;
	return _type;
}

static void backpatch(Ilist* tflist, int label_id){
	for(Ilist* _list = tflist; _list; _list = _list->next){
		instType[_list->inst_id]->dst.id = label_id;
	}
}

static int gen_label(){
	InstType* tp = malloc(sizeof(InstType));
	instType[inst_num++] = tp;
	tp->type = INST_LABEL;
	tp->dst.id = label_num ++;

	return label_num - 1;
}

static Ilist* gen_goto(){
	Ilist* _ilist = malloc(sizeof(Ilist));
	_ilist->inst_id = inst_num;
	_ilist->next = NULL;
	_ilist->tail = _ilist;
	InstType* tp = malloc(sizeof(InstType));
	instType[inst_num++] = tp;
	tp->type = INST_GOTO;

	char* buf = malloc(MAX_INST_WIDTH);

	sprintf(buf, "GOTO ");
	return _ilist;
}

static Ilist* ilist_merge(Ilist* _ilist1, Ilist* _ilist2){
	if(!_ilist1) return _ilist2;
	if(!_ilist2) return _ilist1;
	_ilist1->tail->next = _ilist2;
	_ilist1->tail = _ilist2->tail;
	return _ilist1;
}

static Eret* addr_process(Eret* _exp){
	assert(_exp->_type == EXP_ADDR);
	_exp->_type = EXP_VAR;
	_exp->var_id = new_tmpSym();

	InstType* tp = malloc(sizeof(InstType));
	instType[inst_num++] = tp;
	tp->type = INST_DEREF;
	tp->dst.id = _exp->var_id;
	tp->src1.id = _exp->var_id;
	return _exp;
}