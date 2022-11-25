#pragma once

#include <string>
#include <vector>

using namespace std;
class symbol{
 public:
  std::string ident;
  int val;
  bool is_const;
};

std::vector< std::unique_ptr<class symbol> > symbol_table;