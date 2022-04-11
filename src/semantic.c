#include "semantic.h"
static unsigned int hash_pjw(char* name);
static void hash_init();
static void stack_init();

static int decfunc_check();
static int hash_insert(Symbol* sym);
static void hash_pop(int depth);
static Stype* hash_find(char* name);
static void decfunc_insert(Symbol* _sym);
static void program(Node* root);
static void ExtDefList(Node* root);
static void ExtDef(Node* root, int depth);
static void ExtDecList(Node* root, Stype* _type, int depth);
static Stype* Specifier(Node* root, int depth);
static Stype* StructSpecifier(Node* root, int depth);
static char* Tag(Node* root);
static Symbol* VarDec(Node* root, Stype* _type, int depth, int is_struct);
static Symbol* FunDec(Node* root, Stype* ret_type, int depth);
static Plist* VarList(Node* root, int depth);
static Symbol* ParamDec(Node* root, int depth);
static void CompSt(Node* root, int depth, Stype* func_ret);
static void StmtList(Node* root, int depth, Stype* func_ret);
static void Stmt(Node* root, int depth, Stype* func_ret);
static Plist* DefList(Node* root, int depth, int is_struct);
static Plist* Def(Node* root, int depth, int is_struct);
static Plist* DecList(Node* root, Stype* _type, int depth, int is_struct);
static Symbol* Dec(Node* root, Stype* _type, int depth, int is_struct);
static Stype* Exp(Node* root);
static Plist* Args(Node* root);
static Stype* TYPE_token(Node* root, int depth);
static Stype* find_field(Plist* _plist, char* id);
static int type_equal(Stype* _type1, Stype* _type2);
static int para_equal(Plist* _plist1, Plist* _plist2);

static Symbol* hashtable[HASH_SZ];
static Stack* stack;
static Stack* decfunc_list;
static int is_lval = 0;
// Stype* _type_int;
// Stype* _type_float;
static unsigned int hash_pjw(char* name){
    unsigned int val = 0, i;
    for (; *name; ++name){
        val = (val << 2) + *name;
        if (i = val & ~HASH_SZ) val = (val ^ (i >> 12)) & HASH_SZ;
    }
    return val;
}

static void hash_init(){
    for(int i = 0; i < HASH_SZ; i++) hashtable[i] = NULL;
}

static void stack_init(){
    stack = (Stack*)malloc(sizeof(Stack));
    stack->sz = 0;
    decfunc_list = (Stack*)malloc(sizeof(Stack));
    decfunc_list->sz = 0;
}

int semantic_check(Node* root){
    hash_init();
    stack_init();
    program(root);
    decfunc_check();
}

static int decfunc_check(){
    for(int i = 0; i < decfunc_list->sz; i++){
        Symbol* _func = decfunc_list->s[i];
        Stype* _type = hash_find(_func->name);
        if(!_type){
            printf("Error type 18 at Line %d: function \"%s\" declared but never defined\n", _func->depth, _func->name);
        }
        else if(!type_equal(_func->type, _type)){
            printf("Error type 19 at Line %d: conflicting types for function \"%s\"\n", _func->depth, _func->name);
        }
    }
}

static int hash_insert(Symbol* sym){ // return 1插入成功, return 0: 失败，存在相同元素
    // printf("insert: %s %d\n", sym->name, sym->depth);
    if(sym->type->type == STRUCTURE) sym->depth = 0;
    int idx = hash_pjw(sym->name);
    // printf("insert: %s %d %d\n", sym->name, idx, sym->depth);
    for(Symbol* cur = hashtable[idx]; cur; cur = cur->slot_next){
        // printf("cmp %s %s\n", cur->name, sym->name);
        if(strcmp(cur->name, sym->name) == 0 && (cur->depth == sym->depth  
        || cur->type->type == STRUCTURE || sym->type->type == STRUCTURE)) return 0;
    }
    sym->slot_next = hashtable[idx];
    sym->slot_pre = NULL;
    if(hashtable[idx]) hashtable[idx]->slot_pre = sym;
    hashtable[idx] = sym;
    sym->depth_next = stack->s[sym->depth];
    stack->s[sym->depth] = sym;
    return 1;
}

