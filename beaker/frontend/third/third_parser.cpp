#include <beaker/frontend/third/third_parser.hpp>

#include <beaker/frontend/syntax.hpp>

#include <iostream>
#include <sstream>

namespace beaker
{
  static bool starts_function_type(Parser& p)
  {
    assert(p.lookahead() == Token::lparen_tok);

    // Match `( )` and `( :`
    Token::Kind k1 = p.lookahead(1);
    if (k1 == Token::rparen_tok || k1 == Token::colon_tok)
      return true;
    
    // Match `( identifier :`.
    Token::Kind k2 = p.lookahead(2);
    if (k1 == Token::identifier_tok && k2 == Token::colon_tok)
      return true;
    
    return false;
  }

  /// Parse a prefix-expression.
  ///
  ///   prefix-expression:
  ///     postfix-expression
  ///     [ expression-group? ] prefix-expression
  ///     ( parameter-group? ) prefix-expression
  ///     const prefix-exprssion
  ///     ^ prefix-expression
  ///     - prefix-expression
  ///     + prefix-expression
  ///     not prefix-expression
  Syntax* Third_parser::parse_prefix_expression()
  {
    switch (lookahead())
    {
    case Token::lbracket_tok: {
      Syntax* bound = parse_bracket_group();
      Syntax* type = parse_prefix_expression();
      return new Introduction_syntax(bound, type);
    }

    case Token::lparen_tok: {
      if (!starts_function_type(*this))
        break;
      Syntax* parms = parse_paren_list();
      Syntax* result = parse_prefix_expression();
      return new Introduction_syntax(parms, result);
    }

    case Token::const_tok:
    case Token::caret_tok:
    case Token::plus_tok:
    case Token::dash_tok:
    case Token::not_tok: {
      Token op = consume();
      Syntax* e = parse_prefix_expression();
      return new Prefix_syntax(op, e);
    }
    
    default:
      break;
    }
    return parse_postfix_expression();
  }


  /// Returns a list defining the group.
  static Syntax* make_group(std::vector<Syntax*>& ts)
  {
    // This only happens when there's an error and we can't accumulate
    // a group. If we propagate errors, this shouldn't happen at all.
    if (ts.empty())
      return nullptr;    

    // Don't allocate groups if there's only one present.    
    if (ts.size() == 1)
      return ts[0];

    return new List_syntax(std::move(ts));
  }

  /// Parse an expression-group.
  ///
  ///   parameter-group:
  ///     parameter-list
  ///     parameter-group ; parameter-list
  ///
  /// Groups are only created if multiple groups are present.
  Syntax* Third_parser::parse_parameter_group()
  {
    std::vector<Syntax*> ts;
    parse_item(*this, &Third_parser::parse_parameter_list, ts);
    while (match(Token::semicolon_tok))
      parse_item(*this, &Third_parser::parse_parameter_list, ts);
    return make_group(ts);
  }

  // Returns a list for `ts`.
  static Syntax* make_list(std::vector<Syntax*>& ts)
  {
    // This only happens when an error occurred.
    if (ts.empty())
      return nullptr;

    return new List_syntax(std::move(ts));
  }

  /// Parse an parameter-list.
  ///
  ///   parameter-list:
  ///     parameter
  ///     parameter-list , parameter
  ///
  /// This always returns a list, even if there's a single element.
  Syntax* Third_parser::parse_parameter_list()
  {
    std::vector<Syntax*> ts;
    parse_item(*this, &Third_parser::parse_parameter, ts);
    while (match(Token::comma_tok))
      parse_item(*this, &Third_parser::parse_parameter, ts);
    return make_list(ts);
  }

} // namespace beaker
