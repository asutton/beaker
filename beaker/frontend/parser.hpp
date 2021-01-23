#ifndef BEAKER_FRONTEND_PARSER_HPP
#define BEAKER_FRONTEND_PARSER_HPP

#include <beaker/language/translation.hpp>
#include <beaker/frontend/lexer.hpp>
#include <beaker/frontend/token.hpp>

#include <filesystem>

namespace beaker
{
  /// Constructs a concrete syntax tree from a source file.
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
      return peek().location();
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
        consume();
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

    void parse_file();

    void parse_declaration_seq();
    void parse_declaration();
    void parse_definition();
    void parse_parameter();

    void parse_declarator();

    void parse_type();

    void parse_expression();
    void parse_implication_expression();
    void parse_logical_or_expression();
    void parse_logical_and_expression();
    void parse_equality_expression();
    void parse_relational_expression();
    void parse_additive_expression();
    void parse_multiplicative_expression();
    void parse_prefix_expression();
    void parse_postfix_expression();
    void parse_primary_expression();
    void parse_tuple_expression();
    void parse_list_expression();
    void parse_id_expression();
    void parse_parameter_expression();

    void parse_paren_list();
    void parse_bracket_list();
    void parse_expression_group();
    void parse_expression_list();

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
