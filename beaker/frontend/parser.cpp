#include <beaker/frontend/parser.hpp>

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

  /// Parse a file.
  ///
  ///   file:
  ///     declaration-seq?
  Syntax* Parser::parse_file()
  {
    // FIXME: Do something different for empty files?
    Syntax* s = parse_declaration_seq();
    return new File_syntax(s);
  }

  /// Parse a sequence of declarations.
  ///
  ///   declaration-seq
  ///     declaration
  ///     declaration-seq declaration
  Syntax* Parser::parse_declaration_seq()
  {
    Syntax_seq ss;
    while (!eof())
      parse_item(*this, &Parser::parse_declaration, ss);
    return new Sequence_syntax(std::move(ss));
  }

  /// Parse a declaration:
  ///
  ///   declaration:
  ///     definition
  Syntax* Parser::parse_declaration()
  {
    return parse_definition();
  }

  /// Returns true if the tokens at `la` start a definition.
  static bool starts_definition(Parser& p, std::size_t la)
  {
    // Check for unnamed definitions.
    if (p.nth_token_is(la, Token::colon_tok))
      return true;
    
    // Check for definitions of the form `x:` and `x,y,z:`
    if (p.nth_token_is(la, Token::identifier_tok)) {
      ++la;

      // Match a single declarator.
      if (p.nth_token_is(la, Token::colon_tok))
        return true;
      
      // Match multiple declarators. Basically, search through a comma-separated
      // list of identifiers and stop when we reach anything else. Note that
      // finding anying the like `x, 0` means we can short-circuit at the
      // 0. There's no way this can be a definition.
      while (p.nth_token_is(la, Token::comma_tok)) {
        ++la;
        if (p.nth_token_is(la, Token::identifier_tok))
          ++la;
        else
          return false;
      }

      return p.nth_token_is(la, Token::colon_tok);
    }

    return false;
  }

  /// Returns true if `p` starts a parameter declaration.
  static bool starts_definition(Parser& p)
  {
    return starts_definition(p, 0);
  }

  /// Definition declaration:
  ///
  ///   definition-declaration:
  ///     declarator-list? : type ;
  ///     declarator-list? : initializer
  ///     declarator-list? : type initializer
  ///
  ///   initializer:
  ///     = expression ;
  ///     { statement-list }
  ///
  /// TODO: Make sure this stays in line with the grammar.
  Syntax* Parser::parse_definition()
  {
    // Parse the declarator-list.
    Syntax* decl = nullptr;
    if (next_token_is_not(Token::colon_tok))
      decl = parse_declarator_list();

    // Parse the type.
    expect(Token::colon_tok);
    
    Syntax* type = nullptr;
    if (next_token_is_not(Token::equal_tok)) {
      type = parse_type();

      // Match the 'decl : type ;' case.
      if (match(Token::semicolon_tok))
        return new Declaration_syntax(decl, type, nullptr);

      // Fall through to parse the initializer.
    }

    // Parse the initializer.
    Syntax* init;
    if (match(Token::equal_tok)) {
      init = parse_expression();
      expect(Token::semicolon_tok);
    }
    else if (next_token_is(Token::lbrace_tok)) {
      init = parse_brace_list();
    }
    else {
      diagnose_expected("initializer");
    }

    return new Declaration_syntax(decl, type, init);
  }

  /// Parser a parameter:
  ///
  ///   parameter:
  ///     identifier : type
  ///     identifier : type = expression
  ///     identifeir : = expression
  ///     : type
  ///     : type = expression
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
      return new Declaration_syntax(nullptr, type, init);
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

    return new Declaration_syntax(id, type, init);
  }

  /// Builds the declarator list.
  static Syntax* make_declarator_list(Syntax_seq& ts)
  {
    // TODO: What if `ts` is empty? Recovery means skipping the entire
    // declaration, probably.
    assert(!ts.empty());

    // Collapse singleton lists into simple declarators.
    if (ts.size() == 1)
      return ts[0];

    return new List_syntax(std::move(ts));
  }

  /// Parse a declarator-list.
  ///   declarator-list:
  ///     declarator
  ///     declartor-list , declarator
  ///
  /// Technically, this allows the declaration of multiple functions having
  /// the same return type, but we can semantically limit declarators to just
  /// variables.
  Syntax* Parser::parse_declarator_list()
  {
    Syntax_seq ts;
    parse_item(*this, &Parser::parse_declarator, ts);
    while (match(Token::comma_tok))
      parse_item(*this, &Parser::parse_declarator, ts);
    return make_declarator_list(ts);
  }

  /// Parse a declarator.
  ///
  ///   declarator:
  ///     id-expression
  ///
  /// This is a restriction on a more general grammar that allows function
  /// and/or array-like declarators.
  Syntax* Parser::parse_declarator()
  {
    return parse_id_expression();
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
  ///     infix-expression
  ///     return infix-expression
  ///     yield infix-expression
  ///     throw infix-expression
  ///
  /// TODO: Actually implement throw and yield.
  Syntax* Parser::parse_expression()
  {
    switch (lookahead()) {
    case Token::return_tok: {
      Token tok = consume();
      Syntax* e = parse_infix_expression();
      return new Prefix_syntax(tok, e);
    }
    default:
      break;
    }
    return parse_infix_expression();
  }

  /// Parse an expression.
  ///
  ///   infix-expression:
  ///     implication-expression
  Syntax* Parser::parse_infix_expression()
  {
    return parse_assignment_expression();
  }

  /// Parse an assignment.
  ///
  ///   assignment-expression:
  ///     implication-expression
  ///     implication-expression = assignment-expression
  Syntax* Parser::parse_assignment_expression()
  {
    Syntax* e0 = parse_implication_expression();
    if (Token op = match(Token::equal_tok)) {
      Syntax* e1 = parse_assignment_expression();
      return new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Parse an implication.
  ///
  ///   implication-expression:
  ///     logical-or-expression
  ///     logical-or-expression -> implication-expression
  Syntax* Parser::parse_implication_expression()
  {
    Syntax* e0 = parse_logical_or_expression();
    if (Token op = match(Token::dash_greater_tok)) {
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
  ///     additive-expression + multiplicative-expression
  ///     additive-expression - multiplicative-expression
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
  ///     prefix-expression
  ///     multiplicative-expression * prefix-expression
  ///     multiplicative-expression / prefix-expression
  ///     multiplicative-expression % prefix-expression
  Syntax* Parser::parse_multiplicative_expression()
  {
    Syntax* e0 = parse_prefix_expression();
    while (Token op = match_if(is_multiplicative_operator)) {
      Syntax* e1 = parse_prefix_expression();
      e0 = new Infix_syntax(op, e0, e1);
    }
    return e0;
  }

  /// Returns true if we're at the start of a parameter list.
  static bool is_parameter_list(Parser& p)
  {
    assert(p.next_token_is(Token::lparen_tok) || p.next_token_is(Token::lbracket_tok));

    // Match an empty parameter list.
    if (p.nth_token_is(1, Token::rparen_tok))
      return true;

    // Otherwise, look to see if this contains a definition of one or more
    // parameters.
    return starts_definition(p, 1);
  }

  /// Find the matching offset of the current token.
  ///
  /// FIXME: With parameter groups, this isn't quite right. We need to
  /// skim over all top-level commas, looking parameters.
  ///
  /// TODO: It might be worthwhile combining this and the function below.
  /// Otherwise, we have multiple scans over the same tokens.
  static std::size_t find_matching(Parser& p, Token::Kind k1, Token::Kind k2)
  {
    assert(p.lookahead() == k1);
    std::size_t la = 0;
    std::size_t braces = 0;
    while (Token::Kind k = p.lookahead(la)) {
      if (k == k1) {
        ++braces;
      }
      else if (k == k2) {
        --braces;
        if (braces == 0)
          break;
      }
      ++la;
    }
    return la;
  }

  // An lparen starts a prefix operator if the first few tokens start a
  // prefix operator, and the entire enclosure is not followed by something
  // that is eithe a primary expression or other prefix operator.
  static bool is_prefix_operator(Parser& p)
  {
    if (!is_parameter_list(p))
      return false;

    std::size_t la = find_matching(p, Token::lparen_tok, Token::rparen_tok);
    switch (p.lookahead(la + 1)) {
      // Primary expressions.
      case Token::true_tok:
      case Token::false_tok:
      case Token::integer_tok:
      case Token::int_tok:
      case Token::bool_tok:
      case Token::type_tok:
      case Token::identifier_tok:
      case Token::lbrace_tok:
      // Both prefix and primary.
      case Token::lparen_tok:
      // Other prefix operators.
      case Token::lbracket_tok:
      case Token::const_tok:
      case Token::caret_tok:
      case Token::plus_tok:
      case Token::dash_tok:
      case Token::not_tok:
        return true;
      default:
        break;
    }
    return false;
  }

  /// Parse a prefix-expression.
  ///
  ///   prefix-expression:
  ///     postfix-expression
  ///     [ expression-group? ] prefix-expression
  ///     ( parameter-group? ) prefix-expression
  ///     const prefix-expression
  ///     ^ prefix-expression
  ///     - prefix-expression
  ///     + prefix-expression
  ///     not prefix-expression
  Syntax* Parser::parse_prefix_expression()
  {
    switch (lookahead())
    {
    case Token::lbracket_tok: {
      bool templ = is_parameter_list(*this);
      Syntax* bound = parse_bracket_group();
      Syntax* type = parse_prefix_expression();
      if (templ)
        return new Template_syntax(bound, type);
      return new Array_syntax(bound, type);
    }

    case Token::lparen_tok: {
      if (!is_prefix_operator(*this))
        break;
      Syntax* parms = parse_paren_list();
      Syntax* result = parse_prefix_expression();
      return new Function_syntax(parms, result);
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

  /// Parse a postfix-expression.
  ///
  ///   postfix-expression:
  ///     primary-expression
  ///     postfix-expression ( expression-list? )
  ///     postfix-expression [ expression-list? ]
  ///     postfix-expression . id-expression
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
  ///     id-expression
  ///     ( expression-list? )
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
      return parse_paren_list();
    
    case Token::lbrace_tok:
      return parse_brace_list();

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

  /// Parse an id-expression
  ///
  ///   id-expression:
  ///     identifier
  Syntax* Parser::parse_id_expression()
  {
    Token id = expect(Token::identifier_tok);
    return new Identifier_syntax(id);
  }

  /// Parse a paren-enclosed group.
  ///
  ///   paren-group:
  ///     ( expression-group? )
  Syntax* Parser::parse_paren_group()
  {
    return parse_enclosed<Enclosure::parens>(&Parser::parse_expression_group);
  }

  /// Parse a paren-enclosed list.
  ///
  ///   paren-list:
  ///     ( expression-list? )
  Syntax* Parser::parse_paren_list()
  {
    return parse_enclosed<Enclosure::parens>(&Parser::parse_expression_list);
  }

  /// Parse a bracket-enclosed group.
  ///
  ///   bracket-group:
  ///     [ expression-group? ]
  Syntax* Parser::parse_bracket_group()
  {
    return parse_enclosed<Enclosure::brackets>(&Parser::parse_expression_group);
  }

  /// Parse a bracket-enclosed list.
  ///
  ///   bracket-list:
  ///     [ expression-list? ]
  Syntax* Parser::parse_bracket_list()
  {
    return parse_enclosed<Enclosure::brackets>(&Parser::parse_expression_list);
  }

  /// Returns a list defining the group.
  static Syntax* make_group(Syntax_seq& ts)
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
    Syntax_seq ts;
    parse_item(*this, &Parser::parse_expression_list, ts);
    while (match(Token::semicolon_tok))
      parse_item(*this, &Parser::parse_expression_list, ts);
    return make_group(ts);
  }

  // Returns a list for `ts`.
  static Syntax* make_list(Syntax_seq& ts)
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
    Syntax_seq ts;
    parse_item(*this, &Parser::parse_parameter_or_expression, ts);
    while (match(Token::comma_tok))
      parse_item(*this, &Parser::parse_parameter_or_expression, ts);
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
  Syntax* Parser::parse_parameter_or_expression()
  {
    if (starts_parameter(*this))
      return parse_parameter();
    return parse_expression();
  }

  /// Returns a list defining the group.
  static Syntax* make_parameter_group(std::vector<Syntax*>& ts)
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
  Syntax* Parser::parse_parameter_group()
  {
    std::vector<Syntax*> ts;
    parse_item(*this, &Parser::parse_parameter_list, ts);
    while (match(Token::semicolon_tok))
      parse_item(*this, &Parser::parse_parameter_list, ts);
    return make_parameter_group(ts);
  }

  // Returns a list for `ts`.
  static Syntax* make_parameter_list(std::vector<Syntax*>& ts)
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
  Syntax* Parser::parse_parameter_list()
  {
    std::vector<Syntax*> ts;
    parse_item(*this, &Parser::parse_parameter, ts);
    while (match(Token::comma_tok))
      parse_item(*this, &Parser::parse_parameter, ts);
    return make_parameter_list(ts);
  }

  /// Parse a brace-enclosed list.
  ///
  ///   brace-list:
  ///     { statement-seq }
  Syntax* Parser::parse_brace_list()
  {
    return parse_enclosed<Enclosure::braces>(&Parser::parse_statement_seq);
  }

  /// Parse a statement sequence.
  Syntax* Parser::parse_statement_seq()
  {
    // Parse a statement and increment the count. This is used to allow
    // the omission of the a trailing semicolon on first statements.
    std::size_t num = 0;
    auto parse = [this, &num]() {
      return parse_statement(num++);
    };

    Syntax_seq ts;
    parse_item(parse, ts);
    while (next_token_is_not(Token::rbrace_tok))
      parse_item(parse, ts);

    return make_declarator_list(ts);
  }

  /// Parse a statement.
  ///
  ///   statement:
  ///     declaration-statement
  ///     return-statement
  ///     expression-statement
  Syntax* Parser::parse_statement(std::size_t n)
  {
    if (starts_definition(*this))
      return parse_declaration_statement(n);
    return parse_expression_statement(n);
  }

  /// Parse a declaration-statement.
  ///
  ///   declaration-statement:
  ///     declaration
  ///
  /// Not all declarations are allowed in all scopes. However, we don't
  /// really have a notion of scope attached to the parse, so we have to
  /// filter semantically.
  Syntax* Parser::parse_declaration_statement(std::size_t n)
  {
    return parse_declaration();
  }

  /// Parse an expression-statement.
  ///
  ///   expression-statement:
  ///     expression-list ;?
  ///
  /// The semicolon is required this is the first statement in a brace-list
  /// and the next token is `}`. That allows for brace-lists of the form
  /// `{ a, b, c }`.
  Syntax* Parser::parse_expression_statement(std::size_t n)
  {
    Syntax* e = parse_expression_list();
    if (n == 0 && next_token_is(Token::rbrace_tok))
      return e;
    expect(Token::semicolon_tok);
    return e;
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
