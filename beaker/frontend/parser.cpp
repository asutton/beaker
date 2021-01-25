#include <beaker/frontend/parser.hpp>

#include <beaker/frontend/syntax.hpp>

#include <iostream>
#include <sstream>

namespace beaker
{
  Parser::Parser(Translation& trans, std::filesystem::path const& p)
    : m_trans(trans), m_lex(trans, p)
  {
    // Tokenize the input and point to the first token.
    m_lex.get(m_toks);
    m_pos = 0;
  }

  Syntax* Parser::parse_file()
  {
    Syntax* s = parse_declaration_seq();
    return new File_syntax(s);
  }

  /// A helper function for parsing items in a list or sequence.
  /// Accumulates the result in `ts`.
  template<typename F>
  static Syntax* parse_item(Parser& p, F fn, Syntax_seq& ss)
  {
    // TODO: If we represent syntax errors explicitly, then
    // the parser will always return a non-null pointer.
    Syntax* s = (p.*fn)();
    if (s)
      ss.push_back(s);
    return s;
  }

  Syntax* Parser::parse_declaration_seq()
  {
    Syntax_seq ss;
    while (!eof())
      parse_item(*this, &Parser::parse_declaration, ss);
    return new Sequence_syntax(std::move(ss));
  }

  Syntax* Parser::parse_declaration()
  {
    switch (lookahead()) {
    case Token::def_tok:
      return parse_definition();
    default:
      break;
    }

    // FIXME: Return an error node. Also, how do we recover? We've got
    // tokens not belonging to any particular declaration, so what would
    // we skip to.
    diagnose_expected("declaration");
    return nullptr;
  }

  /// Definition declaration:
  ///
  ///   definition-declaration:
  ///     def declarator : type ;
  ///     def declarator : = expression ;
  ///     def declarator : type = expression ;
  ///
  /// TODO: Support brace initialization `def x : t { ... }`, although I'm
  /// not sure what the grammar of `...` is. A sequence of statements? A
  /// list of expressions. We can probably build a single grammar that supports
  /// both.
  Syntax* Parser::parse_definition()
  {
    Token intro = require(Token::def_tok);

    // Parse the declarator
    Syntax* decl = parse_declarator();

    // Parse the type.
    expect(Token::colon_tok);
    
    Syntax* type = nullptr;
    if (next_token_is_not(Token::equal_tok)) {
      type = parse_type();

      // Match the 'decl : type ;' case.
      if (match(Token::semicolon_tok))
        return new Declaration_syntax(intro, decl, type, nullptr);

      // Fall through to parse the initializer.
    }

    // Parse the initializer.
    expect(Token::equal_tok);
    Syntax* init = parse_expression();
    expect(Token::semicolon_tok);

    return new Declaration_syntax(intro, decl, type, init);
  }

  /// Parser a parameter:
  ///
  ///   identifier : type
  ///   identifier : type = expression
  ///   identifeir : = expression
  ///   : type
  ///   : type = expression
  ///
  /// TODO: Can parameters have introducers?
  ///
  /// TODO: Can paramters be packs (yes, but what's the syntax?).
  Syntax* Parser::parse_parameter()
  {
    // Match unnamed variants.
    if (match(Token::colon_tok)) {
      Syntax* type = parse_type();
      Syntax* init = nullptr;
      if (match(Token::equal_tok))
        init = parse_expression();
      return new Declaration_syntax({}, nullptr, type, init);
    }

    // Match the identifier...
    Syntax* id = parse_id_expression();
    
    // ... And optional declarative information
    Syntax* type = nullptr;
    Syntax* init = nullptr;
    if (match(Token::colon_tok)) {
      if (next_token_is_not(Token::equal_tok))
        type = parse_type();
      if (match(Token::equal_tok))
        init = parse_expression();
    }

    return new Declaration_syntax({}, id, type, init);
  }

  /// Parse a declarator.
  ///
  ///   declarator:
  ///     postfix-expression
  Syntax* Parser::parse_declarator()
  {
    return parse_postfix_expression();
  }

  /// Parse a type expression.
  ///
  ///   type-expression:
  ///     prefix-expression
  Syntax* Parser::parse_type()
  {
    return parse_prefix_expression();
  }

  /// Parse an expression.
  ///
  ///   expression:
  ///     impliciation-expression
  ///
  /// FIXME: Where do conditional expressions fit in? Lower or higher than
  /// implication? Or at the same precedence?
  Syntax* Parser::parse_expression()
  {
    return parse_implication_expression();
  }

