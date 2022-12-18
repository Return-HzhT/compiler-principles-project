#include "global.h"
#include <string>
#include <map>
#include <vector>

std::string koopa_ir="";
std::string riscv_code="";
int koopa_tmp_id=0; // 变量号(%x)
int now_symbol_table_id=0; // 当前符号表
int if_tmp_id=0; // 用于分支语句的then号,else号和end号
int while_tmp_id=0; // 用于循环语句的while号
int loop_num=0; // 当前循环嵌套层数
int unreachable_tmp_id=0; // 用于不可到达的label号
int var_def_id=0; // 用于变量定义的变量号(name_x)
bool is_global_decl=0; // 是否为全局变量声明
std::vector< std::map< std::string, symbol > > symbol_table; // 符号表
std::map< int, int > loop_label_table; // 当前循环嵌套层数对应的while号

