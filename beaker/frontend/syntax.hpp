#ifndef BEAKER_FRONTEND_SYNTAX_HPP
#define BEAKER_FRONTEND_SYNTAX_HPP

#include <beaker/frontend/token.hpp>

#include <vector>
#include <iostream>

namespace beaker
{
  /// The base class of all concrete syntax trees.
  struct Syntax
  {
    enum Kind
    {
#define def_syntax(T, K) \
      K ## _kind,
#include <beaker/frontend/syntax.def>
    };

    Syntax(Kind k)
      : m_kind(k)
    { }

    /// Returns the kind of syntax.
    Kind kind() const
    {
      return m_kind;
    }

    /// Returns the kind name.
    char const* kind_name() const;

    /// Returns the class name.
    char const* class_name() const;

    Kind m_kind;
  };

  using Syntax_seq = std::vector<Syntax*>;

  /// Any tree defined by a single atom.
  struct Atom_syntax : Syntax
  {
    Atom_syntax(Kind k, Token tok)
      : Syntax(k), m_tok(tok)
    { }

    /// Returns the token of the atom.
    Token token() const
    {
      return m_tok;
    }

    Token m_tok;
  };

  /// Represents literal values.
  struct Literal_syntax : Atom_syntax
  {
    static constexpr Kind this_kind = literal_kind;

    Literal_syntax(Token tok)
      : Atom_syntax(this_kind, tok)
    { }
  };

  /// Represents user-defined names.
  struct Identifier_syntax : Atom_syntax
  {
    static constexpr Kind this_kind = identifier_kind;

    Identifier_syntax(Token tok)
      : Atom_syntax(this_kind, tok)
    { }
  };


  /// A sequence of terms.
  struct Vector_syntax : Syntax
  {
    Vector_syntax(Kind k, Syntax_seq const& s)
      : Syntax(k), m_terms(s)
    { }

    Vector_syntax(Kind k, Syntax_seq&& s)
      : Syntax(k), m_terms(std::move(s))
    { }

    /// Returns the terms in the list.
    Syntax_seq& terms()
    {
      return m_terms;
    }

    /// Returns the terms in the list.
    Syntax_seq const& terms() const
    {
      return m_terms;
    }

    Syntax_seq m_terms;
  };

  /// A sequence of delimited terms.
  ///
  /// TODO: This doesn't store the delimiters. I'm not sure if that's
  /// actually important.
  struct List_syntax : Vector_syntax
  {
    static constexpr Kind this_kind = list_kind;

    List_syntax(Syntax_seq const& s)
      : Vector_syntax(this_kind, s)
    { }

    List_syntax(Syntax_seq&& s)
      : Vector_syntax(this_kind, std::move(s))
    { }
  };

  /// A sequence of terms.
  struct Sequence_syntax : Vector_syntax
  {
    static constexpr Kind this_kind = sequence_kind;

    Sequence_syntax(Syntax_seq const& s)
      : Vector_syntax(this_kind, s)
    { }

    Sequence_syntax(Syntax_seq&& s)
      : Vector_syntax(this_kind, std::move(s))
    { }
  };

  /// A term enclosed by a pair of tokens.
  struct Enclosure_syntax : Syntax
  {
    static constexpr Kind this_kind = enclosure_kind;

    Enclosure_syntax(Token o, Token c, Syntax* t)
      : Syntax(this_kind), open(o), close(c), term(t)
    { }

    Token open;
    Token close;
    Syntax* term;
  };

  /// A unary prefix exprssion.
  struct Prefix_syntax : Syntax
  {
    static constexpr Kind this_kind = prefix_kind;

    Prefix_syntax(Token tok, Syntax* s)
      : Syntax(this_kind), op(tok), term(s)
    { }

    Token op;
    Syntax* term;
  };

  /// Compound type constructors.
  struct Constructor_syntax : Syntax
  {
    Constructor_syntax(Kind k, Token tok, Syntax* a, Syntax* s)
      : Syntax(k), ctor(tok), args(a), term(s)
    { }

    Token ctor;
    Syntax* args;
    Syntax* term;
  };

  /// Array type constructor.
  struct Array_syntax : Constructor_syntax
  {
    static constexpr Kind this_kind = array_kind;

    Array_syntax(Token tok, Syntax* a, Syntax* s)
      : Constructor_syntax(this_kind, tok, a, s)
    { }

    using Constructor_syntax::Constructor_syntax;
  };

  /// Function type constructor.
  struct Function_syntax : Constructor_syntax
  {
    static constexpr Kind this_kind = function_kind;

    Function_syntax(Token tok, Syntax* a, Syntax* s)
      : Constructor_syntax(this_kind, tok, a, s)
    { }

    using Constructor_syntax::Constructor_syntax;
  };

  /// Template type constructor.
  struct Template_syntax : Constructor_syntax
  {
    static constexpr Kind this_kind = template_kind;

