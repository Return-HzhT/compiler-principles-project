#pragma once
#include "global.h"
using namespace std;

string reg_name[15]={"t0","t1","t2","t3","t4","t5","t6","a0","a1","a2","a3","a4","a5","a6","a7"};
int stack_offset=0; // 已使用的栈偏移量
int reg_cnt=0; // 已使用的寄存器数量
int ins_cnt=0; // 保存在栈上的指令返回值数量
int var_cnt=0; // 保存在栈上的已分配变量数量
int stack_size=0; // 开辟的栈空间
std::map< koopa_raw_value_t ,std::string> ins2var; // alloc指令分配的变量名对应为"@n"，指令返回值对应的临时变量名对应为"%n"
std::map< std::string, int > stack_dic; // 栈上的内存

void koopa_process(){
  const char* str=koopa_ir.c_str();
  // 解析字符串 str, 得到 Koopa IR 程序
  koopa_program_t program;
  koopa_error_code_t ret = koopa_parse_from_string(str, &program);
  assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
  // 创建一个 raw program builder, 用来构建 raw program
  koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
  // 将 Koopa IR 程序转换为 raw program
  koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
  // 释放 Koopa IR 程序占用的内存
  koopa_delete_program(program);

  // 处理 raw program
  riscv_code+="  .text\n";
  Visit(raw);

  // 处理完成, 释放 raw program builder 占用的内存
  // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
  // 所以不要在 raw program 处理完毕之前释放 builder
  koopa_delete_raw_program_builder(builder);
}

// 访问 raw program
void Visit(const koopa_raw_program_t &program) {
  // 执行一些其他的必要操作
  // 访问所有全局变量
  Visit(program.values);
  // 访问所有函数
  Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice) {
  for (size_t i = 0; i < slice.len; ++i) {
    auto ptr = slice.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    switch (slice.kind) {
      case KOOPA_RSIK_FUNCTION:
        // 访问函数
        Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
        break;
      case KOOPA_RSIK_BASIC_BLOCK:
        // 访问基本块
        Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
        break;
      case KOOPA_RSIK_VALUE:
        // 访问指令
        Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
        break;
      default:
        // 我们暂时不会遇到其他内容, 于是不对其做任何处理
        assert(false);
    }
  }
}

// 访问函数
void Visit(const koopa_raw_function_t &func) {
  // 执行一些其他的必要操作
  riscv_code+="  .global ";
  std::string func_name=(func->name)+1;
  riscv_code+=func_name+"\n";
  riscv_code+=func_name+":\n";

  stack_size=set_prologue(func);
  // 访问所有基本块
  Visit(func->bbs);
}

// 函数prologue，返回分配的栈空间总量
int set_prologue(const koopa_raw_function_t &func){
  int s=0;
  koopa_raw_slice_t bbs=func->bbs;

  int cnt=0,bbs_cnt=0;

  for (size_t i = 0; i < bbs.len; ++i) {
    bbs_cnt++;
    auto ptr1 = bbs.buffer[i];
    // 根据 slice 的 kind 决定将 ptr 视作何种元素
    koopa_raw_basic_block_t bb=reinterpret_cast<koopa_raw_basic_block_t>(ptr1);
    koopa_raw_slice_t insts=bb->insts;

    for (size_t j = 0; j < insts.len; ++j) {
      cnt++;
      auto ptr2 = insts.buffer[j];
      s+=get_value_size(reinterpret_cast<koopa_raw_value_t>(ptr2));
    }
  }

  if (s%16){ // 与16字节对齐
    s=(s/16+1)*16;
  }

  if (!s) return 0;

  if (s>=2048){
    riscv_code+="  li t0, "+std::to_string(-s)+"\n";
    riscv_code+="  add sp, sp, t0\n";
  }
  else {
    riscv_code+="  addi sp, sp, "+std::to_string(-s)+"\n";
  }
  return s;
}

// 指令所需栈空间
int get_value_size(const koopa_raw_value_t &value){
  int s=0;

  const auto &kind = value->kind;
  if (kind.tag==KOOPA_RVT_ALLOC) {
    s+=4;
  }
  const auto ty=value->ty;
  if (ty->tag!=KOOPA_RTT_UNIT){
    s+=4;
  }
  return s;
}


// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb) {
  // 执行一些其他的必要操作
  std::string bb_name=(bb->name)+1;
  if (bb_name!="entry"){
    riscv_code+=bb_name+":\n";
  }
  // 访问所有指令
  Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value) {
  // 根据指令类型判断后续需要如何访问
  int origin_cnt=reg_cnt; // 记录当前已使用的寄存器
  int dst=-1; // 记录返回值所在的寄存器

  const auto &kind = value->kind;
  switch (kind.tag) {
    case KOOPA_RVT_RETURN:
      // 访问 return 指令
      Visit_ret(kind.data.ret);
      break;
    case KOOPA_RVT_INTEGER:
      // 访问 integer 指令
      Visit_integer(kind.data.integer);
      break;
    case KOOPA_RVT_BINARY:
      dst=Visit_binary(kind.data.binary);
      break;
    case KOOPA_RVT_ALLOC:
      Visit_alloc(value);
      break;
    case KOOPA_RVT_LOAD:
      dst=Visit_load(kind.data.load);
      break;
    case KOOPA_RVT_STORE:
      Visit_store(kind.data.store);
      break;
    case KOOPA_RVT_BRANCH:
      Visit_branch(kind.data.branch);
      break;
    case KOOPA_RVT_JUMP:
      Visit_jump(kind.data.jump);
      break;
    default:
      // 其他类型暂时遇不到
      cout<<kind.tag<<endl;
      assert(false);
      break;
  }

  // 保存返回值
  const auto ty=value->ty;
  if (ty->tag!=KOOPA_RTT_UNIT&&dst>=0){
    riscv_code+="  sw "+reg_name[dst]+", "+std::to_string(stack_offset)+"(sp)\n";
    std::string var="%"+std::to_string(ins_cnt);
    ins2var[value]=var;
    ins_cnt++;
    stack_dic[var]=stack_offset;
    stack_offset+=4;
  }

  reg_cnt=origin_cnt; // 释放执行此指令使用的临时寄存器
}

void Visit_alloc(const koopa_raw_value_t &value){ // 分配一个变量到栈上
  koopa_raw_type_t ty=value->ty;
  switch (ty->tag){
    case KOOPA_RTT_POINTER:
    {
      std::string var="@"+std::to_string(var_cnt);
      ins2var[value]=var;
      var_cnt++;
      stack_dic[var]=stack_offset;
      stack_offset+=4;
      break;
    }
    default:
      assert(false);
      break;
  }
}

void Visit_store(const koopa_raw_store_t &store){
  koopa_raw_value_t value=store.value;
  koopa_raw_value_t dest=store.dest;
  std::string now_reg=reg_name[reg_cnt];
  reg_cnt++;

  switch (value->kind.tag){
    case KOOPA_RVT_INTEGER:{// 保存值为一个立即数 
      riscv_code+="  li "+now_reg+", "+std::to_string(value->kind.data.integer.value)+"\n";
      std::string var=ins2var[dest];
      int offset=stack_dic[var];
      riscv_code+="  sw "+now_reg+", "+std::to_string(offset)+"(sp)\n";
      break;
    }
    case KOOPA_RVT_BINARY:{// 保存值为一个指令返回值
      std::string var=ins2var[value];
      int offset=stack_dic[var];
      riscv_code+="  lw "+now_reg+", "+std::to_string(offset)+"(sp)\n";
      var=ins2var[dest];
      offset=stack_dic[var];
      riscv_code+="  sw "+now_reg+", "+std::to_string(offset)+"(sp)\n";
      break;
    }
    default:
      break;
  }
}

int Visit_load(const koopa_raw_load_t &load){
  koopa_raw_value_t src=load.src;
  int dst=reg_cnt;
  std::string now_reg=reg_name[reg_cnt];
  reg_cnt++;
  std::string var=ins2var[src];
  int offset=stack_dic[var];
  riscv_code+="  lw "+now_reg+", "+std::to_string(offset)+"(sp)\n";
  return dst;
}

