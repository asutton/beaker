#ifndef BEAKER_FRONTEND_PARSER_HPP
#define BEAKER_FRONTEND_PARSER_HPP

#include <beaker/language/translation.hpp>
#include <beaker/frontend/lexer.hpp>
#include <beaker/frontend/token.hpp>
#include <beaker/frontend/syntax.hpp>

#include <filesystem>

namespace beaker
{
  struct Syntax;

  /// Constructs a concrete syntax tree from a source file. This is the
  /// base class of experimental language parsers. The "main" entry point
  /// to various syntactic forms are defined as virtual functions to be
  /// overridden for various experiments.
  struct Parser
  {
    Parser(Translation& trans, std::filesystem::path const& p);

    // Token operations

    /// Returns true if we're at the end of file.
    bool eof() const
    {
      return m_pos == m_toks.size();
    }

    Source_location input_location()
    {
      return peek().start_location();
    }

    /// Peeks at the current token.
    Token peek() const
    {
      if (m_pos < m_toks.size())
        return m_toks[m_pos];
      return {};
    }

    /// Peeks at the nth token past the current token.
    Token peek(int n) const
    {
      if (m_pos + n < m_toks.size())
        return m_toks[m_pos + n];
      return {};
    }

    /// Returns the kind of the current token.
    Token::Kind lookahead() const
    {
      return peek().kind();
    }

    /// Returns the kind of the nth lookahead token.
    Token::Kind lookahead(int n) const
    {
      return peek(n).kind();
    }

    /// Returns true if the next token has kind `k`.
    bool next_token_is(Token::Kind k)
    {
      return lookahead() == k;
    }

    /// Returns true if the nth token has kind `k`.
    bool nth_token_is(int n, Token::Kind k)
    {
      return lookahead(n) == k;
    }

    /// Returns true if the next two tokens have kind `k1` and `k2`.
    bool next_tokens_are(Token::Kind k1, Token::Kind k2)
    {
      return next_token_is(k1) && nth_token_is(1, k2);
    }

    /// Returns true if the next token does not have kind `k`.
    bool next_token_is_not(Token::Kind k)
    {
      return lookahead() != k;
    }

    /// Consume the current, returning it.
    Token consume()
    {
      assert(m_pos < m_toks.size());
      Token tok = m_toks[m_pos];
      ++m_pos;
      return tok;
    }

    /// If the next token has kind `k`, consume the token.
    Token match(Token::Kind k)
    {
      if (next_token_is(k))
        return consume();
      return {};
    }

    /// If `pred(k)` is satisfied for the current lookahead, consume the token.
    template<typename P>
    Token match_if(P pred)
    {
      if (pred(lookahead()))
        return consume();
      return {};
    }

    /// Consume the next token if it has kind `k`, otherwise emit a diagnostic.
    Token expect(Token::Kind k)
    {
      if (next_token_is(k))
        return consume();
      diagnose_expected(k);
      return {};
    }

    /// Returns the current token, ensuring that it has kind `k`.
    Token require(Token::Kind k)
    {
      assert(next_token_is(k));
      return consume();
    }

    // Parsing

    // Top-level.
    virtual Syntax* parse_file();

    // Declarations.
    virtual Syntax* parse_declaration();
    Syntax* parse_definition();
    Syntax* parse_parameter();
    Syntax* parse_declaration_seq();

    // Declarators.
    virtual Syntax* parse_declarator();
    Syntax* parse_declarator_list();

    // Types.
    virtual Syntax* parse_type();

    // Expressions, in general.
    virtual Syntax* parse_expression();
    
    // Infix expressions.
    virtual Syntax* parse_infix_expression();
    virtual Syntax* parse_assignment_expression();
    virtual Syntax* parse_implication_expression();
    Syntax* parse_logical_or_expression();
    Syntax* parse_logical_and_expression();
    Syntax* parse_equality_expression();
    Syntax* parse_relational_expression();
    Syntax* parse_additive_expression();
    Syntax* parse_multiplicative_expression();
    
    // Prefix expressions.
    virtual Syntax* parse_prefix_expression();
    
    // Postfix expressions.
    virtual Syntax* parse_postfix_expression();
    
    // Primary expressions.
    virtual Syntax* parse_primary_expression();
    Syntax* parse_tuple_expression();
    Syntax* parse_list_expression();
    Syntax* parse_id_expression();

    // Helper grammars
    Syntax* parse_paren_list();
    Syntax* parse_paren_group();
    Syntax* parse_bracket_list();
    Syntax* parse_bracket_group();
    Syntax* parse_expression_group();
    Syntax* parse_expression_list();
    Syntax* parse_parameter_group();
    Syntax* parse_parameter_list();
    Syntax* parse_brace_list();
    Syntax* parse_parameter_or_expression();

    // Statements
    virtual Syntax* parse_statement(std::size_t n);
    Syntax* parse_declaration_statement(std::size_t n);
    Syntax* parse_expression_statement(std::size_t n);
    Syntax* parse_statement_seq();


    // Generic parsers and utilities

    /// A helper function for parsing items in a list or sequence.
    /// Accumulates the result in `ts`.
    template<typename P, typename F>
    static Syntax* parse_item(P& parser, F fn, Syntax_seq& ss)
    {
      // TODO: If we represent syntax errors explicitly, then
      // the parser will always return a non-null pointer.
      Syntax* s = (parser.*fn)();
      if (s)
        ss.push_back(s);
      return s;
    }

    /// Same as above, but takes a plain function, not a member function
    /// pointer.
    template<typename F>
    static Syntax* parse_item(F parse, Syntax_seq& ss)
    {
      // TODO: If we represent syntax errors explicitly, then
      // the parser will always return a non-null pointer.
      Syntax* s = parse();
      if (s)
        ss.push_back(s);
      return s;
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
    template<Enclosure E, typename F>
    Syntax* parse_enclosed(F fn)
    {
      Token open = require(open_token(E));
      Syntax* t = nullptr;
      if (next_token_is_not(close_token(E)))
        t = (this->*fn)();
      Token close = expect(close_token(E));
      return new Enclosure_syntax(open, close, t);
    }

    // Diagnostics

    void diagnose_expected(char const* what);
    void diagnose_expected(Token::Kind k);

    // Debugging

    void debug(char const* msg);

    Translation& m_trans;
    Lexer m_lex;
    std::vector<Token> m_toks;
    std::size_t m_pos;
  };

} // namespace beaker

#endif
