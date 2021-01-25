#ifndef BEAKER_FRONTEND_FOURTH_PARSER_HPP
#define BEAKER_FRONTEND_FOURTH_PARSER_HPP

#include <beaker/frontend/parser.hpp>

namespace beaker
{
  /// Constructs a concrete syntax tree from a source file.
  struct Fourth_parser : Parser
  {
    using Parser::Parser;

    Syntax* parse_type() override;
    Syntax* parse_implication_expression() override;
    Syntax* parse_prefix_expression() override;
    Syntax* parse_postfix_expression() override;
    Syntax* parse_primary_expression() override;
  };

} // namespace beaker

#endif
