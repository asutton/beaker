#include <beaker/frontend/syntax.hpp>
#include <beaker/frontend/first/first_parser.hpp>
#include <beaker/frontend/second/second_parser.hpp>
#include <beaker/frontend/third/third_parser.hpp>
#include <beaker/frontend/fourth/fourth_parser.hpp>

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

using namespace beaker;

enum Language
{
  default_lang,
  first_lang,   // extension .bkr and .bkr1
  second_lang,  // extension .bkr2
  third_lang,   // extension .bkr3
  fourth_lang,  // extension .bkr4
};

// Parse the value of the -language flag.
Language parse_language(int arg, int argc, char* argv[])
{
  if (arg >= argc)
    throw std::runtime_error("missing language");
  std::string lang = argv[arg];
  if (lang == "first")
    return first_lang;
  if (lang == "second")
    return second_lang;
  if (lang == "third")
    return third_lang;
  if (lang == "fourth")
    return fourth_lang;
  throw std::runtime_error("invalid language");
}

// Try inferring the language variant from the file extension.
Language infer_language(std::filesystem::path const& p)
{
  auto ext = p.extension();
  if (ext == ".bkr" || ext == ".bkr1")
    return first_lang;
  if (ext == ".bkr2")
    return second_lang;
  if (ext == ".bkr3")
    return third_lang;
  if (ext == ".bkr4")
    return fourth_lang;
  throw std::runtime_error("unknown language");
}

static std::unique_ptr<Parser> make_parser(Language lang, Translation& trans, std::filesystem::path const& p)
{
  switch (lang) {
  case default_lang:
  case first_lang:
    return std::make_unique<First_parser>(trans, p);
  case second_lang:
    return std::make_unique<Second_parser>(trans, p);
  case third_lang:
    return std::make_unique<Third_parser>(trans, p);
  case fourth_lang:
    return std::make_unique<Fourth_parser>(trans, p);
  default:
    assert(false);
  }
}

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

  Language lang = default_lang;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg[0] == '-') {
      if (arg == "-language") {
        lang = parse_language(++i, argc, argv);
      }
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

  // If no language was specified, try inferring the language from
  // the file extension.
  if (lang == default_lang)
    lang = infer_language(inputs[0]);

  Translation trans;

  // Parse the input file.
  std::unique_ptr<Parser> parser = make_parser(lang, trans, inputs[0]);
  Syntax* syn = parser->parse_file();
  syn->dump();

  return 0;
}