static void hash_pop(int depth){
    for(Symbol* cur = stack->s[depth]; cur; cur = cur->depth_next){
        // printf("pop: %s %d\n", cur->name, cur->depth);
        if(cur->slot_pre){
            cur->slot_pre->slot_next = cur->slot_next;
        }
        else {
            int idx = hash_pjw(cur->name);
            hashtable[idx] = cur->slot_next;
        }
        if(cur->slot_next) cur->slot_next->slot_pre = cur->slot_pre;
    }
    stack->s[depth] = NULL;

}

static Stype* hash_find(char* name){ //在符号表中查找名为name的符号，并返回其类型
    int idx = hash_pjw(name);
    for(Symbol* _sym = hashtable[idx]; _sym; _sym = _sym->slot_next){
        if(strcmp(_sym->name, name) == 0){
            return _sym->type;
        }
    }
    return NULL;
}

static void decfunc_insert(Symbol* _sym){
    for(int i = 0; i < decfunc_list->sz; i++){
        if(strcmp(_sym->name, decfunc_list->s[i]->name) == 0 && !type_equal(_sym->type, decfunc_list->s[i]->type)){
            printf("Error type 19 at Line %d: Inconsistent declaration of function \"%s\"\n", _sym->depth, _sym->name);
            return;
        }
    }
    decfunc_list->s[decfunc_list->sz++] = _sym;
}

static void program(Node* root){
    ExtDefList(root->child[0]);
}

static void ExtDefList(Node* root){
    if(!root) return;   
    ExtDef(root->child[0], 0);
    ExtDefList(root->child[1]);
}

static void ExtDef(Node* root, int depth){
    #ifdef DEBUG
    assert(root && root->child_num >= 2 && strcmp(root->child[0]->name, "Specifier") == 0);
    #endif
    Stype* _type = Specifier(root->child[0], depth);
    if(strcmp(root->child[1]->name, "ExtDecList") == 0){
        ExtDecList(root->child[1], _type, depth);
    }
    else if(strcmp(root->child[1]->name, "SEMI") == 0) return;
    else if(strcmp(root->child[2]->name, "CompSt") == 0){
        Symbol* _sym = FunDec(root->child[1], _type, depth+1);
        _sym->depth = depth;
        if(!hash_insert(_sym)) printf("Error type 4 at Line %d: Redefined function \"%s\"\n", root->child[0]->lineno, _sym->name);
        CompSt(root->child[2], depth+1, _type);
        hash_pop(depth+1);
    }
    else if(strcmp(root->child[2]->name, "SEMI") == 0){
        Symbol* _sym = FunDec(root->child[1], _type, depth+1);
        hash_pop(depth+1);
        _sym->depth = root->child[1]->lineno;
        decfunc_insert(_sym);
        
    }
    else assert(0);
}

static void ExtDecList(Node* root, Stype* _type, int depth){ //Done 不会出现在struct中，因此不需要返回Plist, 只需要插入hash
    VarDec(root->child[0], _type, depth, 0);
    if(root->child_num > 1) ExtDecList(root->child[2], _type, depth);
}

static Stype* Specifier(Node* root, int depth){ //Done
    #ifdef DEBUG
    assert(strcmp(root->name, "Specifier") == 0 && root->child_num > 0);
    #endif
    // Stype* spe_type = (Stype*)malloc(sizeof(Stype));
    if(strcmp(root->child[0]->name, "TYPE") == 0) return TYPE_token(root->child[0], depth);
    else if(strcmp(root->child[0]->name, "StructSpecifier") == 0) return StructSpecifier(root->child[0], depth);
    assert(0);
}