  /// Parse an implication.
  ///
  ///   implication-expression:
  ///     logical-or-expression
  ///     logical-or-expression -> implication-expression
  Syntax* Parser::parse_implication_expression()
  {
    Syntax* e0 = parse_prefix_expression();
    if (Token op = match(Token::arrow_tok)) {
      Syntax* e1 = parse_implication_expression();
      return new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Parse an logical or.
  ///
  ///   logical-or-expression:
  ///     logical-and-expression
  ///     logical-or-expression or logical-and-expression
  Syntax* Parser::parse_logical_or_expression()
  {
    Syntax* e0 = parse_logical_and_expression();
    while (Token op = match(Token::or_tok)) {
      Syntax* e1 = parse_logical_and_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Parse an logical and.
  ///
  ///   logical-and-expression:
  ///     equality-expression
  ///     logical-and-expression and equality-expression
  Syntax* Parser::parse_logical_and_expression()
  {
    Syntax* e0 = parse_equality_expression();
    while (Token op = match(Token::and_tok)) {
      Syntax* e1 = parse_equality_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  static bool is_equality_operator(Token::Kind k)
  {
    return k == Token::equal_equal_tok ||
           k == Token::bang_equal_tok;
  }

  /// Parse an equality comparison.
  ///
  ///   equality-expression:
  ///     relational-expression
  ///     equality-expression == relational-expression
  ///     equality-expression != relational-expression
  Syntax* Parser::parse_equality_expression()
  {
    Syntax* e0 = parse_relational_expression();
    while (Token op = match_if(is_equality_operator)) {
      Syntax* e1 = parse_relational_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  static bool is_relational_operator(Token::Kind k)
  {
    return k == Token::less_tok ||
           k == Token::greater_tok ||
           k == Token::less_equal_tok ||
           k == Token::greater_equal_tok;
  }

  /// Parse a relational expression.
  ///
  ///   relational-expression:
  ///     additive-expression
  ///     relational-expression < additive-expression
  ///     relational-expression > additive-expression
  ///     relational-expression <= additive-expression
  ///     relational-expression >= additive-expression
  Syntax* Parser::parse_relational_expression()
  {
    Syntax* e0 = parse_additive_expression();
    while (Token op = match_if(is_relational_operator)) {
      Syntax* e1 = parse_additive_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  static bool is_additive_operator(Token::Kind k)
  {
    return k == Token::plus_tok ||
           k == Token::dash_tok;
  }

  /// Parse an additive expression.
  ///
  ///   additive-expression:
  ///     multiplicative-expression
  ///     additive-expression < multiplicative-expression
  ///     additive-expression > multiplicative-expression
  Syntax* Parser::parse_additive_expression()
  {
    Syntax* e0 = parse_multiplicative_expression();
    while (Token op = match_if(is_additive_operator)) {
      Syntax* e1 = parse_multiplicative_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  static bool is_multiplicative_operator(Token::Kind k)
  {
    return k == Token::star_tok ||
           k == Token::slash_tok ||
           k == Token::percent_tok;
  }

  /// Parse a multiplicative expression.
  ///
  ///   multiplicative-expression:
  ///     prefix-exprssion
  ///     multiplicative-expression < prefix-exprssion
  ///     multiplicative-expression > prefix-exprssion
  Syntax* Parser::parse_multiplicative_expression()
  {
    Syntax* e0 = parse_prefix_expression();
    while (Token op = match_if(is_multiplicative_operator)) {
      Syntax* e1 = parse_prefix_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Parse a prefix-expression.
  ///
  ///   prefix-expression:
  ///     postfix-expression
  ///     array [ expression-list? ] prefix-expression
  ///     templ [ expression-list? ] prefix-expression
  ///     func ( expression-list? ) prefix-expression
  ///     const prefix-exprssion
  ///     ^ prefix-expression
  ///     - prefix-expression
  ///     + prefix-expression
  ///     not prefix-expression
  ///
  /// NOTE: This is the minimal version of a grammar that both avoids extra
  /// lookahead and permits expressions and types to occupy the same grammar.
  /// We could add extra annotations after template and function type
  /// constructors (e.g., `func(int)->int`), but they aren't strictly necessary.
  ///
  /// TODO: The name array is somewhat unfortunate, since it makes a nice
  /// library structure. If arrays in this (or whatever) language had regular
  /// semantics, we probably wouldn't need the data type.
  ///
  /// Eliminating the leading keyword and adding an annotation before the prefix
  /// expression also works (e.g., (int)->int), but requires lookahead to
  /// disambiguate the enclosure from primary expressions. It also means that
  /// we can't parse implications, unless we had some way of explicitly
  /// characterizing the leading parameter list as something other than a
  /// primary expression.
  Syntax* Parser::parse_prefix_expression()
  {
    // TODO: Save tokens for type constructors.
    switch (lookahead())
    {
    case Token::array_tok: {
      Token tok = consume();
      Syntax* bound = parse_bracket_list();
      Syntax* type = parse_prefix_expression();
      return new Array_syntax(tok, bound, type);
    }

    case Token::templ_tok: {
      Token tok = consume();
      Syntax* parms = parse_bracket_list();
      Syntax* result = parse_prefix_expression();
      return new Template_syntax(tok, parms, result);
    }

    case Token::func_tok: {
      Token tok = consume();
      Syntax* parms = parse_paren_list();
      Syntax* result = parse_prefix_expression();
      return new Function_syntax(tok, parms, result);
    }

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

  /// Parse a postfix-expression.
  ///
  ///   postfix-expression:
  ///     primary-expression
  ///     postfix-expression ( expression-list )
  ///     postfix-expression [ expression-list ]
  ///     postfix-expression .
  ///     postfix-expression ^
  Syntax* Parser::parse_postfix_expression()
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
      else if (Token op = match(Token::caret_tok)) {
        e0 = new Postfix_syntax(op, e0);
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
  ///     identifier
  ///     ( expression-list? )
  ///     [ expression-list? ]
  ///     id-expression
  Syntax* Parser::parse_primary_expression()
  {
    switch (lookahead()) {
      // Value literals
    case Token::true_tok:
    case Token::false_tok:
    case Token::integer_tok:
    // Type literals
    case Token::int_tok:
    case Token::bool_tok:
    case Token::type_tok: {
      Token value = consume();
      return new Literal_syntax(value);
    }

    case Token::identifier_tok:
      return parse_id_expression();

    case Token::lparen_tok:
      return parse_tuple_expression();

    case Token::lbracket_tok:
      return parse_list_expression();

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

  /// Parse a tuple-expression.
  ///
  ///   tuple-expression:
  ///     paren-list
  Syntax* Parser::parse_tuple_expression()
  {
    return parse_paren_list();
  }

  /// Parse a list-expression.
  ///
  ///   list-expression:
  ///     paren-list
  Syntax* Parser::parse_list_expression()
  {
    return parse_bracket_list();
  }

  /// Parse an id-expression
  ///
  ///   id-expression:
  ///     identifier
  Syntax* Parser::parse_id_expression()
  {
    Token id = expect(Token::identifier_tok);
    return new Identifier_syntax(id);
  }

  enum class Enclosure
  {
    parens,
    brackets,
    braces,
  };

  struct Enclosing_tokens
  {
    Token::Kind open;
    Token::Kind close;
  };

  static constexpr Enclosing_tokens enclosing_toks[] = {
    { Token::lparen_tok, Token::rparen_tok },
    { Token::lbracket_tok, Token::rbracket_tok },
    { Token::lbrace_tok, Token::rbrace_tok },
  };

  static constexpr Token::Kind open_token(Enclosure e)
  {
    return enclosing_toks[(int)e].open;
  }

  static constexpr Token::Kind close_token(Enclosure e)
  {
    return enclosing_toks[(int)e].close;
  }

  /// Parse a list enclosed by the tokens of E. Note that a list
  /// is comprised of groups, so that's allowed.
  template<Enclosure E>
  static Syntax* parse_enclosed_list(Parser& p)
  {
    Token open = p.require(open_token(E));
    Syntax* t = nullptr;
    if (p.next_token_is_not (Token::rparen_tok))
      t = p.parse_expression_group();
    Token close = p.match(close_token(E));
    return new Enclosure_syntax(open, close, t);
  }

  /// Parse a paren-enclosed list.
  ///
  ///   paren-list:
  ///     ( expression-list? )
  Syntax* Parser::parse_paren_list()
  {
    return parse_enclosed_list<Enclosure::parens>(*this);
  }

  /// Parse a bracket-enclosed list.
  ///
  ///   bracket-list:
  ///     [ expression-list? ]
  Syntax* Parser::parse_bracket_list()
  {
    return parse_enclosed_list<Enclosure::brackets>(*this);
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
  ///   expression-group:
  ///     expression-list
  ///     expression-group ; expression-list
  ///
  /// Groups are only created if multiple groups are present.
  Syntax* Parser::parse_expression_group()
  {
    std::vector<Syntax*> ts;
    parse_item(*this, &Parser::parse_expression_list, ts);
    while (match(Token::semicolon_tok))
      parse_item(*this, &Parser::parse_expression_list, ts);
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

  /// Parse an expression-list.
  ///
  ///   expression-list:
  ///     parameter-expression
  ///     expression-list , parameter-expression
  ///
  /// This always returns a list, even if there's a single element.
  Syntax* Parser::parse_expression_list()
  {
    std::vector<Syntax*> ts;
    parse_item(*this, &Parser::parse_parameter_expression, ts);
    while (match(Token::comma_tok))
      parse_item(*this, &Parser::parse_parameter_expression, ts);
    return make_list(ts);
  }

  /// Returns true if `p` starts a parameter declaration.
  static bool starts_parameter(Parser& p)
  {
    return p.next_token_is(Token::colon_tok) ||
           p.next_tokens_are(Token::identifier_tok, Token::colon_tok);
  }

  /// Parse a parameter or expression.
  ///
  ///   parameter-expression:
  ///     parameter
  ///     expression
  Syntax* Parser::parse_parameter_expression()
  {
    if (starts_parameter(*this))
      return parse_parameter();
    return parse_expression();
  }

  void Parser::diagnose_expected(char const* what)
  {
    std::stringstream ss;
    ss << input_location() << ": "
       << "expected '" << what 
       << "' but got '" << peek().spelling() << "'";
    throw std::runtime_error(ss.str());
  }

  void Parser::diagnose_expected(Token::Kind k)
  {
    diagnose_expected(Token::spelling(k));
  }

  void Parser::debug(char const* msg)
  {
    std::cerr << msg << ": " << input_location() << ": " << peek() << '\n';   
  }

} // namespace beaker