void Visit_branch(const koopa_raw_branch_t &branch){
  koopa_raw_value_t cond=branch.cond;
  koopa_raw_basic_block_t true_bb=branch.true_bb;
  koopa_raw_basic_block_t false_bb=branch.false_bb;

  std::string now_reg=reg_name[reg_cnt];
  reg_cnt++;
  if (cond->kind.tag==KOOPA_RVT_INTEGER){ // 分支条件是立即数
    int32_t int_val = cond->kind.data.integer.value;
    riscv_code+="  li "+now_reg+", "+std::to_string(int_val)+"\n";
  }
  else { // 分支条件是变量
    std::string now_reg=reg_name[reg_cnt];
    reg_cnt++;
    std::string var=ins2var[cond];
    int offset=stack_dic[var];
    riscv_code+="  lw "+now_reg+", "+std::to_string(offset)+"(sp)\n";
  }

  std::string true_label=(true_bb->name)+1;
  std::string false_label=(false_bb->name)+1;
  riscv_code+="  bnez "+now_reg+", "+true_label+"\n";
  riscv_code+="  j "+false_label+"\n";
}

void Visit_jump(const koopa_raw_jump_t &jump){
  koopa_raw_basic_block_t target=jump.target;
  std::string target_label=(target->name)+1;
  riscv_code+="  j "+target_label+"\n";
}