    Template_syntax(Token tok, Syntax* a, Syntax* s)
      : Constructor_syntax(this_kind, tok, a, s)
    { }

    using Constructor_syntax::Constructor_syntax;
  };

  /// Unary postfix operators.
  struct Postfix_syntax : Syntax
  {
    static constexpr Kind this_kind = postfix_kind;

    Postfix_syntax(Token tok, Syntax* s)
      : Syntax(this_kind), op(tok), term(s)
    { }

    Token op;
    Syntax* term;
  };

  /// A postfix operator applied to an array, function, or template.
  /// This "eliminates" its compound term, yielding a single value.
  struct Destructor_syntax : Syntax
  {
    Destructor_syntax(Kind k, Syntax* s0, Syntax* s1)
      : Syntax(k), term(s0), args(s1)
    { }

    Syntax* term;
    Syntax* args;
  };

  /// Represents a function call.
  struct Call_syntax : Destructor_syntax
  {
    static constexpr Kind this_kind = call_kind;

    Call_syntax(Syntax* s0, Syntax* s1)
      : Destructor_syntax(this_kind, s0, s1)
    { }
  };

  /// Represents indexing into a table.
  struct Index_syntax : Destructor_syntax
  {
    static constexpr Kind this_kind = index_kind;

    Index_syntax(Syntax* s0, Syntax* s1)
      : Destructor_syntax(this_kind, s0, s1)
    { }
  };

  /// Infix binary operators.
  struct Infix_syntax : Syntax
  {
    static constexpr Kind this_kind = infix_kind;

    Infix_syntax(Token t, Syntax* s0, Syntax* s1)
      : Syntax(this_kind), op(t), lhs(s0), rhs(s1)
    { }

    Token op;
    Syntax* lhs;
    Syntax* rhs;
  };

  /// A declaration.
  struct Declaration_syntax : Syntax
  {
    static constexpr Kind this_kind = declaration_kind;

    Declaration_syntax(Syntax* d, Syntax* t, Syntax* i)
      : Syntax(this_kind), decl(d), type(t), init(i)
    { }

    Syntax* decl;
    Syntax* type;
    Syntax* init;
  };

  /// The top-level container of terms.
  struct File_syntax : Syntax
  {
    static constexpr Kind this_kind = file_kind;

    File_syntax(Syntax* ds)
      : Syntax(this_kind), m_decls(ds)
    { }

    /// Returns the sequence of declarations in the file.
    Syntax* declarations() const
    {
      return m_decls;
    }

    Syntax* m_decls;
  };

  // Visitors

  /// Defines the structure of visitors. This is a CRTP class. D is the derived
  /// implementation. MF is a metafunction that when used as MF<T>::type yields
  /// a (possibly cv-qualified) pointer to T. R is the result type, and Parms is
  /// a sequence of additional parameters.
  ///
  /// Stolen from clang.
  template<typename D, template<typename> typename MF, typename R, typename... Parms>
  struct Syntax_visitor_base
  {
    // A helper to access the pointer type.
    template<typename T>
    using Ptr = typename MF<T>::type;

    /// Returns the derived class object.
    D* derived()
    {
      return static_cast<D*>(this);
    }

    // By default, returns a default-constructed `R`.
    R visit_syntax(Ptr<Syntax> s, Parms... parms)
    {
      return R();
    }

    // Generates a default overload for each syntax node of the
    // form R visit_<name>(<Name>* s, Parms... parms). The default
    // behavior falls back to visit(s, parms...).
#define def_syntax(T, K) \
    R visit_ ## K(Ptr<T ## _syntax> s, Parms... parms) \
    { \
      return derived()->visit_syntax(s, parms...); \
    }
#include <beaker/frontend/syntax.def>

    // Dispatch to one of the functions above.
    R visit(Ptr<Syntax> s, Parms... parms)
    {
      switch (s->kind()) {
#define def_syntax(T, K) \
      case Syntax::K ## _kind: \
        return derived()->visit_ ## K(static_cast<Ptr<T ## _syntax>>(s), parms...);
#include <beaker/frontend/syntax.def>
      default:
        break;
      }
      assert(false);
    }
  };

  // FIXME: This should move into a util location somewhere.

  template<typename T>
  struct make_nonconst_ptr : std::add_pointer<T>
  {
  };

  template<typename T>
  struct make_const_ptr : std::add_pointer<typename std::add_const<T>::type>
  {
  };

  /// Visits syntax.
  template<typename D, typename R, typename... Parms>
  struct Syntax_visitor : Syntax_visitor_base<D, make_nonconst_ptr, R, Parms...>
  {
  };

  // Visits const syntax.
  template<typename D, typename R, typename... Parms>
  struct Const_syntax_visitor : Syntax_visitor_base<D, make_const_ptr, R, Parms...>
  {
  };

  // Operations

  // Print a tree representation of the syntax.
  void dump_syntax(Syntax const* s);

} // namespace beaker


#endif
