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
  virtual ~BaseAST() = default;
  virtual void Dump_ast(){return;}
  virtual std::string Dump(){return "";}
  virtual std::string get_op(){return "";}
  virtual void get_rest_of_const_def_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;};
  virtual void get_rest_of_var_def_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;};
  virtual void get_rest_of_block_item_vec(std::vector< std::unique_ptr<BaseAST> > &vec){return;};
  virtual int get_val(){return 0;}
  virtual std::string get_ident(){return "";}
  virtual bool get_have_ret(){return 0;} // 是否为返回语句(如果为分支语句，则全部情况均返回)
};

class CompUnitAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_def;

  void Dump_ast(){
    std::cout << "CompUnitAST { ";
    func_def->Dump_ast();
    std::cout << " }"<<endl;
  }

  std::string Dump(){
    func_def->Dump();
    return "";
  }
};

class FuncDefAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> func_type;
  std::string ident;
  std::unique_ptr<BaseAST> block;

  void Dump_ast(){
    std::cout << "FuncDefAST { ";
    func_type->Dump_ast();
    std::cout << ", " << ident << ", ";
    block->Dump_ast();
    std::cout << " }\n";
  }

  std::string Dump(){
    koopa_ir+="fun @";
    koopa_ir+=ident;
    koopa_ir+="(): ";
    func_type->Dump();
    koopa_ir+=" {\n";
    koopa_ir+="%entry:\n";
    block->Dump();
    koopa_ir+="}\n";
    return "";
  }
};

class FuncTypeAST : public BaseAST {
 public:
  std::string func_type;

  void Dump_ast(){
    std::cout << "FuncTypeAST { ";
    std::cout << func_type;
    std::cout << " }";
  }

  std::string Dump(){
    if (func_type=="int"){
        koopa_ir+="i32";
    }
    return "";
  }
};

class BlockAST : public BaseAST {
 public:
  int op;
  std::unique_ptr<BaseAST> rest_of_block_item;
  std::vector< std::unique_ptr<BaseAST> > block_item;
  bool have_ret; // 标记block最终是否返回

