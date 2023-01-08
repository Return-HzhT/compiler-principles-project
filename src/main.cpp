#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>
#include <string>
#include "global.h"
#include "ast.h"
#include "koopa2riscv.h"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);

int main(int argc, const char *argv[]) {
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];

  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  assert(!ret);

  koopa_ir="";
  ast->Dump();
  riscv_code="";

  ofstream out(output);
  if (mode[1]=='k'){
    out<<koopa_ir;
  }
  else if (mode[1]=='r'||mode[1]=='p'){
    koopa_process();

    // 删除冗余load
    vector<string> split_riscv_code;
    string tmp_str=riscv_code;
	  int pos=tmp_str.find("\n");
	  while (pos!=tmp_str.npos){
		  string tmp=tmp_str.substr(0,pos);
		  split_riscv_code.push_back(tmp);
		  tmp_str=tmp_str.substr(pos+1,tmp_str.size());
		  pos=tmp_str.find("\n");
	  }

    int n=split_riscv_code.size();
    bool deleted[n];
    memset(deleted,0,sizeof(deleted));
    for (int i=0;i<n;i++){
      if (deleted[i]){
        continue;
      }
      else {
        if (split_riscv_code[i].size()>=4){
          string opt=split_riscv_code[i].substr(0,4);
          if (opt=="  sw"){
            for (int j=i+1;j<n;j++){
              if (split_riscv_code[j].size()==split_riscv_code[i].size()){
                string another_opt=split_riscv_code[j].substr(0,4);
                if (another_opt=="  lw"){
                  string str1=split_riscv_code[i].substr(3,split_riscv_code[i].size());
                  string str2=split_riscv_code[j].substr(3,split_riscv_code[j].size());
                  if (str1==str2){
                    deleted[j]=1;
                  }
                }
              }
              if (!deleted[j]){
                break;
              }
            }
          }
        }
      }
    }
    string final_riscv_code="";
    for (int i=0;i<n;i++){
      if (!deleted[i]){
        final_riscv_code+=split_riscv_code[i];
        final_riscv_code+="\n";
      }
    }
    
    out<<final_riscv_code;
  }
  return 0;
}
