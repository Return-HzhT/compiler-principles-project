#pragma once

#include "koopa.h"
#include <string>
#include <map>
#include <vector>

void koopa_process();
void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);

void Visit_ret(const koopa_raw_return_t &ret);
void Visit_integer(const koopa_raw_integer_t &int_num);
int Visit_binary(const koopa_raw_binary_t &binary);
void Visit_alloc(const koopa_raw_value_t &value);
void Visit_store(const koopa_raw_store_t &store);
int Visit_load(const koopa_raw_load_t &load);
void Visit_branch(const koopa_raw_branch_t &branch);
void Visit_jump(const koopa_raw_jump_t &jump);
void Visit_call(const koopa_raw_call_t &call);
void Visit_global_alloc(const koopa_raw_value_t &value);


int set_prologue(const koopa_raw_function_t &func);
// void get_operand(const koopa_raw_value_t &t, std::string &operand_str);
void get_operand_load_reg(const koopa_raw_value_t &t, std::string &operand_str);

int get_value_size(const koopa_raw_value_t &value);

extern std::string koopa_ir;
extern std::string riscv_code;
extern int koopa_tmp_id;
extern int now_symbol_table_id;
extern int if_tmp_id;
extern int while_tmp_id;
extern int loop_num;
extern int unreachable_tmp_id;
extern int var_def_id;
extern bool is_global_decl;
extern bool func_arg_is_arr;

class symbol{
 public:
  std::string name; // koopa代码中的变量名(a->a_x)或函数名
  std::string val; // 此符号对应的值(一个数或者一个中间变量名)("x"或"%x")
  bool is_const; // 是否为常量
  bool is_assigned; // 是否已赋值
  bool is_func; // 此符号是否对应一个函数
  bool is_void; // 此函数是否为void型
  bool is_ptr; // 此符号是否对应一个指针
  bool is_arr; // 此符号是否对应一个数组
  int axis; // 数组或指针的维数
  
  symbol(bool i_f,bool i_c_or_i_v,std::string n="",std::string v="",bool i_p=0,bool i_a=0,int ax=0){
    if (!i_f){
      is_func=0;
      is_const=i_c_or_i_v;
      name=n;
      val=v;
      if (val==""){
        val="0";
        is_assigned=0;
      }
      else {
        is_assigned=1;
      }
      is_ptr=i_p;
      is_arr=i_a;
      axis=ax;
    }
    else {
      is_func=1;
      is_void=i_c_or_i_v;
      name=n;
    }
  }
  symbol(){}
};

extern std::vector< std::map< std::string, symbol > > symbol_table;
extern std::map< int, int > loop_label_table;