#ifndef INTERCODE_H
#define INTERCODE_H

#include <Node.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_INST_NUM 4096
#define MAX_ENTRY_NUM 2048
#define MAX_SYM_NUM 1024
#define MAX_INST_WIDTH 64
#define MAX_STACK_DEPTH 128

enum EXP_TYPE{EXP_INT = 1, EXP_FLOAT, EXP_VAR, EXP_ADDR}; //exp返回类型
enum InterType_ {TP_INT=1, TP_FLOAT, TP_ARRAY, TP_FUNC, TP_STRUCTURE}; //数据结构类型

typedef struct VAR_LIST{       // 存储struct内部变量
  char* name;                //变量名
  struct VAR_TYPE* var_type; //变量类型
  struct VAR_LIST* next;     //指针
  struct VAR_LIST* pre;
  int offset;                //变量开头位置想对于结构体开始的位置
  int followed_sz;           //跟随变量大小（方便中间结果计算）
  int var_id;                //在计算中如果该变量开始位置被载入临时变量，则存储变量id
}Vlist;

typedef struct VAR_TYPE{       //存储变量的类型信息
  int type;                  //变量类别：BASIC, ARRAY...
  int width;
  union{
    struct{
      // int basic_type;     //对于BASIC 记录为int类型还是float类型
      int basic_int;      //int类型的值
      float basic_float;  //float类型的值
    };
    /* Type-Array */
    struct{
      int sz;             //数组大小
      int ele_width;      //数组中元素宽度
      struct VAR_TYPE* next; //数组中元素的类型
    };
    /* structure and structure_var */
    struct{
      struct VAR_LIST* _vlist;  //结构体中变量链表
    };
    /* Type-FUNC */
    struct{
      struct VAR_LIST* func_para;  //函数参数
      struct VAR_TYPE* ret_type;   //函数返回类型
    };
  };
}Vtype;

typedef struct HASH_ENTRY{  //符号表中元素结构（实际上并没有使用hash表实现）
  char* name;             //符号名
	int id;
  Vtype* type;            //符号类型
}Entry;

typedef struct EXP_RET{     //EXP返回类型，记录EXP解析的信息
  int _type;              //该EXP数据的类型：常数EXP_INT/FLOAT, 变量id EXP_VAR
  union{
    int ival;
    float fval;
    struct {
      char* name;     //好像没有用？
      Vtype* _vtype;  //对于复杂类型，输出其类型信息
    };
  };
  int var_id;
  struct INST_LIST* truelist;
  struct INST_LIST* falselist;
}Eret;

typedef struct INST_LIST{
  int inst_id;
  struct INST_LIST* next;
  struct INST_LIST* tail;
}Ilist;

// void gen_intercode(Node* root);
typedef struct INFO{
  int id;
  int value;
}Info;

typedef struct INSTTYPE{
  enum {INST_FUNC=1, INST_LABEL, INST_DEREF, INST_ASSIGN, INST_OP,
				INST_ADD, INST_SUB, INST_MUL, INST_DIV, INST_ADDR, INST_STAR,
				INST_GOTO, INST_IF, INST_RETURN, INST_DEC, INST_ARG, INST_CALL, INST_PARAM}inst_type;
  int type;
  char* name;
  Info src1;
  char* op;
  Info src2;
  Info dst;
}InstType;

typedef struct symStack{
	// int tmp_symId;
	int sym_num;
}symStack_t;

/* all intercode
LABEL x :               定义标号x
FUNCTION f :            定义函数f。
x := y                  赋值操作。
x := y + z              加法操作。
x := y - z              减法操作。
x := y * z              乘法操作。
x := y / z              除法操作。
x := &y                 取y的地址赋给x。
x := *y                 取以y值为地址的内存单元的内容赋给x。
*x := y                 取y值赋给以x值为地址的内存单元。
GOTO x                  无条件跳转至标号x。
IF x [relop] y GOTO z   如果x与y满足[relop]关系则跳转至标号z。
RETURN x                退出当前函数并返回x值。
DEC x [size]            内存空间申请，大小为4的倍数。
ARG x                   传实参x。
x := CALL f             调用函数，并将其返回值赋给x。
PARAM                   x函数参数声明。
*/
#endif