  void Dump_ast(){
    std::cout << "BlockAST {";
    int n=block_item.size();
    for (int i=0;i<n;i++){
      block_item[i]->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    now_symbol_table_id++;
    symbol_table.push_back(std::map< std::string, symbol >());
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

    symbol_table.pop_back();
    now_symbol_table_id--;
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
  int op;
  std::unique_ptr<BaseAST> decl;
  std::unique_ptr<BaseAST> stmt;

  void Dump_ast(){
    std::cout << " BlockItemAST { ";
    if (op==1){
      decl->Dump_ast();
    }
    else if (op==2){
      stmt->Dump_ast();
    }
    std::cout << " }";
  }

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
  int op;
  std::unique_ptr<BaseAST> open_stmt;
  std::unique_ptr<BaseAST> closed_stmt;

  void Dump_ast(){
    std::cout << "StmtAST { ";
    if (op==1){
      open_stmt->Dump_ast();
    }
    else if (op==2){
      closed_stmt->Dump_ast();
    }
    std::cout << " }";
  }

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
  int op;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> stmt;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  std::unique_ptr<BaseAST> open_stmt;

  void Dump_ast(){
    std::cout << "OpenStmtAST { ";
    if (op==1){
      exp->Dump_ast();
      stmt->Dump_ast();
    }
    else if (op==2){
      exp->Dump_ast();
      if_stmt->Dump_ast();
      else_stmt->Dump_ast();
    }
    else if (op==3){
      exp->Dump_ast();
      open_stmt->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      std::string tmp_id=exp->Dump();
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

      std::string tmp_id=exp->Dump();
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

      std::string tmp_id=exp->Dump();
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
  int op;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> simple_stmt;
  std::unique_ptr<BaseAST> if_stmt;
  std::unique_ptr<BaseAST> else_stmt;
  std::unique_ptr<BaseAST> closed_stmt;

  void Dump_ast(){
    std::cout << "ClosedStmtAST { ";
    if (op==1){
      simple_stmt->Dump_ast();
    }
    else if (op==2){
      exp->Dump_ast();
      if_stmt->Dump_ast();
      else_stmt->Dump_ast();
    }
    else if (op==3){
      exp->Dump_ast();
      closed_stmt->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      simple_stmt->Dump();
    }
    else if (op==2){
      bool need_end=0; // 是否需要跳转到end

      std::string tmp_id=exp->Dump();
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

      std::string tmp_id=exp->Dump();
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
  int op;
  std::unique_ptr<BaseAST> lval;
  std::unique_ptr<BaseAST> exp;
  std::unique_ptr<BaseAST> block;

  void Dump_ast(){
    std::cout << "SimpleStmtAST { ";
    if (op==1){
      lval->Dump_ast();
      std::cout<<" = ";
      exp->Dump_ast();
    }
    else if (op==2){
      std::cout<<" Do nothing ; ";
    }
    else if (op==3){
      exp->Dump_ast();
    }
    else if (op==4){
      block->Dump_ast();
    }
    else if (op==5){
      std::cout<<" return ";
    }
    else if (op==6){
      std::cout<<" return ";
      exp->Dump_ast();
    }
    else if (op==7){
      std::cout<<" break; ";
    }
    else if (op==8){
      std::cout<<" continue; ";
    }
    std::cout << " }";
  }

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
            symbol_table[i][ident].val=exp->get_val();
            std::string tmp_id=exp->Dump();
            koopa_ir+="  store "+tmp_id+", @"+ident+"_"+std::to_string(i)+"\n";
            symbol_table[i][ident].is_assigned=1;
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
      exp->Dump();
    }
    else if (op==4){
      block->Dump();
    }
    else if (op==5){
      koopa_ir+="  ret\n";
    }
    else if (op==6){
      std::string ret_val=exp->Dump();
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
  std::string ir_id;
  std::unique_ptr<BaseAST> lor_exp;

  void Dump_ast(){
    std::cout << "ExpAST { ";
    lor_exp->Dump_ast();
    std::cout << " }";
  }

  std::string Dump(){
    ir_id=lor_exp->Dump();
    return ir_id;
  }

  int get_val(){
    return lor_exp->get_val();
  }
};

class PrimaryExpAST : public BaseAST {
  public:
   int op;
   std::string ir_id;
   std::unique_ptr<BaseAST> exp;
   std::unique_ptr<BaseAST> lval;
   int number;

  void Dump_ast(){
    std::cout << "PrimaryExp { ";
    if (op==1){
      exp->Dump_ast();
    }
    else if (op==2){
      lval->Dump_ast();
    }
    else if (op==3){
      std::cout << number;
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      ir_id=exp->Dump();
    }
    else if (op==2){
      ir_id=lval->Dump();
    }
    else if (op==3){
      ir_id=std::to_string(number);
    }
    return ir_id;
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
};

class UnaryExpAST : public BaseAST {
 public:
  int op;
  std::string ir_id;
  std::unique_ptr<BaseAST> primary_exp;
  std::unique_ptr<BaseAST> unary_op;
  std::unique_ptr<BaseAST> unary_exp;

  void Dump_ast(){
    std::cout << "UnaryExpAST { ";
    if (op==1){
      primary_exp->Dump_ast();
    }
    else if (op==2){
      unary_op->Dump_ast();
      std::cout<<", ";
      unary_exp->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      ir_id=primary_exp->Dump();
    }
    else if (op==2){
      string opt=unary_op->get_op();
      if (opt=="-"){
        std::string tmp_id=unary_exp->Dump();
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = sub 0, "+tmp_id+"\n";
      }
      else if (opt=="!"){
        std::string tmp_id=unary_exp->Dump();
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = eq ";
        koopa_ir+=tmp_id;
        koopa_ir+=", 0\n";
      }
      else {
        ir_id=unary_exp->Dump();
      }
    }
    return ir_id;
  }

  int get_val(){
    if (op==1){
      return primary_exp->get_val();
    }
    else if (op==2){
      string opt=unary_op->get_op();
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
};

class UnaryOpAST : public BaseAST {
 public:
  std::string op;

  void Dump_ast(){
    std::cout << "UnaryOpAST { ";
    std::cout << op;
    std::cout << " }";
  }

  string get_op(){
    return op;
  }
};

class AddExpAST : public BaseAST {
 public:
  int op;
  std::string ir_id;
  std::string add_op;
  std::unique_ptr<BaseAST> add_exp;
  std::unique_ptr<BaseAST> mul_exp;

  void Dump_ast(){
    std::cout << "AddExpAST { ";
    if (op==1){
      mul_exp->Dump_ast();
    }
    else if (op==2){
      add_exp->Dump_ast();
      std::cout<<" "+add_op+" ";
      mul_exp->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      ir_id=mul_exp->Dump();
    }
    else if (op==2){
      std::string mul_id=mul_exp->Dump();
      std::string add_id=add_exp->Dump();
      if (add_op=="+"){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = add "+add_id+", "+mul_id+"\n";
      }
      else if (add_op=="-"){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = sub "+add_id+", "+mul_id+"\n";
      }
    }
    return ir_id;
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
};

class MulExpAST : public BaseAST {
 public:
  int op;
  std::string ir_id;
  std::unique_ptr<BaseAST> mul_exp;
  std::string mul_op;
  std::unique_ptr<BaseAST> unary_exp;

  void Dump_ast(){
    std::cout << "MulExpAST { ";
    if (op==1){
      unary_exp->Dump_ast();
    }
    else if (op==2){
      mul_exp->Dump_ast();
      std::cout<<" "+mul_op+" ";
      unary_exp->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      ir_id=unary_exp->Dump();
    }
    else if (op==2){
      std::string unary_id=unary_exp->Dump();
      std::string mul_id=mul_exp->Dump();
      if (mul_op=="*"){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = mul "+mul_id+", "+unary_id+"\n";
      }
      else if (mul_op=="/"){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = div "+mul_id+", "+unary_id+"\n";
      }
      else if (mul_op=="%"){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = mod "+mul_id+", "+unary_id+"\n";
      }
    }    
    return ir_id;
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
};

class LOrExpAST : public BaseAST {
 public:
  int op;
  std::string ir_id;
  std::unique_ptr<BaseAST> lor_exp;
  std::string lor_op;
  std::unique_ptr<BaseAST> land_exp;

  void Dump_ast(){
    std::cout << "LOrExpAST { ";
    if (op==1){
      land_exp->Dump_ast();
    }
    else if (op==2){
      lor_exp->Dump_ast();
      std::cout<<" "+lor_op+" ";
      land_exp->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      ir_id=land_exp->Dump();
    }
    else if (op==2){
      std::string res_var="result_"+std::to_string(if_tmp_id);
      std::string then_label="%then_"+std::to_string(if_tmp_id);
      std::string end_label="%end_"+std::to_string(if_tmp_id);
      if_tmp_id++;

      symbol_table[now_symbol_table_id][res_var]=symbol(1,0);
      koopa_ir+="  @"+res_var+" = alloc i32\n";
      koopa_ir+="  store "+std::to_string(1)+", @"+res_var+"\n";

      std::string lor_id=lor_exp->Dump();
      koopa_ir+="  br "+lor_id+", "+end_label+", "+then_label+"\n";
      koopa_ir+=then_label+":\n";

      std::string land_id=land_exp->Dump();
      std::string tmp_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      koopa_ir+="  "+tmp_id+" = ne "+land_id+", 0"+"\n";
      koopa_ir+="  store "+tmp_id+", @"+res_var+"\n";
      koopa_ir+="  jump "+end_label+"\n";

      koopa_ir+=end_label+":\n";

      ir_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      koopa_ir+="  "+ir_id+" = load @"+res_var+"\n";
    }    
    return ir_id;
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
};

class LAndExpAST : public BaseAST {
 public:
  int op;
  std::string ir_id;
  std::unique_ptr<BaseAST> land_exp;
  std::string land_op;
  std::unique_ptr<BaseAST> eq_exp;

  void Dump_ast(){
    std::cout << "LAndExpAST { ";
    if (op==1){
      eq_exp->Dump_ast();
    }
    else if (op==2){
      land_exp->Dump_ast();
      std::cout<<" "+land_op+" ";
      eq_exp->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      ir_id=eq_exp->Dump();
    }
    else if (op==2){
      std::string res_var="result_"+std::to_string(if_tmp_id);
      std::string then_label="%then_"+std::to_string(if_tmp_id);
      std::string end_label="%end_"+std::to_string(if_tmp_id);
      if_tmp_id++;

      symbol_table[now_symbol_table_id][res_var]=symbol(0,0);
      koopa_ir+="  @"+res_var+" = alloc i32\n";
      koopa_ir+="  store "+std::to_string(0)+", @"+res_var+"\n";

      std::string land_id=land_exp->Dump();
      koopa_ir+="  br "+land_id+", "+then_label+", "+end_label+"\n";
      koopa_ir+=then_label+":\n";

      std::string eq_id=eq_exp->Dump();
      std::string tmp_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      koopa_ir+="  "+tmp_id+" = ne "+eq_id+", 0"+"\n";
      koopa_ir+="  store "+tmp_id+", "+"  @"+res_var+"\n";
      koopa_ir+="  jump "+end_label+"\n";

      koopa_ir+=end_label+":\n";

      ir_id="%"+std::to_string(koopa_tmp_id);
      koopa_tmp_id++;
      koopa_ir+="  "+ir_id+" = load @"+res_var+"\n";
    }    
    return ir_id;
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
};

class EqExpAST : public BaseAST {
 public:
  int op;
  std::string ir_id;
  std::unique_ptr<BaseAST> eq_exp;
  std::string eq_op;
  std::unique_ptr<BaseAST> rel_exp;

  void Dump_ast(){
    std::cout << "EqExpAST { ";
    if (op==1){
      rel_exp->Dump_ast();
    }
    else if (op==2){
      eq_exp->Dump_ast();
      std::cout<<" "+eq_op+" ";
      rel_exp->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      ir_id=rel_exp->Dump();
    }
    else if (op==2){
      std::string eq_id=eq_exp->Dump();
      std::string rel_id=rel_exp->Dump();
      if (eq_op=="=="){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = eq "+eq_id+", "+rel_id+"\n";
      }
      else if (eq_op=="!="){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = ne "+eq_id+", "+rel_id+"\n";
      }
    }
    return ir_id;
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
};

class RelExpAST : public BaseAST {
 public:
  int op;
  std::string ir_id;
  std::unique_ptr<BaseAST> rel_exp;
  std::string rel_op;
  std::unique_ptr<BaseAST> add_exp;

  void Dump_ast(){
    std::cout << "RelExpAST { ";
    if (op==1){
      add_exp->Dump_ast();
    }
    else if (op==2){
      rel_exp->Dump_ast();
      std::cout<<" "+rel_op+" ";
      add_exp->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (op==1){
      ir_id=add_exp->Dump();
    }
    else if (op==2){
      std::string rel_id=rel_exp->Dump();
      std::string add_id=add_exp->Dump();
      if (rel_op=="<"){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = lt "+rel_id+", "+add_id+"\n";
      }
      else if (rel_op==">"){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = gt "+rel_id+", "+add_id+"\n";
      }
      else if (rel_op=="<="){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = le "+rel_id+", "+add_id+"\n";
      }
      else if (rel_op==">="){
        ir_id="%"+std::to_string(koopa_tmp_id);
        koopa_tmp_id++;
        koopa_ir+="  "+ir_id+" = ge "+rel_id+", "+add_id+"\n";
      }
    }
    return ir_id;
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
};

class DeclAST : public BaseAST {
 public:
  int op;
  std::unique_ptr<BaseAST> const_decl;
  std::unique_ptr<BaseAST> var_decl;

  void Dump_ast(){
    std::cout << "DeclAST { ";
    if (op==1){
      const_decl->Dump_ast();
    }
    else if (op==2){
      var_decl->Dump_ast();
    }
    std::cout << " }";
  }

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

  void Dump_ast(){
    std::cout << "ConstDeclAST { ";
    int n=const_def.size();
    for (int i=0;i<n;i++){
      const_def[i]->Dump_ast();
	  }
    std::cout << " }";
  }

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

  void Dump_ast(){
    std::cout << "VarDeclAST { ";
    int n=var_def.size();
    for (int i=0;i<n;i++){
      var_def[i]->Dump_ast();
	  }
    std::cout << " }";
  }

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

  void Dump_ast(){
    std::cout << "BTypeAST { "+btype+" }";
  }

  std::string Dump(){
    // TBD
    return "";
  }
};

class ConstDefAST : public BaseAST {
 public:
  std::string ident;
  std::unique_ptr<BaseAST> const_init_val;

  void Dump_ast(){
    std::cout << "ConstDefAST { "+ident<<" = ";
    const_init_val->Dump_ast();
    std::cout << " }";
  }

  std::string Dump(){
    if (symbol_table[now_symbol_table_id].count(ident)){
      std::cout<<"error: redefined.\n";
      exit(0);
      // return "";
    }
    int val=const_init_val->get_val();
    symbol_table[now_symbol_table_id][ident]=symbol(val,1);
    return "";
  }
};

class VarDefAST : public BaseAST {
 public:
  int op;
  std::string ident;
  std::unique_ptr<BaseAST> init_val;

  void Dump_ast(){
    if (op==1){
      std::cout << "VarDefAST { "+ident;
    }
    if (op==2){
      std::cout << "VarDefAST { "+ident+" = ";
      init_val->Dump_ast();
    }
    std::cout << " }";
  }

  std::string Dump(){
    if (symbol_table[now_symbol_table_id].count(ident)){
      std::cout<<"error: redefined.\n";
      exit(0);
      // return "";
    }
    if (op==1){
      symbol_table[now_symbol_table_id][ident]=symbol(0);
      koopa_ir+="  @"+ident+"_"+std::to_string(now_symbol_table_id)+" = alloc i32\n";
    }
    else if (op==2){
      int val=init_val->get_val();
      symbol_table[now_symbol_table_id][ident]=symbol(val,0);
      koopa_ir+="  @"+ident+"_"+std::to_string(now_symbol_table_id)+" = alloc i32\n";
      koopa_ir+="  store "+std::to_string(val)+", @"+ident+"_"+std::to_string(now_symbol_table_id)+"\n";
    }
    return "";
  }
};

class ConstInitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> const_exp;

  void Dump_ast(){
    std::cout << "ConstInitValAST { ";
    const_exp->Dump_ast();
    std::cout << " }";
  }
  
  int get_val(){
    return const_exp->get_val();
  }
};

class InitValAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;

  void Dump_ast(){
    std::cout << "InitValAST { ";
    exp->Dump_ast();
    std::cout << " }";
  }
  
  int get_val(){
    return exp->get_val();
  }
};

class ConstExpAST : public BaseAST {
 public:
  std::unique_ptr<BaseAST> exp;

  void Dump_ast(){
    std::cout << "ConstExpAST { ";
    exp->Dump_ast();
    std::cout << " }";
  }

  int get_val(){
    return exp->get_val();
  }
};

class LValAST : public BaseAST {
 public:
  std::string ident;

  void Dump_ast(){
    std::cout << "LValAST { "+ident+" }";
  }

  std::string Dump(){
    for (int i=now_symbol_table_id;i>=0;i--){
      if (symbol_table[i].count(ident)){
        if (symbol_table[i][ident].is_const){
          return std::to_string(symbol_table[i][ident].val);
        }
        else {
          if (symbol_table[i][ident].is_assigned==0){
            std::cout<<"error: unassigned variable.\n";
            exit(0);
            // return "";
          }
          else {
            std::string tmp_id="%"+std::to_string(koopa_tmp_id);
            koopa_tmp_id++;
            koopa_ir+="  "+tmp_id+" = load @"+ident+"_"+std::to_string(i)+"\n";
            return tmp_id;
          }
        }
      }
    }
    return "";
  }

  int get_val(){
    for (int i=now_symbol_table_id;i>=0;i--){
      if (symbol_table[i].count(ident)){
        if (symbol_table[i][ident].is_assigned){
          return symbol_table[i][ident].val;
        }
        else {
          std::cout<<"error: unassigned variable.\n";
          exit(0);
          // return 0;
        }
      }
    }
    
    // 运行到这里说明未找到此变量的定义
    std::cout<<"error: undeclared variable.\n";
    exit(0);
    // return 0;
  }

  std::string get_ident(){
    return ident;
  }
};