static Stype* StructSpecifier(Node* root, int depth){ //Done，返回解析的结构类型
    #ifdef DEBUG
    assert(strcmp(root->name, "StructSpecifier") == 0 && root->child_num >= 2 && strcmp(root->child[0], "STRUCT") == 0);
    #endif
    Stype* _type; //结构类型STRUCTURE
    Stype* _type2 = (Stype*)malloc(sizeof(Stype)); //STRUCTURE_VAR
    _type2->type = STRUCT_VAR;
    // printf("struct spe %d %d\n", root->child[1]== NULL, root->child_num);
    if(!root->child[1] || strcmp(root->child[1]->name, "OptTag") == 0){
        char* _name = Tag(root->child[1]);
        _type = (Stype*)malloc(sizeof(Stype));
        Plist* _deflist = DefList(root->child[3], depth+1, 1); 
        _type->type = STRUCTURE;
        _type->struct_para = _deflist;
        if(_name){ //插入structure类型
            Symbol* _struct_sym = (Symbol*)malloc(sizeof(Symbol));
            _struct_sym->type = _type;
            _struct_sym->name = _name;
            _struct_sym->depth = depth;
            if(!hash_insert(_struct_sym))printf("Error type 16 at Line %d: symbol \"%s\" has been defined\n", root->child[1]->lineno, _struct_sym->name);
        }
        hash_pop(depth+1);
        _type2->struct_para = _deflist;
    }
    else if(strcmp(root->child[1]->name, "Tag") == 0) { // 需要去符号表中查找类型id, 先定义后使用，因此保证能查找到
        char* _name = Tag(root->child[1]);
        assert(_name);
        _type = hash_find(_name);
        if(!_type || _type->type != STRUCTURE) printf("Error type 17 at Line %d: undefined structure \"%s\"\n", root->child[1]->lineno, _name);
        else _type2->struct_para = _type->struct_para;
        // return _type;
    }
    else assert(0);
    return _type2;
}

static char* Tag(Node* root){ //Done: OptTag and Tag
    if(!root) return NULL;
    return root->child[0]->text;
}

static Symbol* VarDec(Node* root, Stype* _type, int depth, int is_struct){ //Done: 解析变量/数组
    Symbol* _sym;
    if(strcmp(root->child[0]->name, "ID") == 0){
        _sym = (Symbol*)malloc(sizeof(Symbol));
        _sym->name = root->child[0]->text;
        _sym->type = _type;
        _sym->depth = depth;
        if(!hash_insert(_sym)) {
            if(is_struct) printf("Error type 15 at Line %d: Redefined field \"%s\"\n", root->child[0]->lineno, _sym->name);
            else printf("Error type 3 at Line %d: Redefined variable \"%s\"\n", root->child[0]->lineno, _sym->name);
        }
    }
    else if(strcmp(root->child[0]->name, "VarDec") == 0){
        Stype* new_head = (Stype*)malloc(sizeof(Stype));
        new_head->type = ARRAY;
        new_head->sz = root->child[2]->ival;
        new_head->next = _type;
        _sym = VarDec(root->child[0], new_head, depth, is_struct);
        // printf("_sym_type: %d\n", _sym->type->type);
        
    }
    else assert(0);
    return _sym;
}

static Symbol* FunDec(Node* root, Stype* ret_type, int depth){ //Done
    Symbol* _sym = malloc(sizeof(Symbol));
    Stype* _type = malloc(sizeof(Stype));
    _sym->name = root->child[0]->text;
    _sym->type = _type;
    _sym->depth = depth;
    _type->type = FUNC;
    _type->ret_type = ret_type;
    _type->func_para = NULL;
    if(strcmp(root->child[2]->name, "VarList") == 0) _type->func_para = VarList(root->child[2], depth);
    return _sym;
}

static Plist* VarList(Node* root, int depth){ //Done 返回参数列表Plist
    Symbol* _sym = ParamDec(root->child[0], depth);
    Plist* _plist = malloc(sizeof(Plist));
    _plist->para = _sym;
    _plist->next = NULL;
    _plist->tail = _plist;
    if(root->child_num > 1){
        Plist* _surfix = VarList(root->child[2], depth);
        _plist->next = _surfix;
        _plist->tail = _surfix->next;
    }
    return _plist;
}

static Symbol* ParamDec(Node* root, int depth){ //Done
    Stype* _type = Specifier(root->child[0], depth);
    Symbol* _sym = VarDec(root->child[1], _type, depth, 0);
    return _sym;
}

static void CompSt(Node* root, int depth, Stype* func_ret){ //Done
    DefList(root->child[1], depth, NOT_STRUCTURE);
    StmtList(root->child[2], depth, func_ret);
}

static void StmtList(Node* root, int depth, Stype* func_ret){ //Done
    if(!root) return;
    Stmt(root->child[0], depth, func_ret);
    StmtList(root->child[1], depth, func_ret);
}

