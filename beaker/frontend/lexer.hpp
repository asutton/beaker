#ifndef BEAKER_FRONTEND_LEXER_HPP
#define BEAKER_FRONTEND_LEXER_HPP

#include <beaker/language/translation.hpp>
#include <beaker/frontend/token.hpp>

#include <filesystem>

namespace beaker
{
  struct Lexer;

  /// The base class of all scanners used by the lexer.
  struct Scanner
  {
    Scanner(Lexer& lex, char const* first, char const* last)
      : m_lex(lex), m_first(first), m_last(last)
    { }

    /// Returns true if at the end of input.
    bool eof() const
    {
      return m_first == m_last;
    }

    /// Peek at the current character.
    char peek()
    {
      return m_first != m_last ? *m_first : 0;
    }

    /// Peek at the nth character past the current character.
    char peek(int n)
    {
      return (m_last - m_first > n) ? *(m_first + n) : 0;
    }

    /// Returns true if the current character is `c`.
    bool next_char_is(char c)
    {
      return peek() == c;
    }

    /// Returns true if the nth character past the current character is `c`.
    bool nth_char_is(int n, char c)
    {
      return peek(n) == c;
    }

    /// Returns true if the current character is not `c`.
    bool next_char_is_not(char c)
    {
      return peek() != c;
    }

    Lexer& m_lex;
    char const* m_first;
    char const* m_last;
  };

  /// Scans identifiers and keywords.
  struct Word_scanner : Scanner
  {
    using Scanner::Scanner;

    Token get();
  };

  /// Scans integers and floating point values.
  struct Number_scanner : Scanner
  {
    using Scanner::Scanner;

    Token get();
  };

  /// Scans punctuators and operators.
  struct Puncop_scanner : Scanner
  {
    using Scanner::Scanner;

    Token get();
  };

  /// Transforms the input text into tokens.
  struct Lexer
  {
    Lexer(Translation& trans, std::filesystem::path const& p);

    /// Returns the next token.
    Token get();

    /// Read all tokens into the output buffer.
    template<typename T>
    void get(T& out)
    {
      while (Token tok = get())
        out.push_back(tok);
    }

    /// Returns the current line/column of the lexer.
    Source_location input_location() const
    {
      return {m_line, m_pos - m_line_pos};
    }

    using Keyword_table = std::unordered_map<Symbol, Token::Kind>;

    Translation& m_trans;
    Keyword_table m_keywords;
    std::filesystem::path m_path;
    std::string m_text;
    std::size_t m_pos;
    std::size_t m_line_pos;
    std::size_t m_line;
  };

} // namespace beaker

#endif
