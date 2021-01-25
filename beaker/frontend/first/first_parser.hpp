#ifndef BEAKER_FRONTEND_FIRST_PARSER_HPP
#define BEAKER_FRONTEND_FIRST_PARSER_HPP

#include <beaker/frontend/parser.hpp>

namespace beaker
{
  /// Constructs a concrete syntax tree from a source file.
  ///
  /// This language simply inherits the grammar of the "root" parser. It
  /// doesn't change any of the syntax.
  struct First_parser : Parser
  {
    using Parser::Parser;
  };

} // namespace beaker

#endif
