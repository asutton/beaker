#include <beaker/language/translation.hpp>
#include <beaker/frontend/syntax.hpp>
#include <beaker/tools/fuzzer/fuzzer.hpp>

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

using namespace beaker;

int main(int argc, char* argv[])
{
  // TODO: There are probably lots and lots of options for fuzzing.
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg[0] == '-') {
      throw std::runtime_error("invalid option");
    }
    else {
      throw std::runtime_error("invalid option");
    }
  }

  Translation trans;

  // Parse the input file.
  Fuzzer fuzzer(trans);
  Syntax* syn = fuzzer.fuzz_file();
  syn->dump();

  return 0;
}
