/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Skeleton interface for Bison GLR parsers in C

   Copyright (C) 2002-2015, 2018-2020 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_COMPILER_PRINCIPLES_PROJECT_BUILD_SYSY_TAB_HPP_INCLUDED
# define YY_YY_COMPILER_PRINCIPLES_PROJECT_BUILD_SYSY_TAB_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "/compiler-principles-project/src/sysy.y"

  #include <memory>
  #include <string>
  #include <ast.h>

#line 50 "/compiler-principles-project/build/sysy.tab.hpp"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INT = 258,
    VOID = 259,
    RETURN = 260,
    CONST = 261,
    IF = 262,
    ELSE = 263,
    WHILE = 264,
    BREAK = 265,
    CONTINUE = 266,
    IDENT = 267,
    LEq = 268,
    GEq = 269,
    Eq = 270,
    NEq = 271,
    LAnd = 272,
    LOr = 273,
    INT_CONST = 274
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 33 "/compiler-principles-project/src/sysy.y"

  std::string *str_val;
  int int_val;
  BaseAST *ast_val;

#line 87 "/compiler-principles-project/build/sysy.tab.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (std::unique_ptr<BaseAST> &ast);

#endif /* !YY_YY_COMPILER_PRINCIPLES_PROJECT_BUILD_SYSY_TAB_HPP_INCLUDED  */
