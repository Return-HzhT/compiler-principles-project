%code requires {
  #include <memory>
  #include <string>
  #include <ast.h>
}

%{

#include <iostream>
#include <memory>
#include <string>
#include <ast.h>

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}


// lexer 返回的所有 token 种类的声明
%token INT RETURN CONST IF ELSE  WHILE BREAK CONTINUE
%token <str_val> IDENT LEq GEq Eq NEq LAnd LOr
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block BlockItem RestOfBlockItem Stmt OpenStmt ClosedStmt SimpleStmt
%type <ast_val> Exp UnaryExp PrimaryExp UnaryOp AddExp MulExp LOrExp LAndExp EqExp RelExp ConstExp 
%type <ast_val> Decl ConstDecl VarDecl BType ConstDef VarDef RestOfConstDef RestOfVarDef ConstInitVal InitVal LVal 
%type <int_val> Number

%%

CompUnit
  : FuncDef {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->func_def = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->func_type = "int";
    $$ = ast;
  }
  ;

Block
  : '{' '}' {
    auto ast = new BlockAST();
    $$ = ast;
  }
  | '{' RestOfBlockItem '}'{
    auto ast = new BlockAST();
    ast->rest_of_block_item=unique_ptr<BaseAST>($2);
    ast->get_block_item_vec();
    $$ = ast;
  }
  ;

RestOfBlockItem
  : BlockItem {
    auto ast = new RestOfBlockItemAST();
    ast->rest_of_block_item = NULL;
    ast->block_item=unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | BlockItem RestOfBlockItem {
    auto ast = new RestOfBlockItemAST();
    ast->block_item=unique_ptr<BaseAST>($1);
    ast->rest_of_block_item = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->op = 1;
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->op = 2;
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Stmt 
  : OpenStmt {
    auto ast = new StmtAST();
    ast->op = 1;
    ast->open_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  } 
  | ClosedStmt {
    auto ast = new StmtAST();
    ast->op = 2;
    ast->closed_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

OpenStmt
  : IF '(' Exp ')' Stmt {
    auto ast = new OpenStmtAST();
    ast->op = 1;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | IF '(' Exp ')' ClosedStmt ELSE OpenStmt {
    auto ast = new OpenStmtAST();
    ast->op = 2;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' OpenStmt {
    auto ast = new OpenStmtAST();
    ast->op = 3;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->open_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

ClosedStmt
  : SimpleStmt {
    auto ast = new ClosedStmtAST();
    ast->op = 1;
    ast->simple_stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | IF '(' Exp ')' ClosedStmt ELSE ClosedStmt {
    auto ast = new ClosedStmtAST();
    ast->op = 2;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->if_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | WHILE '(' Exp ')' ClosedStmt {
    auto ast = new ClosedStmtAST();
    ast->op = 3;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->closed_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

SimpleStmt
  : LVal '=' Exp ';' {
    auto ast = new SimpleStmtAST();
    ast->op = 1;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);    
    $$ = ast;
  }
  | ';' {
    auto ast=new SimpleStmtAST();
    ast->op = 2;
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new SimpleStmtAST();
    ast->op = 3;
    ast->exp = unique_ptr<BaseAST>($1);    
    $$ = ast;
  }
  | Block {
    auto ast = new SimpleStmtAST();
    ast->op = 4;
    ast->block = unique_ptr<BaseAST>($1);    
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new SimpleStmtAST();
    ast->op = 5;
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new SimpleStmtAST();
    ast->op = 6;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new SimpleStmtAST();
    ast->op = 7;
    $$ = ast;
  }
  | CONTINUE ';'{
    auto ast = new SimpleStmtAST();
    ast->op = 8;
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->op = 1;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp '+' MulExp {
    auto ast = new AddExpAST();
    ast->op = 2;
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->add_op = "+";
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | AddExp '-' MulExp {
    auto ast = new AddExpAST();
    ast->op = 2;
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->add_op = "-";
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->op = 1;
    ast-> unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp {
    auto ast = new MulExpAST();
    ast->op = 2;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->mul_op = "*";
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '/' UnaryExp {
    auto ast = new MulExpAST();
    ast->op = 2;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->mul_op = "/";
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '%' UnaryExp {
    auto ast = new MulExpAST();
    ast->op = 2;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->mul_op = "%";
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->op = 1;
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOr LAndExp {
    auto ast = new LOrExpAST();
    ast->op = 2;
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->lor_op = "||";
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->op = 1;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAnd EqExp {
    auto ast = new LAndExpAST();
    ast->op = 2;
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->land_op = "&&";
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->op = 1;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp Eq RelExp {
    auto ast = new EqExpAST();
    ast->op = 2;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->eq_op = "==";
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | EqExp NEq RelExp {
    auto ast = new EqExpAST();
    ast->op = 2;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->eq_op = "!=";
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->op = 1;
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp '<' AddExp {
    auto ast = new RelExpAST();
    ast->op = 2;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = "<";
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '>' AddExp {
    auto ast = new RelExpAST();
    ast->op = 2;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = ">";
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp LEq AddExp {
    auto ast = new RelExpAST();
    ast->op = 2;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = "<=";
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GEq AddExp {
    auto ast = new RelExpAST();
    ast->op = 2;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = ">=";
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->op = 1;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->op = 2;
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->op = 3;
    ast->number = int($1);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->op = 1;
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->op = 2;
    ast->unary_op = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->op = 1;
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->op = 2;
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDef ';'{
    auto ast = new ConstDeclAST();
    ast->btype = unique_ptr<BaseAST>($2);
    ast->const_def.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  | CONST BType ConstDef RestOfConstDef ';'{
    auto ast = new ConstDeclAST();
    ast->btype = unique_ptr<BaseAST>($2);
    ast->first_const_def = unique_ptr<BaseAST>($3);
    ast->rest_of_const_def = unique_ptr<BaseAST>($4);
    ast->get_const_def_vec();
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDef ';'{
    auto ast = new VarDeclAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->var_def.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | BType VarDef RestOfVarDef ';'{
    auto ast = new VarDeclAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->first_var_def = unique_ptr<BaseAST>($2);
    ast->rest_of_var_def = unique_ptr<BaseAST>($3);
    ast->get_var_def_vec();
    $$ = ast;
  }
  ;

RestOfConstDef
  : ',' ConstDef {
    auto ast = new RestOfConstDefAST();
    ast->rest_of_const_def = NULL;
    ast->const_def = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | ',' ConstDef RestOfConstDef {
    auto ast = new RestOfConstDefAST();
    ast->const_def = unique_ptr<BaseAST>($2);
    ast->rest_of_const_def = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RestOfVarDef
  : ',' VarDef {
    auto ast = new RestOfVarDefAST();
    ast->rest_of_var_def = NULL;
    ast->var_def = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | ',' VarDef RestOfVarDef {
    auto ast = new RestOfVarDefAST();
    ast->var_def = unique_ptr<BaseAST>($2);
    ast->rest_of_var_def = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

BType
  : INT {
    auto ast = new BTypeAST();
    ast->btype = "int";
    $$ = ast;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->op = 1;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->op = 2;
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    auto ast = new UnaryOpAST();
    ast->op = "+";
    $$=ast;
  }
  | '-' {
    auto ast = new UnaryOpAST();
    ast->op = "-";
    $$=ast;
  }
  |  '!' {
    auto ast = new UnaryOpAST();
    ast->op = "!";
    $$=ast;
  }
  ;

Number
  : INT_CONST {
    $$ = int($1);
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
