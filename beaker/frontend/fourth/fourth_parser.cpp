#include <beaker/frontend/fourth/fourth_parser.hpp>

#include <beaker/frontend/syntax.hpp>

#include <iostream>
#include <sstream>

namespace beaker
{
  /// Parse a type expression.
  ///
  ///   type-expression:
  ///     implication-expression
  Syntax* Fourth_parser::parse_type()
  {
    return parse_implication_expression();
  }

  static bool is_implication_operator(Token::Kind k)
  {
    return k == Token::dash_greater_tok ||
           k == Token::equal_greater_tok;
  }

  /// Parse an implication.
  ///
  ///   implication-expression:
  ///     logical-or-expression
  ///     logical-or-expression -> implication-expression
  ///     logical-or-expression => implication-expression
  ///
  /// We currently keep `->` and `=>` at the same precedence even though types
  /// like `(int) -> [t:type] => t` are somewhat peculiar. This is a (probably
  /// compile-time) function returning some unary variable template.
  Syntax* Fourth_parser::parse_implication_expression()
  {
    Syntax* e0 = parse_prefix_expression();
    if (Token op = match_if(is_implication_operator)) {
      Syntax* e1 = parse_implication_expression();
      return new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Parse a prefix-expression.
  ///
  ///   prefix-expression:
  ///     postfix-expression
  ///     const prefix-exprssion
  ///     & prefix-expression
  ///     * prefix-expression
  ///     - prefix-expression
  ///     + prefix-expression
  ///     not prefix-expression
  Syntax* Fourth_parser::parse_prefix_expression()
  {
    switch (lookahead())
    {
    case Token::const_tok:
    case Token::star_tok:
    case Token::amper_tok:
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

  /// Parse a postfix-expression.
  ///
  ///   postfix-expression:
  ///     primary-expression
  ///     postfix-expression ( expression-list )
  ///     postfix-expression [ expression-list ]
  ///     postfix-expression . id-expression
  Syntax* Fourth_parser::parse_postfix_expression()
  {
    Syntax* e0 = parse_primary_expression();
    while (true)
    {
      if (next_token_is(Token::lparen_tok)) {
        Syntax* args = parse_paren_list();
        e0 = new Call_syntax(e0, args);
      }
      else if (next_token_is(Token::lbracket_tok)) {
        Syntax* args = parse_bracket_list();
        e0 = new Call_syntax(e0, args);
      }
      else if (Token dot = match(Token::dot_tok)) {
        Syntax* member = parse_id_expression();
        e0 = new Infix_syntax(dot, e0, member);
      }
      else
        break;
    }
    return e0;
  }

  /// Parse a primary expression.
  ///
  ///   primary-expression:
  ///     literal
  ///     ( expression-group? )
  ///     [ expression-group? ]
  ///     id-expression
  Syntax* Fourth_parser::parse_primary_expression()
  {
    switch (lookahead()) {
      // Value literals
    case Token::true_tok:
    case Token::false_tok:
    case Token::integer_tok:
    // Type literals
    case Token::int_tok:
    case Token::bool_tok:
    case Token::type_tok: 
    case Token::ptr_tok: 
    case Token::array_tok: {
      Token value = consume();
      return new Literal_syntax(value);
    }

    case Token::identifier_tok:
      return parse_id_expression();

    case Token::lparen_tok:
      return parse_paren_group();

    case Token::lbracket_tok:
      return parse_bracket_group();

    default:
      break;
    }

    // FIXME: Return an error tree. Also, how can we recover from this?
    // It might depend on what we're parsing (declarator, type, initialzer,
    // etc.). To do that, we'd have to maintain a stack of recovery strategies
    // that we can use to skip tokens.
    diagnose_expected("primary-expression");
    return nullptr;
  }

} // namespace beaker