int Visit_binary(const koopa_raw_binary_t &binary){
  koopa_raw_binary_op_t op = binary.op;

  string l,r;
  int dst=reg_cnt;
  string now_reg=reg_name[reg_cnt];
  reg_cnt++;
  
  switch (op) {
    case KOOPA_RBO_ADD: // 加
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  add "+now_reg+", "+l+", "+r+"\n";
      break;
    case KOOPA_RBO_SUB: // 减
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  sub "+now_reg+", "+l+", "+r+"\n";
      break;
    case KOOPA_RBO_MUL: // 乘
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  mul "+now_reg+", "+l+", "+r+"\n";
      break;
    case KOOPA_RBO_DIV: // 除
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  div "+now_reg+", "+l+", "+r+"\n";
      break;
    case KOOPA_RBO_MOD: // 模
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  rem "+now_reg+", "+l+", "+r+"\n";
      break;
    case KOOPA_RBO_NOT_EQ: // 不等
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  xor "+now_reg+", "+l+", "+r+"\n";
      riscv_code+="  snez "+now_reg+", "+now_reg+"\n";
      break;
    case KOOPA_RBO_EQ: // 等于
      if (r=="x0"){
        riscv_code+="  li "+now_reg+", ";
        riscv_code+=l;
        riscv_code+="\n";
        riscv_code+="  xor "+now_reg+", "+now_reg+", x0\n";
        riscv_code+="  seqz "+now_reg+", "+now_reg+"\n";
      }
      if (l=="x0"){
        riscv_code+="  li "+now_reg+", ";
        riscv_code+=r;
        riscv_code+="\n";
        riscv_code+="  xor "+now_reg+", "+now_reg+", x0\n";
        riscv_code+="  seqz "+now_reg+", "+now_reg+"\n";
      }
      else {
        get_operand_load_reg(binary.lhs,l);
        get_operand_load_reg(binary.rhs,r);
        riscv_code+="  xor "+now_reg+", "+l+", "+r+"\n";
        riscv_code+="  seqz "+now_reg+", "+now_reg+"\n";
      }
      break;
    case KOOPA_RBO_GT: // 大于
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  slt "+now_reg+", "+r+", "+l+"\n";
      break;
    case KOOPA_RBO_LT: // 小于
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  slt "+now_reg+", "+l+", "+r+"\n";
      break;
    case KOOPA_RBO_GE: // 大于等于
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  slt "+now_reg+", "+l+", "+r+"\n";
      riscv_code+="  xori "+now_reg+", "+now_reg+", 1\n";
      break;    
    case KOOPA_RBO_LE: // 小于等于
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  slt "+now_reg+", "+r+", "+l+"\n";
      riscv_code+="  xori "+now_reg+", "+now_reg+", 1\n";
      break;
    case KOOPA_RBO_AND: // 按位与
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  and "+now_reg+", "+l+", "+r+"\n";
      break;
    case KOOPA_RBO_OR: // 按位或
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  or "+now_reg+", "+l+", "+r+"\n";
      break;
    case KOOPA_RBO_XOR: // 按位异或
      get_operand_load_reg(binary.lhs,l);
      get_operand_load_reg(binary.rhs,r);
      riscv_code+="  xor "+now_reg+", "+l+", "+r+"\n";
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
  return dst;
}


void Visit_ret(const koopa_raw_return_t &ret){ // 返回指令
  koopa_raw_value_t ret_value = ret.value;
  
  switch (ret_value->kind.tag){ // 返回立即数
    case KOOPA_RVT_INTEGER:{
      int32_t int_val = ret_value->kind.data.integer.value;
      riscv_code+="  li a0, "+(std::to_string(int_val))+"\n";
      break;
    }
    case KOOPA_RVT_LOAD:
    case KOOPA_RVT_BINARY:{ // 返回变量
      const auto ty=ret_value->ty;
      if (ty->tag!=KOOPA_RTT_UNIT){ 
        if (ins2var.count(ret_value)){
          std::string var=ins2var[ret_value];
          int offset=stack_dic[var];
          std::string now_reg=reg_name[reg_cnt];
          reg_cnt++;
          riscv_code+="  lw a0, "+std::to_string(offset)+"(sp)\n";
        }
      }
      break;
    }
    default:
      std::cout<<ret_value->kind.tag<<endl;
      break;
  }

  if (stack_size){
    if (stack_size>=2048){
      riscv_code+="  li t0, "+std::to_string(stack_size)+"\n";
      riscv_code+="  add sp, sp, t0\n";
    }
    else {
      riscv_code+="  addi sp, sp, "+std::to_string(stack_size)+"\n";
    }
  }

  riscv_code+="  ret\n";
}

void Visit_integer(const koopa_raw_integer_t &int_num){
  // TBD
}

void get_operand_load_reg(const koopa_raw_value_t &t, std::string &operand_str){ // 解析操作数，如果其为立即数则将其放入寄存器中
  // 已保存在栈上的指令的返回值
  const auto ty=t->ty;
  if (ty->tag!=KOOPA_RTT_UNIT){
    if (ins2var.count(t)){
      std::string var=ins2var[t];
      int offset=stack_dic[var];
      std::string now_reg=reg_name[reg_cnt];
      reg_cnt++;
      riscv_code+="  lw "+now_reg+", "+std::to_string(offset)+"(sp)\n";
      operand_str=now_reg;
      return;
    }
  }

  switch(t->kind.tag){
    case KOOPA_RVT_INTEGER:
      if (t->kind.data.integer.value==0){ // 零寄存器
        operand_str="x0";
      }
      else { // 放入寄存器
        string now_reg=reg_name[reg_cnt];
        reg_cnt++;
        riscv_code+="  li "+now_reg+", ";
        riscv_code+=std::to_string(t->kind.data.integer.value);
        riscv_code+="\n";
        operand_str=now_reg;
      }
      break;
    default:
      // 其他类型暂时遇不到
      assert(false);
  }
}

// void get_operand(const koopa_raw_value_t &t, std::string &operand_str){ // 解析操作数，返回立即数或寄存器
//   // 已保存在栈上的指令的返回值
//   const auto ty=t->ty;
//   if (ty->tag!=KOOPA_RTT_UNIT){
//     if (ins2var.count(t)){
//       std::string var=ins2var[t];
//       int offset=stack_dic[var];
//       std::string now_reg=reg_name[reg_cnt];
//       reg_cnt++;
//       riscv_code+="  lw "+now_reg+", "+std::to_string(offset)+"(sp)\n";
//       operand_str=now_reg;
//       return;
//     }
//   }

//   switch(t->kind.tag){
//     case KOOPA_RVT_INTEGER:
//       if (t->kind.data.integer.value==0){
//         operand_str="x0";
//       }
//       else operand_str=std::to_string(t->kind.data.integer.value);
//       break;
//     default:
//       // 其他类型暂时遇不到
//       assert(false);
//   }
// }

// 访问对应类型指令的函数定义略
// 视需求自行实现
// ...