static void Stmt(Node* root, int depth, Stype* func_ret){ //Done
    // printf("stmt %d %d\n", depth, root->child_num);
    if(root->child_num == 2) Exp(root->child[0]);
    else if(root->child_num == 1) {
        CompSt(root->child[0], depth+1, func_ret);
        hash_pop(depth+1);
    }
    else if(root->child_num == 3){
        Stype* _type = Exp(root->child[1]);
        if(!_type) return;
        if(!type_equal(_type, func_ret)) printf("Error type 8 at Line %d: function return type mismatched\n",root->child[1]->lineno);
    }
    else if(root->child_num == 5){
        Exp(root->child[2]);
        // Stype* _type = Exp(root->child[2]);
        // if(_type->type != BASIC || _type->basic_type != TYPE_INT) 
        Stmt(root->child[4], depth, func_ret);
        hash_pop(depth+1);
    }
    else if(root->child_num == 7){
        Exp(root->child[2]);
        Stmt(root->child[4], depth, func_ret);
        hash_pop(depth+1);
        Stmt(root->child[6], depth, func_ret);
        hash_pop(depth+1);
    }
    else assert(0);
}

static Plist* DefList(Node* root, int depth, int is_struct){ //Done: 解析deflist, 返回Plist类型
    if(!root) return NULL;
    Plist* prefix = Def(root->child[0], depth, is_struct);
    Plist* surfix = DefList(root->child[1], depth, is_struct);
    prefix->tail->next = surfix;
    if(surfix) prefix->tail = surfix->tail;
    return prefix;
}

static Plist* Def(Node* root, int depth, int is_struct){ //Done
    Stype* _type = Specifier(root->child[0], depth);
    return DecList(root->child[1], _type, depth, is_struct);
}

static Plist* DecList(Node* root, Stype* _type, int depth, int is_struct){ //Done
    Symbol* _sym = Dec(root->child[0], _type, depth, is_struct);
    Plist* _plist = (Plist*)malloc(sizeof(Plist));
    _plist->para = _sym;
    _plist->tail = _plist;
    _plist->next = NULL;
    if(root->child_num == 3) {
        Plist* follow = DecList(root->child[2], _type, depth, is_struct);
        _plist->next = follow;
        _plist->tail = follow->tail;
    }
    return _plist;
}

static Symbol* Dec(Node* root, Stype* _type, int depth, int is_struct){ //Done
    Symbol* _sym = VarDec(root->child[0], _type, depth, is_struct);
    if(root->child_num > 1 && is_struct) printf("Error type 15 at Line %d: Field is initialized \"%s\"\n", root->child[1]->lineno, _sym->name);
    else if(root->child_num > 1) Exp(root->child[2]);
    return _sym; //struct内部不能赋值
}

