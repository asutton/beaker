#ifndef BEAKER_FRONTEND_TOKEN_HPP
#define BEAKER_FRONTEND_TOKEN_HPP

#include <beaker/language/symbol.hpp>
#include <beaker/frontend/location.hpp>

#include <cassert>

namespace beaker
{
  struct Token
  {
    enum Kind
    {
#define def_token(K) \
      K ## _tok,
#include <beaker/frontend/token.def>
    };

    Token()
      : m_kind(eof_tok), m_sym(), m_loc()
    { }

    Token(Kind k, Symbol sym, Source_location loc = {})
      : m_kind(k), m_sym(sym), m_loc(loc)
    { }

    /// Converts to true if this is not the end-of-file.
    explicit operator bool() const
    {
      return !is_eof();
    }

    /// Returns the kind of token.
    Kind kind() const
    {
      return m_kind;
    }

    /// Returns a string representing the kind `k`.
    static const char* kind_name(Kind k);

    /// Returns a string representing the token's kind.
    char const* kind_name() const
    {
      return kind_name(m_kind);
    }

    /// Returns true if this is end-of-file.
    bool is_eof() const
    {
      return m_kind == eof_tok;
    }

    /// Returns true if this is a token has a single spelling.
    bool is_singleton() const;

    /// Returns true ift he token defines a class of equivalent spellings.
    bool is_class() const
    {
      return !is_singleton();
    }

    /// Returns the symbol of the token.
    Symbol symbol() const
    {
      return m_sym;
    }

    /// Returns the spelling for the single token `k`.
    static const char* spelling(Kind k);

    /// Returns the spelling of the token.
    std::string const& spelling() const
    {
      return m_sym.str();
    }

    /// Returns the start location.
    Source_location start_location() const
    {
      return m_loc;
    }

    // Returns the end location.
    Source_location end_location() const
    {
      return {m_loc.line, m_loc.column + m_sym.size()};
    }

    Kind m_kind;
    Symbol m_sym;
    Source_location m_loc;
  };

  std::ostream& operator<<(std::ostream& os, Token const& tok);

} // namespace beaker

#endif
