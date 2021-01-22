#ifndef BEAKER_LANGUAGE_TRANSLATION_HPP
#define BEAKER_LANGUAGE_TRANSLATION_HPP

#include <beaker/language/symbol.hpp>

namespace beaker
{
  /// Maintains language-level context for the translation and creation of
  /// Beaker programs.
  struct Translation
  {
    /// Returns the symbol table for this translation.
    Symbol_table& symbol_table()
    {
      return m_syms;
    }

    /// Returns a symbol for `str`.
    Symbol get_symbol(std::string const& str)
    {
      return m_syms.get(str);
    }

    /// Returns a symbol for `str`.
    Symbol get_symbol(char const* str)
    {
      return m_syms.get(str);
    }

    /// Returns a symbol for the characters is `[first, last)`.
    Symbol get_symbol(char const* first, char const* last)
    {
      return m_syms.get(first, last);
    }

    Symbol_table m_syms;
  };

} // namespace beaker


#endif