static Stype* Exp(Node* root){ // Done
    // printf("Exp: %d %d\n", root->lineno, root->child_num);
    is_lval = 0;
    if(root->child_num == 1){
        if(strcmp(root->child[0]->name, "ID") == 0){
            Stype* _type = hash_find(root->child[0]->text);
            if(!_type) {
                printf("Error type 1 at Line %d: variable \"%s\" is not defined\n",root->child[0]->lineno, root->child[0]->text);
                return NULL;
            }
            is_lval = 1;
            return _type;
        }
        Stype* _type = malloc(sizeof(Stype));
        _type->type = BASIC;
        is_lval = 1;
        if(strcmp(root->child[0]->name, "INT") == 0) _type->basic_type = TYPE_INT;
        else if(strcmp(root->child[0]->name, "FLOAT") == 0) _type->basic_type = TYPE_FLOAT;
        return _type;
    }
    if(root->child_num == 2){
        Stype* _type = Exp(root->child[1]);
        if(strcmp(root->child[0]->name, "MINUS") == 0){
            if(_type->type != BASIC){
                printf("Error type 7 at Line %d: Operand type mismatched\n", root->child[0]->lineno);
                return NULL;
            }
        }
        if(strcmp(root->child[0]->name, "NOT") == 0){
            if(_type->type != BASIC || _type->basic_type != TYPE_INT){
                printf("Error type 7 at Line %d: Operand type mismatched\n", root->child[0]->lineno);
                return NULL;
            }
        }
        // is_lval = 0;
        return _type;
    }
    if(root->child_num == 3){
        if(strcmp(root->child[1]->name, "ASSIGNOP") == 0){
            Stype* _type1 = Exp(root->child[0]);
            // printf("%d %d\n", _type1 == NULL, is_lval);
            if(_type1 && !is_lval){
                printf("Error type 6 at Line %d: the left side of the assignment should be l-value\n", root->child[1]->lineno);
                return NULL;
            }
            Stype* _type2 = Exp(root->child[2]);
            if(_type1 && _type2){
                // printf("%d %d\n", _type1->type, _type2->type);
                if(!type_equal(_type1, _type2)){
                    printf("Error type 5 at Line %d: Operand type mismatched\n", root->child[1]->lineno);
                    return NULL;
                }
                
                return _type2;
            }
            return NULL;
        }
        else if(strcmp(root->child[1]->name, "AND") == 0 || strcmp(root->child[1]->name, "OR") == 0){
            Stype* _type1 = Exp(root->child[0]);
            Stype* _type2 = Exp(root->child[2]);
  
            if(_type1 && _type2){
                if(_type1->type == BASIC && _type1->basic_type == TYPE_INT && _type2->type == BASIC && _type2->basic_type == TYPE_INT){
                    is_lval = 0;
                    return _type1;
                }
                printf("Error type 7 at Line %d: Operand type mismatched\n", root->child[1]->lineno);
            }
            return NULL;
        }
        else if(strcmp(root->child[1]->name, "RELOP") == 0){
            Stype* _type1 = Exp(root->child[0]);
            Stype* _type2 = Exp(root->child[2]);
            is_lval = 0;
            if(_type1 && _type2){
                if(_type1->type == BASIC && type_equal(_type1, _type2)) {
                    Stype* ret_type = malloc(sizeof(Stype));
                    ret_type->type = BASIC;
                    ret_type->basic_type = TYPE_INT;
                    return ret_type;
                }
                printf("Error type 7 at Line %d: Operand type mismatched\n", root->child[1]->lineno);
            }
            return NULL;
        }
        else if(strcmp(root->child[1]->name, "PLUS") == 0 
            || strcmp(root->child[1]->name, "MINUS") == 0 || strcmp(root->child[1]->name, "STAR") == 0
            || strcmp(root->child[1]->name, "DIV") == 0){
            Stype* _type1 = Exp(root->child[0]);
            Stype* _type2 = Exp(root->child[2]);
            is_lval = 0;
            if(_type1 && _type2){
                if(_type1->type == BASIC && type_equal(_type1, _type2)) return _type1;
                printf("Error type 7 at Line %d: Operand type mismatched\n", root->child[1]->lineno);
            }
            return NULL;
        }
        else if(strcmp(root->child[0]->name, "LP") == 0) return Exp(root->child[1]);
        else if(strcmp(root->child[0]->name, "ID") == 0){
            Stype* _type = hash_find(root->child[0]->text);
            if(!_type){
                printf("Error type 2 at Line %d: undefined function \"%s\"\n", root->child[0]->lineno, root->child[0]->text);
                return NULL;
            }
            if(_type->type != FUNC){
                printf("Error type 11 at Line %d: symbol \"%s\" does not name a function\n", root->child[0]->lineno, root->child[0]->text);
                return NULL;
            }
            is_lval = 0;
            return _type->ret_type;
        }
        else if(strcmp(root->child[1]->name, "DOT") == 0){
            Stype* _type = Exp(root->child[0]);
            if(!_type) return NULL;
            // printf("%d\n", _type->type);
            if(_type->type != STRUCT_VAR){
                printf("Error type 13 at Line %d: Illegal use of \".\"\n", root->child[0]->lineno);
                return NULL;
            }
            Stype* field_type = find_field(_type->struct_para, root->child[2]->text);
            if(!field_type){
                printf("Error type 14 at Line %d: Non-existent field \"%s\"\n", root->child[0]->lineno, root->child[2]->text);
                return NULL;
            }
            is_lval = 1;
            return field_type;
        }
        else assert(0);
    }
    else if(root->child_num == 4){
        if(strcmp(root->child[1]->name, "LP") == 0){
            Stype* _type = hash_find(root->child[0]->text);
            if(!_type){
                printf("Error type 2 at Line %d: undefined function %s\n", root->child[0]->lineno, root->child[0]->text);
                return NULL;
            }
            if(_type->type != FUNC){
                printf("Error type 11 at Line %d: symbol \"%s\" does not name a function\n", root->child[0]->lineno, root->child[0]->text);
                return NULL;
            }
            if(!para_equal(Args(root->child[2]), _type->func_para)){
                printf("Error type 9 at Line %d: wrong parameters in function \"%s\"\n", root->child[0]->lineno, root->child[0]->text);
                return NULL;
            }
            is_lval = 0;
            return _type->ret_type;
        }
        if(strcmp(root->child[1]->name, "LB") == 0){
            Stype* _type1 = Exp(root->child[0]);
            Stype* _type2 = Exp(root->child[2]);
            // printf("type1: %d, type2: %d %d\n", _type1->type, _type2->type, _type2->basic_type);
            if(!_type1 || !_type2) return NULL;
            if(_type1->type != ARRAY){
                printf("Error type 10 at Line %d: Symbol does not name an array or Array dimension mismatched\n", root->child[0]->lineno);
                return NULL;
            }
            if(_type2->type != BASIC || _type2->basic_type != TYPE_INT){
                printf("Error type 12 at Line %d: The index of array should be integer\n", root->child[2]->lineno);
                return NULL;
            }
            is_lval = 1;
            return _type1->next;
        }
    }
    // printf("%d, %s %s\n", root->child_num, root->child[0]->name, root->child[1]->name);
    assert(0);
}

