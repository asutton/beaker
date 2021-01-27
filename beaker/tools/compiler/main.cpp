#include <beaker/language/translation.hpp>
#include <beaker/frontend/syntax.hpp>
#include <beaker/frontend/parser.hpp>

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

using namespace beaker;

int main(int argc, char* argv[])
{
  if (argc == 1)
    throw std::runtime_error("usage error");

  // The input file(s).
  std::vector<std::filesystem::path> inputs;

  // FIXME: Parse commands out of the argument vector. For exmaple,
  // we might want the following:
  //
  //    beaker-compile executable ...
  //    beaker-compile archive ...
  //    beaker-compile module ...

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg[0] == '-') {
      throw std::runtime_error("invalid option");
    }
    else {
      // FIXME: Check to see if the file exists before canonicalizing. Or
      // trap the exception and diagnose the error.
      std::filesystem::path p = argv[i];
      inputs.push_back(std::filesystem::canonical(p));
    }
  }
  
  if (inputs.empty())
    throw std::runtime_error("no inputs given");
  if (inputs.size() > 1)
    throw std::runtime_error("only one input allowed");

  Translation trans;

  // Parse the input file.
  Parser parser(trans, inputs[0]);
  Syntax* syn = parser.parse_file();
  syn->dump();

  return 0;
}
