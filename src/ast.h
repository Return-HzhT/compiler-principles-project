#pragma once

#include <iostream>
#include <cstdlib>
#include <vector>
#include <map>
#include "global.h"
using namespace std;

// 所有 AST 的基类
class BaseAST {
 public:
  int op;
  std::string ir_id;
  virtual ~BaseAST() = default;
  virtual std::string Dump(){return "";}
  virtual std::string get_opt(){return "";}
  virtual void get_rest_of_const_def_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;};
  virtual void get_rest_of_var_def_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;};
  virtual void get_rest_of_block_item_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;};
  virtual void get_rest_of_global_item_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;};
  virtual void get_rest_of_param_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;};
  virtual void get_rest_of_exp_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;};
  virtual std::string get_ir_id(){return ir_id;}
  virtual std::string get_ident(){return "";}
  virtual std::string get_params(){return "";}
  virtual int get_val(){return 0;}
  virtual bool get_have_ret(){return 0;} // 是否为返回语句(如果为分支语句，则全部情况均返回)
  virtual bool is_void(){return 0;}
  virtual void pre_alloc_store(){return;} // 函数提前将输入参数加入符号表
  virtual void get_rest_of_axis_info_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;} // 获取数组各维信息
  virtual void get_rest_of_const_init_val_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;}
  virtual void get_rest_of_init_val_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;}
  virtual void get_rest_of_init_vec(std::vector<int> &vec,std::vector<int> axis_length,int now_axis){return;}
  virtual std::string load(std::string &loaded_id){return "";} // 若目标为指针，则load并更新loaded_id，否则等同于get_ir_id()
};

class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> rest_of_global_item;
  std::vector< std::unique_ptr<BaseAST> > global_item;

  std::string Dump(){
    // 初始化全局符号表
    symbol_table.push_back(std::map< std::string, symbol >());
    symbol_table[now_symbol_table_id]["getint"]=symbol(1,0,"getint");
    symbol_table[now_symbol_table_id]["getch"]=symbol(1,0,"getch");
    symbol_table[now_symbol_table_id]["getarray"]=symbol(1,0,"getarray");
    symbol_table[now_symbol_table_id]["putint"]=symbol(1,1,"putint");
    symbol_table[now_symbol_table_id]["putch"]=symbol(1,1,"putch");
    symbol_table[now_symbol_table_id]["putarray"]=symbol(1,1,"putarray");
    symbol_table[now_symbol_table_id]["starttime"]=symbol(1,1,"starttime");
    symbol_table[now_symbol_table_id]["stoptime"]=symbol(1,1,"stoptime");
    // 声明所有库函数
    koopa_ir+="decl @getint(): i32\n";
    koopa_ir+="decl @getch(): i32\n";
    koopa_ir+="decl @getarray(*i32): i32\n";
    koopa_ir+="decl @putint(i32)\n";
    koopa_ir+="decl @putch(i32)\n";
    koopa_ir+="decl @putarray(i32, *i32)\n";
    koopa_ir+="decl @starttime()\n";
    koopa_ir+="decl @stoptime()\n";

    int n=global_item.size();
    for (int i=0;i<n;i++){
      global_item[i]->Dump();
	  }
    return "";
  }

  void get_global_item_vec(){
    rest_of_global_item->get_rest_of_global_item_vec(global_item);
  }
};

class RestOfGlobalItemAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> global_item;
  std::unique_ptr<BaseAST> rest_of_global_item;

  void get_rest_of_global_item_vec(std::vector< std::unique_ptr<BaseAST> > &vec){
    vec.push_back(std::move(global_item));
    if (rest_of_global_item){
      rest_of_global_item->get_rest_of_global_item_vec(vec);
    }
  }
};

class GlobalItemAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> decl;
  std::unique_ptr<BaseAST> func_def;

  std::string Dump(){
    if (op==1){ // 全局变量声明
      is_global_decl=1;
      decl->Dump();
      is_global_decl=0;
    }
    else if (op==2){ // 函数定义
      func_def->Dump();
    }
    return "";
  }
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> func_f_params;
  std::unique_ptr<BaseAST> block;

  std::string Dump(){
    bool is_void=func_type->is_void();
    symbol_table[now_symbol_table_id][ident]=symbol(1,is_void,ident);
    koopa_ir+="fun @";
    koopa_ir+=ident;
    if (op==1){
      koopa_ir+="()";
    }
    else if (op==2){
      koopa_ir+="(";
      func_f_params->Dump();
      koopa_ir+=")";
    }
    func_type->Dump();
    koopa_ir+=" {\n";
    koopa_ir+="%entry:\n";

    now_symbol_table_id++;
    symbol_table.push_back(std::map< std::string, symbol >());

    // 将函数参数提前alloc和store
    if (op==2){
      func_f_params->pre_alloc_store();
    }
    block->Dump();
      
    symbol_table.pop_back();
    now_symbol_table_id--;

    if (!block->get_have_ret()){
      koopa_ir+="  ret\n";
    }
    koopa_ir+="}\n";
    return "";
  }
};

class FuncTypeAST : public BaseAST {
 public:
  std::string func_type;

  std::string Dump(){
    if (func_type=="int"){
      koopa_ir+=": i32";
    }
    return "";
  }

  bool is_void(){
    if (func_type=="int"){
      return 0;
    }
    return 1;
  }
};

class FuncFParamsAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> first_param;
  std::unique_ptr<BaseAST> rest_of_param;
  std::vector< std::unique_ptr<BaseAST> > param;

  std::string Dump(){
    int n=param.size();
    for (int i=0;i<n;i++){
      param[i]->Dump();
      if (i<n-1){
        koopa_ir+=", ";
      }
	  }
    return "";
  }

  void get_param_vec(){
    param.push_back(std::move(first_param));
    rest_of_param->get_rest_of_param_vec(param);
  }

  void pre_alloc_store(){
    int n=param.size();
    for (int i=0;i<n;i++){
      param[i]->pre_alloc_store();
	  }
  }
};

class RestOfFuncFParamAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> param;
  std::unique_ptr<BaseAST> rest_of_param;
  void get_rest_of_param_vec(std::vector< std::unique_ptr<BaseAST> > &vec){
    vec.push_back(std::move(param));
    if (rest_of_param){
      rest_of_param->get_rest_of_param_vec(vec);
    }
  }
};

class FuncFParamAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> btype;
  std::string ident;
  std::unique_ptr<BaseAST> axis;
  std::vector< unique_ptr<BaseAST> > axis_info;
  std::string axis_str;

  std::string Dump(){
    koopa_ir+="@"+ident+": ";
    if (op==1){
      btype->Dump();
    }
    else if (op==2){
      koopa_ir+="*i32";
    }
    else if (op==3){
      int n=axis_info.size();

      for (int i=n-1;i>=0;i--){
        if (i==n-1){
          axis_str="[i32, "+std::to_string(axis_info[i]->get_val())+"]";
        }
        else {
          axis_str="["+axis_str+", "+std::to_string(axis_info[i]->get_val())+"]";
        }
      }
      koopa_ir+="*"+axis_str;
    }
    return "";
  }

  void pre_alloc_store(){
    std::string name=ident+"_"+std::to_string(var_def_id);
    symbol_table[now_symbol_table_id][ident]=symbol(0,0,name,"0"); // 认为输入参数已被赋值
    if (op==1){
      koopa_ir+="  @"+name+" = alloc i32\n";
      koopa_ir+="  store @"+ident+", @"+name+"\n";
    }
    else if (op==2){
      koopa_ir+="  @"+name+" = alloc *i32\n";
      koopa_ir+="  store @"+ident+", @"+name+"\n";
    }
    else if (op==3){
      koopa_ir+="  @"+name+" = alloc "+axis_str+"\n";
      koopa_ir+="  store @"+ident+", @"+name+"\n";
    }
    var_def_id++;
  }

  void get_axis_info_vec(){
    axis->get_rest_of_axis_info_vec(axis_info);
  }
};

class FuncRParamsAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> first_exp;
  std::unique_ptr<BaseAST> rest_of_exp;
  std::vector< std::unique_ptr<BaseAST> > exp;
  std::string params;

  std::string Dump(){
    int n=exp.size();
    params="";
    std::string tmp_str="";
    for (int i=0;i<n;i++){
      tmp_str+=exp[i]->Dump();
      std::string loaded_id="";
      tmp_str+=exp[i]->load(loaded_id);
      params+=loaded_id;
      if (i<n-1){
        params+=", ";
      }
	  }
    return tmp_str;
  }

  void get_exp_vec(){
    exp.push_back(std::move(first_exp));
    rest_of_exp->get_rest_of_exp_vec(exp);
  }

  std::string get_params(){
    return params;
  }
};

class RestOfFuncRParamAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> rest_of_exp;
  void get_rest_of_exp_vec(std::vector< std::unique_ptr<BaseAST> > &vec){
    vec.push_back(std::move(exp));
    if (rest_of_exp){
      rest_of_exp->get_rest_of_exp_vec(vec);
    }
  }
};

class BlockAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> rest_of_block_item;
  std::vector< std::unique_ptr<BaseAST> > block_item;
  bool have_ret; // 标记block最终是否返回
  bool have_create_symbol_table; // 是否已经创建符号表

  std::string Dump(){
    int n=block_item.size();

    bool flag=0; // 标记是否被返回语句打断
    for (int i=0;i<n;i++){
      block_item[i]->Dump();
      if (block_item[i]->get_have_ret()){
        flag=1;
        break;
      }
    }

    if (flag){
      have_ret=1;
    }
    else {
      have_ret=0;
    }
    return "";
  }

  void get_block_item_vec(){
    rest_of_block_item->get_rest_of_block_item_vec(block_item);
  }

  bool get_have_ret(){
    return have_ret;
  }

};

class RestOfBlockItemAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> block_item;
  std::unique_ptr<BaseAST> rest_of_block_item;

  void get_rest_of_block_item_vec(std::vector< std::unique_ptr<BaseAST> > &vec){
    vec.push_back(std::move(block_item));
    if (rest_of_block_item){
      rest_of_block_item->get_rest_of_block_item_vec(vec);
    }
  }
};

class BlockItemAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> decl;
  std::unique_ptr<BaseAST> stmt;

  std::string Dump(){
    if (op==1){
      decl->Dump();
    }
    else if (op==2){
      stmt->Dump();
    }
    return "";
  }

  bool get_have_ret(){
    if (op==2){
      return stmt->get_have_ret();
    }
    return 0;
  }
};

class StmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> open_stmt;
  std::unique_ptr<BaseAST> closed_stmt;

  std::string Dump(){
    if (op==1){
      open_stmt->Dump();
    }
    else if (op==2){
      closed_stmt->Dump();
    }
    return "";
  }

  bool get_have_ret(){
    if (op==1){
      return 0;
    }
    else if (op==2){
      return closed_stmt->get_have_ret();
    }
    return 0;
  }
};

class OpenStmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> stmt;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  std::unique_ptr<BaseAST> open_stmt;

  std::string Dump(){
    if (op==1){
      koopa_ir+=exp->Dump();
      std::string tmp_id=exp->get_ir_id();
      std::string then_label="%then_"+std::to_string(if_tmp_id);
      std::string end_label="%end_"+std::to_string(if_tmp_id);
      if_tmp_id++;

      koopa_ir+="  br "+tmp_id+", "+then_label+", "+end_label+"\n";

      koopa_ir+=then_label+":\n";
      stmt->Dump();

      if (!stmt->get_have_ret()){
        koopa_ir+="  jump "+end_label+"\n";
      }
      
      koopa_ir+=end_label+":\n";
    }
    else if (op==2){
      bool need_end=0; // 是否需要跳转到end

      koopa_ir+=exp->Dump();
      std::string tmp_id=exp->get_ir_id();
      std::string then_label="%then_"+std::to_string(if_tmp_id);
      std::string else_label="%else_"+std::to_string(if_tmp_id);
      std::string end_label="%end_"+std::to_string(if_tmp_id);
      if_tmp_id++;
      
      koopa_ir+="  br "+tmp_id+", "+then_label+", "+else_label+"\n";

      koopa_ir+=then_label+":\n";
      if_stmt->Dump();
      if (!if_stmt->get_have_ret()){
        koopa_ir+="  jump "+end_label+"\n";
        need_end=1;
      }

      koopa_ir+=else_label+":\n";
      else_stmt->Dump();

      if (!else_stmt->get_have_ret()){
        koopa_ir+="  jump "+end_label+"\n";
        need_end=1;
      }

      if (need_end){
        koopa_ir+=end_label+":\n";
      }
    }
    else if (op==3){
      loop_num++;

      loop_label_table[loop_num]=while_tmp_id;
      std::string while_entry_label="%while_entry_"+std::to_string(while_tmp_id);
      std::string while_body_label="%while_body_"+std::to_string(while_tmp_id);
      std::string while_end_label="%while_end_"+std::to_string(while_tmp_id);
      while_tmp_id++;
      
      koopa_ir+="  jump "+while_entry_label+"\n";
      koopa_ir+=while_entry_label+":\n";

      koopa_ir+=exp->Dump();
      std::string tmp_id=exp->get_ir_id();
      koopa_ir+="  br "+tmp_id+", "+while_body_label+", "+while_end_label+"\n";

      koopa_ir+=while_body_label+":\n";
      open_stmt->Dump();
      if (!open_stmt->get_have_ret()){
        koopa_ir+="  jump "+while_entry_label+"\n";
      }

      koopa_ir+=while_end_label+":\n";

      loop_num--;
    }
    return "";
  }
};

class ClosedStmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> simple_stmt;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  std::unique_ptr<BaseAST> closed_stmt;

  std::string Dump(){
    if (op==1){
      simple_stmt->Dump();
    }
    else if (op==2){
      bool need_end=0; // 是否需要跳转到end

      koopa_ir+=exp->Dump();
      std::string tmp_id=exp->get_ir_id();
      std::string then_label="%then_"+std::to_string(if_tmp_id);
      std::string else_label="%else_"+std::to_string(if_tmp_id);
      std::string end_label="%end_"+std::to_string(if_tmp_id);
      if_tmp_id++;
      koopa_ir+="  br "+tmp_id+", "+then_label+", "+else_label+"\n";

      koopa_ir+=then_label+":\n";
      if_stmt->Dump();
      if (!if_stmt->get_have_ret()){
        koopa_ir+="  jump "+end_label+"\n";
        need_end=1;
      }

      koopa_ir+=else_label+":\n";
      else_stmt->Dump();

      if (!else_stmt->get_have_ret()){
        koopa_ir+="  jump "+end_label+"\n";
        need_end=1;
      }

      if (need_end){
        koopa_ir+=end_label+":\n";
      }
    }
    else if (op==3){
      loop_num++;

      loop_label_table[loop_num]=while_tmp_id;
      std::string while_entry_label="%while_entry_"+std::to_string(while_tmp_id);
      std::string while_body_label="%while_body_"+std::to_string(while_tmp_id);
      std::string while_end_label="%while_end_"+std::to_string(while_tmp_id);
      while_tmp_id++;
      
      koopa_ir+="  jump "+while_entry_label+"\n";
      koopa_ir+=while_entry_label+":\n";

      koopa_ir+=exp->Dump();
      std::string tmp_id=exp->get_ir_id();
      koopa_ir+="  br "+tmp_id+", "+while_body_label+", "+while_end_label+"\n";

      koopa_ir+=while_body_label+":\n";
      closed_stmt->Dump();
      if (!closed_stmt->get_have_ret()){
        koopa_ir+="  jump "+while_entry_label+"\n";
      }

      koopa_ir+=while_end_label+":\n";

      loop_num--;
    }
    return "";
  }

  bool get_have_ret(){
    if (op==1){
      return simple_stmt->get_have_ret();
    }
    else if (op==2){
      return (if_stmt->get_have_ret())&&(else_stmt->get_have_ret());
    }
    return 0;
  }

};

class SimpleStmtAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lval;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> block;

  std::string Dump(){
    if (op==1){
      std::string ident=lval->get_ident();
      bool flag=0; // 是否找到此变量的定义
      for (int i=now_symbol_table_id;i>=0;i--){
        if (symbol_table[i].count(ident)){
          if (symbol_table[i][ident].is_const){
            std::cout<<"error: const value being modified.\n";
            exit(0);
          }
          else {
            koopa_ir+=exp->Dump();
            koopa_ir+=lval->Dump();
            std::string tmp_id="";
            koopa_ir+=exp->load(tmp_id);
            if (lval->op==1){
              symbol_table[i][ident].val=exp->get_ir_id();
              koopa_ir+="  store "+tmp_id+", @"+symbol_table[i][ident].name+"\n";
              symbol_table[i][ident].is_assigned=1;
            }
            else if (lval->op==2){
              koopa_ir+="  store "+tmp_id+", "+lval->get_ir_id()+"\n";
            }
          }
          flag=1;
          break;
        }
      }
      if (!flag){
        std::cout<<"error: undeclared variable.\n";
        exit(0);
      }
    }
    else if (op==2){} // Do nothing
    else if (op==3){
      koopa_ir+=exp->Dump();
    }
    else if (op==4){
      now_symbol_table_id++;
      symbol_table.push_back(std::map< std::string, symbol >());
      
      block->Dump();

      symbol_table.pop_back();
      now_symbol_table_id--;
    }
    else if (op==5){
      koopa_ir+="  ret\n";
    }
    else if (op==6){
      koopa_ir+=exp->Dump();
      std::string ret_val="";
      koopa_ir+=exp->load(ret_val);
      koopa_ir+="  ret "+ret_val+"\n";
    }
    else if (op==7){ // break
      if (!loop_num){
        std::cout<<"error: break not in a loop.\n";
        exit(0);
      }
      std::string end_label="%while_end_"+std::to_string(loop_label_table[loop_num]);
      koopa_ir+="  jump "+end_label+"\n";

      std::string unreachable_label="%unreachable_"+std::to_string(unreachable_tmp_id);
      unreachable_tmp_id++;
      koopa_ir+=unreachable_label+":\n";
    }
    else if (op==8){ // continue
      if (!loop_num){
        std::cout<<"error: continue not in a loop.\n";
        exit(0);
      }
      std::string entry_label="%while_entry_"+std::to_string(loop_label_table[loop_num]);
      koopa_ir+="  jump "+entry_label+"\n";
      
      std::string unreachable_label="%unreachable_"+std::to_string(unreachable_tmp_id);
      unreachable_tmp_id++;
      koopa_ir+=unreachable_label+":\n";
    }
    return "";
  }

  bool get_have_ret(){
    if (op==4){
      return block->get_have_ret();
    }
    else if (op==5||op==6){
      return 1;
    }
    return 0;
  }
};

class ExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lor_exp;

  std::string Dump(){
    std::string tmp_str=lor_exp->Dump();
    ir_id=lor_exp->get_ir_id();
    return tmp_str;
  }

  std::string load(std::string &loaded_id){
    return lor_exp->load(loaded_id);
  }

  int get_val(){
    return lor_exp->get_val();
  }
};

class PrimaryExpAST : public BaseAST {
  public:
   std::unique_ptr<BaseAST> exp;
   std::unique_ptr<BaseAST> lval;
   int number;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=exp->Dump();
      ir_id=exp->get_ir_id();
    }
    else if (op==2){
      tmp_str=lval->Dump();
      ir_id=lval->get_ir_id();
    }
    else if (op==3){
      ir_id=std::to_string(number);
    }
    return tmp_str;
  }

  int get_val(){
    if (op==1){
      return exp->get_val();
    }
    else if (op==2){
      return lval->get_val();
    }
    else if (op==3){
      return number;
    }
    return 0;
  }

  std::string load(std::string &loaded_id){
    std::string tmp_str="";
    if (op==1){
      tmp_str=exp->load(loaded_id);
    }
    else if (op==2){
      tmp_str=lval->load(loaded_id);
    }
    else if (op==3){
      loaded_id=ir_id;
    }
    return tmp_str;
  }
};

class UnaryExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> primary_exp;
  std::unique_ptr<BaseAST> unary_op;
  std::unique_ptr<BaseAST> unary_exp;
  std::string ident;
  std::unique_ptr<BaseAST> func_r_params;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=primary_exp->Dump();
      ir_id=primary_exp->get_ir_id();
    }
    else if (op==2){
      tmp_str+=unary_exp->Dump();
      string opt=unary_op->get_opt();

      if (opt=="-"){
        std::string tmp_id="";
        tmp_str+=unary_exp->load(tmp_id);
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        tmp_str+="  "+ir_id+" = sub 0, "+tmp_id+"\n";
      }
      else if (opt=="!"){
        std::string tmp_id="";
        tmp_str+=unary_exp->load(tmp_id);
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        tmp_str+="  "+ir_id+" = eq ";
        tmp_str+=tmp_id;
        tmp_str+=", 0\n";
      }
      else {
        ir_id=unary_exp->get_ir_id();
      }
    }
    else if (op==3){
      if (symbol_table[0].count(ident)){
        if (!symbol_table[0][ident].is_func){
          std::cout<<"error: undefined function: "+ident+" .\n";
          exit(0);
        }
        if (symbol_table[0][ident].is_void){
          tmp_str+="  call @"+ident+"()\n";
        }
        else {
          ir_id="%"+std::to_string(koopa_tmp_id);
          koopa_tmp_id++;
          tmp_str+="  "+ir_id+" = call @"+ident+"()\n";
        }
      }
      else {
        std::cout<<"error: undefined function: "+ident+" .\n";
        exit(0);
      }
    }
    else if (op==4){
      if (symbol_table[0].count(ident)){
        if (!symbol_table[0][ident].is_func){
          std::cout<<"error: undefined function: "+ident+" .\n";
          exit(0);
        }
        if (symbol_table[0][ident].is_void){
          tmp_str+=func_r_params->Dump();
          std::string params=func_r_params->get_params();
          tmp_str+="  call @"+ident+"("+params+")\n";
        }
        else {
          ir_id="%"+std::to_string(koopa_tmp_id);
          koopa_tmp_id++;
          tmp_str+=func_r_params->Dump();
          std::string params=func_r_params->get_params();
          tmp_str+="  "+ir_id+" = call @"+ident+"("+params+")\n";
        }
      }
      else {
        std::cout<<"error: undefined function: "+ident+" .\n";
        exit(0);
      }
    }
    return tmp_str;
  }

  int get_val(){
    if (op==1){
      return primary_exp->get_val();
    }
    else if (op==2){
      string opt=unary_op->get_opt();
      if (opt=="-"){
        return -(unary_exp->get_val());
      }
      else if (opt=="!"){
        return !(unary_exp->get_val());
      }
      else {
        return unary_exp->get_val();
      }
    }
    return 0;
  }

  std::string load(std::string &loaded_id){
    std::string tmp_str="";
    if (op==1){
      tmp_str=primary_exp->load(loaded_id);
    }
    else {
      loaded_id=ir_id;
    }
    return tmp_str;
  }
};

class UnaryOpAST : public BaseAST {
 public:
  std::string opt;

  string get_opt(){
    return opt;
  }
};

class AddExpAST : public BaseAST {
 public:
  std::string add_op;
  std::unique_ptr<BaseAST> add_exp;
  std::unique_ptr<BaseAST> mul_exp;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=mul_exp->Dump();
      ir_id=mul_exp->get_ir_id();
    }
    else if (op==2){
      tmp_str+=mul_exp->Dump();
      tmp_str+=add_exp->Dump();
      std::string mul_id="",add_id="";
      tmp_str+=mul_exp->load(mul_id);
      tmp_str+=add_exp->load(add_id);
      
      ir_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;

      if (add_op=="+"){
        tmp_str+="  "+ir_id+" = add "+add_id+", "+mul_id+"\n";
      }
      else if (add_op=="-"){
        tmp_str+="  "+ir_id+" = sub "+add_id+", "+mul_id+"\n";
      }
    }
    return tmp_str;
  }

  int get_val(){
    if (op==1){
      return mul_exp->get_val();
    }
    else if (op==2){
      if (add_op=="+"){
        return (add_exp->get_val())+(mul_exp->get_val());
      }
      else if (add_op=="-"){
        return (add_exp->get_val())-(mul_exp->get_val());
      }
    }
    return 0;
  }

  std::string load(std::string &loaded_id){
    std::string tmp_str="";
    if (op==1){
      tmp_str=mul_exp->load(loaded_id);
    }
    else if (op==2){
      loaded_id=ir_id;
    }
    return tmp_str;
  }
};

class MulExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> mul_exp;
  std::string mul_op;
  std::unique_ptr<BaseAST> unary_exp;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=unary_exp->Dump();
      ir_id=unary_exp->get_ir_id();
    }
    else if (op==2){
      tmp_str+=unary_exp->Dump();
      tmp_str+=mul_exp->Dump();
      std::string unary_id="",mul_id="";
      tmp_str+=unary_exp->load(unary_id);
      tmp_str+=mul_exp->load(mul_id);
      
      ir_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      
      if (mul_op=="*"){
        tmp_str+="  "+ir_id+" = mul "+mul_id+", "+unary_id+"\n";
      }
      else if (mul_op=="/"){
        tmp_str+="  "+ir_id+" = div "+mul_id+", "+unary_id+"\n";
      }
      else if (mul_op=="%"){
        tmp_str+="  "+ir_id+" = mod "+mul_id+", "+unary_id+"\n";
      }
    }    
    return tmp_str;
  }

  int get_val(){
    if (op==1){
      return unary_exp->get_val();
    }
    else if (op==2){
      if (mul_op=="*"){
        return (mul_exp->get_val())*(unary_exp->get_val());
      }
      else if (mul_op=="/"){
        return (mul_exp->get_val())/(unary_exp->get_val());
      }
      else if (mul_op=="%"){
        return (mul_exp->get_val())%(unary_exp->get_val());
      }
    }
    return 0;
  }

  std::string load(std::string &loaded_id){
    std::string tmp_str="";
    if (op==1){
      tmp_str=unary_exp->load(loaded_id);
    }
    else if (op==2){
      loaded_id=ir_id;
    }
    return tmp_str;
  }
};

class LOrExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> lor_exp;
  std::string lor_op;
  std::unique_ptr<BaseAST> land_exp;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=land_exp->Dump();
      ir_id=land_exp->get_ir_id();
    }
    else if (op==2){
      std::string res_var="result_"+std::to_string(if_tmp_id);
      std::string then_label="%then_"+std::to_string(if_tmp_id);
      std::string end_label="%end_"+std::to_string(if_tmp_id);
      if_tmp_id++;

      symbol_table[now_symbol_table_id][res_var]=symbol(0,0,"1","1");
      tmp_str+="  @"+res_var+" = alloc i32\n";
      tmp_str+="  store "+std::to_string(1)+", @"+res_var+"\n";

      tmp_str+=lor_exp->Dump();
      std::string lor_id="";
      tmp_str+=lor_exp->load(lor_id);
      tmp_str+="  br "+lor_id+", "+end_label+", "+then_label+"\n";
      tmp_str+=then_label+":\n";

      tmp_str+=land_exp->Dump();
      std::string land_id="";
      tmp_str+=land_exp->load(land_id);
      std::string tmp_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      tmp_str+="  "+tmp_id+" = ne "+land_id+", 0"+"\n";
      tmp_str+="  store "+tmp_id+", @"+res_var+"\n";
      tmp_str+="  jump "+end_label+"\n";

      tmp_str+=end_label+":\n";

      ir_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      tmp_str+="  "+ir_id+" = load @"+res_var+"\n";
    }
    return tmp_str;
  }

  int get_val(){
    if (op==1){
      return land_exp->get_val();
    }
    else if (op==2){
      return (lor_exp->get_val())||(land_exp->get_val());
    }
    return 0;
  }

  std::string load(std::string &loaded_id){
    std::string tmp_str="";
    if (op==1){
      tmp_str=land_exp->load(loaded_id);
    }
    else if (op==2){
      loaded_id=ir_id;
    }
    return tmp_str;
  }
};

class LAndExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> land_exp;
  std::string land_op;
  std::unique_ptr<BaseAST> eq_exp;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=eq_exp->Dump();
      ir_id=eq_exp->get_ir_id();
    }
    else if (op==2){
      std::string res_var="result_"+std::to_string(if_tmp_id);
      std::string then_label="%then_"+std::to_string(if_tmp_id);
      std::string end_label="%end_"+std::to_string(if_tmp_id);
      if_tmp_id++;

      symbol_table[now_symbol_table_id][res_var]=symbol(0,0,"0","0");
      tmp_str+="  @"+res_var+" = alloc i32\n";
      tmp_str+="  store "+std::to_string(0)+", @"+res_var+"\n";

      tmp_str+=land_exp->Dump();
      std::string land_id="";
      tmp_str+=land_exp->load(land_id);
      tmp_str+="  br "+land_id+", "+then_label+", "+end_label+"\n";
      tmp_str+=then_label+":\n";

      tmp_str+=eq_exp->Dump();
      std::string eq_id="";
      tmp_str+=eq_exp->load(eq_id);
      std::string tmp_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      tmp_str+="  "+tmp_id+" = ne "+eq_id+", 0"+"\n";
      tmp_str+="  store "+tmp_id+", "+"  @"+res_var+"\n";
      tmp_str+="  jump "+end_label+"\n";

      tmp_str+=end_label+":\n";

      ir_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      tmp_str+="  "+ir_id+" = load @"+res_var+"\n";
    }    
    return tmp_str;
  }

  int get_val(){
    if (op==1){
      return eq_exp->get_val();
    }
    else if (op==2){
      return (land_exp->get_val())&&(eq_exp->get_val());
    }
    return 0;
  }

  std::string load(std::string &loaded_id){
    std::string tmp_str="";
    if (op==1){
      tmp_str=eq_exp->load(loaded_id);
    }
    else if (op==2){
      loaded_id=ir_id;
    }
    return tmp_str;
  }
};

class EqExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> eq_exp;
  std::string eq_op;
  std::unique_ptr<BaseAST> rel_exp;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=rel_exp->Dump();
      ir_id=rel_exp->get_ir_id();
    }
    else if (op==2){
      tmp_str+=eq_exp->Dump();
      tmp_str+=rel_exp->Dump();
      std::string eq_id="",rel_id="";
      tmp_str+=eq_exp->load(eq_id);
      tmp_str+=rel_exp->load(rel_id);

      ir_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;

      if (eq_op=="=="){
        tmp_str+="  "+ir_id+" = eq "+eq_id+", "+rel_id+"\n";
      }
      else if (eq_op=="!="){
        tmp_str+="  "+ir_id+" = ne "+eq_id+", "+rel_id+"\n";
      }
    }
    return tmp_str;
  }

  int get_val(){
    if (op==1){
      return rel_exp->get_val();
    }
    else if (op==2){
      if (eq_op=="=="){
        return (eq_exp->get_val())==(rel_exp->get_val());
      }
      else if (eq_op=="!="){
        return (eq_exp->get_val())!=(rel_exp->get_val());
      }
    }
    return 0;
  }

  std::string load(std::string &loaded_id){
    std::string tmp_str="";
    if (op==1){
      tmp_str=rel_exp->load(loaded_id);
    }
    else if (op==2){
      loaded_id=ir_id;
    }
    return tmp_str;
  }
};

class RelExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> rel_exp;
  std::string rel_op;
  std::unique_ptr<BaseAST> add_exp;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=add_exp->Dump();
      ir_id=add_exp->get_ir_id();
    }
    else if (op==2){
      tmp_str+=rel_exp->Dump();
      tmp_str+=add_exp->Dump();
      std::string rel_id="",add_id="";
      tmp_str+=rel_exp->load(rel_id);
      tmp_str+=add_exp->load(add_id);
      
      ir_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;

      if (rel_op=="<"){
        tmp_str+="  "+ir_id+" = lt "+rel_id+", "+add_id+"\n";
      }
      else if (rel_op==">"){
        tmp_str+="  "+ir_id+" = gt "+rel_id+", "+add_id+"\n";
      }
      else if (rel_op=="<="){
        tmp_str+="  "+ir_id+" = le "+rel_id+", "+add_id+"\n";
      }
      else if (rel_op==">="){
        tmp_str+="  "+ir_id+" = ge "+rel_id+", "+add_id+"\n";
      }
    }
    return tmp_str;
  }

  int get_val(){
    if (op==1){
      return add_exp->get_val();
    }
    else if (op==2){
      if (rel_op=="<"){
        return (rel_exp->get_val())<(add_exp->get_val());
      }
      else if (rel_op==">"){
        return (rel_exp->get_val())>(add_exp->get_val());
      }
      else if (rel_op=="<="){
        return (rel_exp->get_val())<=(add_exp->get_val());
      }
      else if (rel_op==">="){
        return (rel_exp->get_val())>=(add_exp->get_val());
      }
    }
    return 0;
  }

  std::string load(std::string &loaded_id){
    std::string tmp_str="";
    if (op==1){
      tmp_str=add_exp->load(loaded_id);
    }
    else if (op==2){
      loaded_id=ir_id;
    }
    return tmp_str;
  }
};

class DeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_decl;
  std::unique_ptr<BaseAST> var_decl;

  std::string Dump(){
    if (op==1){
      const_decl->Dump();
    }
    else if (op==2){
      var_decl->Dump();
    }
    return "";
  }
};

class ConstDeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> btype;
  std::unique_ptr<BaseAST> first_const_def;
  std::unique_ptr<BaseAST> rest_of_const_def;
  std::vector< std::unique_ptr<BaseAST> > const_def;

  std::string Dump(){
    int n=const_def.size();
    for (int i=0;i<n;i++){
      const_def[i]->Dump();
	  }
    return "";
  }

  void get_const_def_vec(){
    const_def.push_back(std::move(first_const_def));
    rest_of_const_def->get_rest_of_const_def_vec(const_def);
  }
};

class VarDeclAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> btype;
  std::unique_ptr<BaseAST> first_var_def;
  std::unique_ptr<BaseAST> rest_of_var_def;
  std::vector< std::unique_ptr<BaseAST> > var_def;

  std::string Dump(){
    int n=var_def.size();
    for (int i=0;i<n;i++){
      var_def[i]->Dump();
	  }
    return "";
  }

  void get_var_def_vec(){
    var_def.push_back(std::move(first_var_def));
    rest_of_var_def->get_rest_of_var_def_vec(var_def);
  }
};

class RestOfConstDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_def;
  std::unique_ptr<BaseAST> rest_of_const_def;
  void get_rest_of_const_def_vec(std::vector< std::unique_ptr<BaseAST> > &vec){
    vec.push_back(std::move(const_def));
    if (rest_of_const_def){
      rest_of_const_def->get_rest_of_const_def_vec(vec);
    }
  }
};

class RestOfVarDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> var_def;
  std::unique_ptr<BaseAST> rest_of_var_def;
  void get_rest_of_var_def_vec(std::vector< std::unique_ptr<BaseAST> > &vec){
    vec.push_back(std::move(var_def));
    if (rest_of_var_def){
      rest_of_var_def->get_rest_of_var_def_vec(vec);
    }
  }
};


class BTypeAST : public BaseAST {
 public:
  std::string btype;

  std::string Dump(){
    if (btype=="int"){
      koopa_ir+="i32";
    }
    return "";
  }
};

class ConstDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> axis;
  std::unique_ptr<BaseAST> const_init_val;
  std::vector< unique_ptr<BaseAST> > axis_info;
  std::vector<int> init_vec;

  std::string Dump(){
    if (symbol_table[now_symbol_table_id].count(ident)){
      std::cout<<"error: redefined.\n";
      exit(0);
    }
    if (op==1){
      if (is_global_decl){
        const_init_val->Dump();
      }
      else {
        koopa_ir+=const_init_val->Dump();
      }
    
      std::string val=const_init_val->get_ir_id();
      int digit_val=const_init_val->get_val();
      symbol_table[now_symbol_table_id][ident]=symbol(0,1,val,std::to_string(digit_val));
    }
    else if (op==2){
      for (int i=0;i<axis_info.size();i++){
        axis_info[i]->Dump();
      }
      const_init_val->Dump();

      std::string name=ident+"_"+std::to_string(var_def_id);
      symbol_table[now_symbol_table_id][ident]=symbol(0,0,name);
      symbol_table[now_symbol_table_id][ident].is_arr=1;

      int n=axis_info.size();
      std::string axis_str="";

      for (int i=n-1;i>=0;i--){
        if (i==n-1){
          axis_str="[i32, "+std::to_string(axis_info[i]->get_val())+"]";
        }
        else {
          axis_str="["+axis_str+", "+std::to_string(axis_info[i]->get_val())+"]";
        }
      }

      get_init_vec();
      // std::cout<<init_vec.size()<<"\n";
      // for (int i=0;i<init_vec.size();i++){
      //   std::cout<<init_vec[i]<<" ";
      // }
      // std::cout<<"\n";

      if (is_global_decl){
        koopa_ir+="global @"+name+" = alloc "+axis_str;
        koopa_ir+=", {";
        global_arr_init(0,0);
        koopa_ir+="}\n";
      }
      else {
        koopa_ir+="  @"+name+" = alloc "+axis_str+"\n";
        arr_init("@"+name,0,0);
      }

      var_def_id++;
    }

    return "";
  }

    void global_arr_init(int now_axis,int cnt){
    if (now_axis==axis_info.size()-1){
      for (int i=0;i<axis_info[now_axis]->get_val();i++){
        if (i==0){
          koopa_ir+=std::to_string(init_vec[cnt+i]);
        }
        else {
          koopa_ir+=", "+std::to_string(init_vec[cnt+i]);
        }
      }
      return;
    }
    
    int axis_sum=1; // 当前维度总长度
    for (int i=now_axis+1;i<axis_info.size();i++){
      axis_sum*=axis_info[i]->get_val();
    }
    for (int i=0;i<axis_info[now_axis]->get_val();i++){
      if (i==0){
        koopa_ir+="{";
      }
      else {
        koopa_ir+=", {";
      }
      global_arr_init(now_axis+1,cnt+i*axis_sum);
      koopa_ir+="}";
    }
  }

  void arr_init(std::string pre_ptr,int now_axis,int cnt){
    int axis_sum=1; // 当前维度总长度
    for (int i=now_axis+1;i<axis_info.size();i++){
      axis_sum*=axis_info[i]->get_val();
    }

    for (int i=0;i<axis_info[now_axis]->get_val();i++){
      std::string tmp_ptr_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      koopa_ir+="  "+tmp_ptr_id+" = getelemptr "+pre_ptr+", "+std::to_string(i)+"\n";

      if (now_axis==axis_info.size()-1){
        koopa_ir+="  store "+std::to_string(init_vec[cnt+i])+", "+tmp_ptr_id+"\n";
      }
      else {
        arr_init(tmp_ptr_id,now_axis+1,cnt+i*axis_sum);
      }

    }
  }

  void get_init_vec(){
    std::vector<int> axis_length=std::vector<int>();
    for (int i=0;i<axis_info.size();i++){
      axis_length.push_back(axis_info[i]->get_val());
    }
    const_init_val->get_rest_of_init_vec(init_vec,axis_length,0);
  }

  void get_axis_info_vec(){
    axis->get_rest_of_axis_info_vec(axis_info);
  }
};

class AxisAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_exp;
  std::unique_ptr<BaseAST> axis;

  void get_rest_of_axis_info_vec(std::vector< std::unique_ptr<BaseAST> > &vec){
    vec.push_back(std::move(const_exp));
    if (axis!=NULL){
      axis->get_rest_of_axis_info_vec(vec);
    }
  }
};

class VarDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> init_val;
  std::unique_ptr<BaseAST> axis;
  std::vector< unique_ptr<BaseAST> > axis_info;
  std::vector<int> init_vec;

  std::string Dump(){
    if (symbol_table[now_symbol_table_id].count(ident)){
      std::cout<<"error: redefined.\n";
      exit(0);
    }
    if (op==1){
      std::string name=ident+"_"+std::to_string(var_def_id);
      symbol_table[now_symbol_table_id][ident]=symbol(0,0,name);
      if (is_global_decl){
        koopa_ir+="global";
      }
      koopa_ir+="  @"+name+" = alloc i32";
      if (is_global_decl){
        koopa_ir+=", zeroinit";
        symbol_table[now_symbol_table_id][ident].val="0";
        symbol_table[now_symbol_table_id][ident].is_assigned=1;
      }
      koopa_ir+="\n";
      var_def_id++;
    }
    else if (op==2){
      koopa_ir+=init_val->Dump();
      std::string val=init_val->get_ir_id();
      std::string name=ident+"_"+std::to_string(var_def_id);
      symbol_table[now_symbol_table_id][ident]=symbol(0,0,name,val);

      if (is_global_decl){
        koopa_ir+="global";
      }
      koopa_ir+="  @"+name+" = alloc i32";
      if (is_global_decl){
        koopa_ir+=", "+val+"\n";
      }
      else {
        koopa_ir+="\n  store "+val+", @"+name+"\n";
      }
      var_def_id++;
    }
    else if (op==3){
      for (int i=0;i<axis_info.size();i++){
        axis_info[i]->Dump();
      }

      std::string name=ident+"_"+std::to_string(var_def_id);
      symbol_table[now_symbol_table_id][ident]=symbol(0,0,name);
      symbol_table[now_symbol_table_id][ident].is_arr=1;

      std::string axis_str="";
      int n=axis_info.size();
      for (int i=n-1;i>=0;i--){
        if (i==n-1){
          axis_str="[i32, "+std::to_string(axis_info[i]->get_val())+"]";
        }
        else {
          axis_str="["+axis_str+", "+std::to_string(axis_info[i]->get_val())+"]";
        }
      }

      if (is_global_decl){
        koopa_ir+="global @"+name+" = alloc "+axis_str+", zeroinit\n";
      }
      else {
        koopa_ir+="  @"+name+" = alloc "+axis_str+"\n";
      }
      var_def_id++;
    }
    else if (op==4){
      for (int i=0;i<axis_info.size();i++){
        axis_info[i]->Dump();
      }
      if (is_global_decl){
        init_val->Dump();
      }
      else {
        koopa_ir+=init_val->Dump();
      }

      std::string name=ident+"_"+std::to_string(var_def_id);
      symbol_table[now_symbol_table_id][ident]=symbol(0,0,name);
      symbol_table[now_symbol_table_id][ident].is_arr=1;
      
      int n=axis_info.size();
      std::string axis_str="";

      for (int i=n-1;i>=0;i--){
        if (i==n-1){
          axis_str="[i32, "+std::to_string(axis_info[i]->get_val())+"]";
        }
        else {
          axis_str="["+axis_str+", "+std::to_string(axis_info[i]->get_val())+"]";
        }
      }

      get_init_vec();
      // std::cout<<init_vec.size()<<"\n";
      // for (int i=0;i<init_vec.size();i++){
      //   std::cout<<init_vec[i]<<" ";
      // }
      // std::cout<<"\n";

      if (is_global_decl){
        koopa_ir+="global @"+name+" = alloc "+axis_str;
        koopa_ir+=", {";
        global_arr_init(0,0);
        koopa_ir+="}\n";
      }
      else {
        koopa_ir+="  @"+name+" = alloc "+axis_str+"\n";
        arr_init("@"+name,0,0);
      }

      var_def_id++;
    }
    return "";
  }

  void global_arr_init(int now_axis,int cnt){
    if (now_axis==axis_info.size()-1){
      for (int i=0;i<axis_info[now_axis]->get_val();i++){
        if (i==0){
          koopa_ir+=std::to_string(init_vec[cnt+i]);
        }
        else {
          koopa_ir+=", "+std::to_string(init_vec[cnt+i]);
        }
      }
      return;
    }
    
    int axis_sum=1; // 当前维度总长度
    for (int i=now_axis+1;i<axis_info.size();i++){
      axis_sum*=axis_info[i]->get_val();
    }
    for (int i=0;i<axis_info[now_axis]->get_val();i++){
      if (i==0){
        koopa_ir+="{";
      }
      else {
        koopa_ir+=", {";
      }
      global_arr_init(now_axis+1,cnt+i*axis_sum);
      koopa_ir+="}";
    }
  }

  void arr_init(std::string pre_ptr,int now_axis,int cnt){
    int axis_sum=1; // 当前维度总长度
    for (int i=now_axis+1;i<axis_info.size();i++){
      axis_sum*=axis_info[i]->get_val();
    }

    for (int i=0;i<axis_info[now_axis]->get_val();i++){
      std::string tmp_ptr_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      koopa_ir+="  "+tmp_ptr_id+" = getelemptr "+pre_ptr+", "+std::to_string(i)+"\n";
      if (now_axis==axis_info.size()-1){
        koopa_ir+="  store "+std::to_string(init_vec[cnt+i])+", "+tmp_ptr_id+"\n";
      }
      else {
        arr_init(tmp_ptr_id,now_axis+1,cnt+i*axis_sum);
      }
    }
  }

  void get_init_vec(){
    std::vector<int> axis_length=std::vector<int>();
    for (int i=0;i<axis_info.size();i++){
      axis_length.push_back(axis_info[i]->get_val());
    }
    init_val->get_rest_of_init_vec(init_vec,axis_length,0);
  }

  void get_axis_info_vec(){
    axis->get_rest_of_axis_info_vec(axis_info);
  }
};

class ConstInitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_exp;
  std::unique_ptr<BaseAST> const_init_val;
  std::unique_ptr<BaseAST> rest_of_const_init_val;
  std::vector< std::unique_ptr<BaseAST> > const_init_val_vec;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=const_exp->Dump();
    }
    else if (op==2){}
    else if (op==3){
      for (int i=0;i<const_init_val_vec.size();i++){
        tmp_str+=const_init_val_vec[i]->Dump();
      }
    }
    return tmp_str;
  }

  std::string get_ir_id(){
    return const_exp->get_ir_id();
  }

  int get_val(){
    return const_exp->get_val();
  }

  void get_const_init_val_vec(){
    const_init_val_vec.push_back(std::move(const_init_val));
    rest_of_const_init_val->get_rest_of_const_init_val_vec(const_init_val_vec);
  }

  virtual void get_rest_of_init_vec(std::vector<int> &vec,std::vector<int> axis_length,int now_axis){
    if (op==1){
      vec.push_back(const_exp->get_val());
    }
    else if (op==2){
      int tot=1;
      for (int i=now_axis;i<axis_length.size();i++){
        tot*=axis_length[i];
      }
      for (int i=0;i<tot;i++){
        vec.push_back(0);
      }
    }
    else if (op==3){
      for (int i=0;i<const_init_val_vec.size();i++){
        int now_tot=vec.size(),tmp_axis=now_axis+1;
        for (int j=axis_length.size()-1;j>now_axis;j--){
          if (now_tot%axis_length[j]==0){
            now_tot/=axis_length[j];
          }
          else {
            tmp_axis=j;
            break;
          }
        }
        const_init_val_vec[i]->get_rest_of_init_vec(vec,axis_length,tmp_axis);
      }
      
      int tot=1;
      for (int i=now_axis;i<axis_length.size();i++){
        tot*=axis_length[i];
      }
      int rest=vec.size()%tot;
      if (rest){
        for (int i=rest;i<tot;i++){
          vec.push_back(0);
        }
      }
    }
  }
};


class RestOfConstInitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_init_val;
  std::unique_ptr<BaseAST> rest_of_const_init_val;

  void get_rest_of_const_init_val_vec(std::vector< std::unique_ptr<BaseAST> > &vec){
    if (op==1){
      vec.push_back(std::move(const_init_val));
      if (rest_of_const_init_val){
        rest_of_const_init_val->get_rest_of_const_init_val_vec(vec);
      }
    }
  }
};

class InitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> init_val;
  std::unique_ptr<BaseAST> rest_of_init_val;
  std::vector< std::unique_ptr<BaseAST> > init_val_vec;
  
  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      tmp_str=exp->Dump();
    }
    else if (op==2){}
    else if (op==3){
      for (int i=0;i<init_val_vec.size();i++){
        tmp_str+=init_val_vec[i]->Dump();
      }
    }
    return tmp_str;
  }

  std::string get_ir_id(){
    return exp->get_ir_id();
  }

  int get_val(){
    return exp->get_val();
  }

  void get_init_val_vec(){
    init_val_vec.push_back(std::move(init_val));
    rest_of_init_val->get_rest_of_init_val_vec(init_val_vec);
  }

  virtual void get_rest_of_init_vec(std::vector<int> &vec,std::vector<int> axis_length,int now_axis){
    if (op==1){
      vec.push_back(exp->get_val());
    }
    else if (op==2){
      int tot=1;
      for (int i=now_axis;i<axis_length.size();i++){
        tot*=axis_length[i];
      }
      for (int i=0;i<tot;i++){
        vec.push_back(0);
      }
    }
    else if (op==3){
      for (int i=0;i<init_val_vec.size();i++){
        int now_tot=vec.size(),tmp_axis=now_axis+1;
        for (int j=axis_length.size()-1;j>now_axis;j--){
          if (now_tot%axis_length[j]==0){
            now_tot/=axis_length[j];
          }
          else {
            tmp_axis=j;
            break;
          }
        }
        init_val_vec[i]->get_rest_of_init_vec(vec,axis_length,tmp_axis);
      }
      
      int tot=1;
      for (int i=now_axis;i<axis_length.size();i++){
        tot*=axis_length[i];
      }
      int rest=vec.size()%tot;
      if (rest){
        for (int i=rest;i<tot;i++){
          vec.push_back(0);
        }
      }
    }
  }
};

class RestOfInitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> init_val;
  std::unique_ptr<BaseAST> rest_of_init_val;

  void get_rest_of_init_val_vec(std::vector< std::unique_ptr<BaseAST> > &vec){
    if (op==1){
      vec.push_back(std::move(init_val));
      if (rest_of_init_val){
        rest_of_init_val->get_rest_of_init_val_vec(vec);
      }
    }
  }
};

class ConstExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;

  std::string Dump(){
    return exp->Dump();
  }

  std::string get_ir_id(){
    return exp->get_ir_id();
  }

  int get_val(){
    return exp->get_val();
  }
};

class LValAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> axis;
  std::vector< unique_ptr<BaseAST> > axis_info;

  std::string Dump(){
    std::string tmp_str="";
    if (op==1){
      for (int i=now_symbol_table_id;i>=0;i--){
        if (symbol_table[i].count(ident)){
          if (symbol_table[i][ident].is_const){
            ir_id=symbol_table[i][ident].val;
            return "";
          }
          else {
            ir_id="@"+symbol_table[i][ident].name;
            return "";
          }
        }
      }
    }
    else if (op==2){
      for (int i=now_symbol_table_id;i>=0;i--){
        if (symbol_table[i].count(ident)){
          for (int i=0;i<axis_info.size();i++){
            tmp_str+=axis_info[i]->Dump();
          }

          std::string now_ptr_id="@"+symbol_table[i][ident].name;
          for (int i=0;i<axis_info.size();i++){
            std::string next_ptr_id="%"+std::to_string(koopa_tmp_id);
            koopa_tmp_id++;
            tmp_str+="  "+next_ptr_id+" = getelemptr "+now_ptr_id+", "+axis_info[i]->get_ir_id()+"\n";
            now_ptr_id=next_ptr_id;
          }
          ir_id=now_ptr_id;
          break;
        }
      }
    }
    return tmp_str;
  }

  std::string load(std::string &loaded_id){
    std::string tmp_str="";
    if (op==1){
      for (int i=now_symbol_table_id;i>=0;i--){
        if (symbol_table[i].count(ident)){
          if (symbol_table[i][ident].is_const){
            loaded_id=ir_id;
            return "";
          }
          else {
            loaded_id="%"+std::to_string(koopa_tmp_id);
            koopa_tmp_id++;

            tmp_str+="  "+loaded_id+" = load "+ir_id+"\n";
            return tmp_str;
          }
        }
      }
    }
    else if (op==2){
      loaded_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;

      tmp_str+="  "+loaded_id+" = load "+ir_id+"\n";
    }
    return tmp_str;
  }

  std::string get_ident(){
    return ident;
  }

  int get_val(){
    for (int i=now_symbol_table_id;i>=0;i--){
      if (symbol_table[i].count(ident)){
        if (symbol_table[i][ident].is_assigned){
          int val=stoi(symbol_table[i][ident].val);
          return val;
        }
        else {
          std::cout<<"error: unassigned variable.\n";
          exit(0);
        }
      }
    }
    
    // 运行到这里说明未找到此变量的定义
    std::cout<<"error: undeclared variable.\n";
    exit(0);
  }

  void get_axis_info_vec(){
    axis->get_rest_of_axis_info_vec(axis_info);
  }
};