static Plist* Args(Node* root){ //Done
    Stype* _type = Exp(root->child[0]);
    Symbol* _sym = malloc(sizeof(Symbol));
    _sym->name = NULL;
    _sym->type = _type;
    Plist* _plist = malloc(sizeof(Plist));
    _plist->para = _sym;
    _plist->tail = _plist;
    _plist->next = NULL;
    if(root->child_num > 1){
        Plist* surfix = Args(root->child[2]);
        _plist->next = surfix;
        _plist->tail = surfix->tail;
    }
    return _plist;
}

static Stype* TYPE_token(Node* root, int depth){  //DONE: 返回TYPE的类型
    Stype* _type = (Stype*)malloc(sizeof(Stype));
    _type->type = BASIC;
    // _type->depth = depth;
    if(strcmp(root->text, "int") == 0) _type->basic_type = TYPE_INT;
    else if(strcmp(root->text, "float") ==  0) _type->basic_type = TYPE_FLOAT;
    else assert(0);
    return _type;
}

static Stype* find_field(Plist* _plist, char* id){
    if(!_plist) return NULL;
    // printf("%s %s\n", _plist->para->name, id);
    if(strcmp(_plist->para->name, id) == 0) return _plist->para->type;
    return find_field(_plist->next, id);
}

static int type_equal(Stype* _type1, Stype* _type2){ //Done: 判断两个类型是否结构等价
    // printf("emp: %d %d\n", _type1 == NULL, _type2 == NULL);
    if(!_type1 && !_type2) return 1;
    if(!_type1 || !_type2) return 0;
    // printf("%d %d\n", _type1->type, _type2->type);
    if(_type1->type != _type2->type) return 0;
    switch(_type1->type){
        case BASIC: return _type1->basic_type == _type2->basic_type;
        case ARRAY: 
            // if(_type1->sz != _type2->sz) return 0;
            return type_equal(_type1->next, _type2->next);
        case FUNC:
            return type_equal(_type1->ret_type, _type2->ret_type) && para_equal(_type1->func_para, _type2->func_para);
            // assert(0); //不会出现函数比较操作
        case STRUCT_VAR: 
        case STRUCTURE:
            return para_equal(_type1->struct_para, _type2->struct_para);
        default:
            assert(0);
    }
    
}

static int para_equal(Plist* _plist1, Plist* _plist2){ //Done: 判断两个参数列表是否结构等价
    if(!_plist1 && !_plist2) return 1;
    if(!_plist1 || !_plist2) return 0;
    if(!type_equal(_plist1->para->type, _plist2->para->type)) return 0;
    return para_equal(_plist1->next, _plist2->next);
}
/*
P247 6.4.3(3) A:2*3 int, B: 2*4 int C: 5 int
    int width=4
P248 6.4.8(1)(2)(3)
    read width=8

*/