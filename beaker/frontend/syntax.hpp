#ifndef BEAKER_FRONTEND_SYNTAX_HPP
#define BEAKER_FRONTEND_SYNTAX_HPP

#include <beaker/frontend/token.hpp>

#include <vector>
#include <span>

namespace beaker
{
  /// The base class of all concrete syntax trees.
  ///
  /// Note that syntax is always a tree, it is not a graph. One implication
  /// is that syntax trees can be readily destroyed (i.e., it's safe for their
  /// destructors to deallocate memory).
  struct Syntax
  {
    enum Kind
    {
#define def_syntax(T, B) \
      T,
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

    /// Returns a span over the children of this node.
    std::span<Syntax const* const> children() const;
    
    /// Returns a span over the children of this node.
    std::span<Syntax*> children();

    /// Returns the source range of the tree.
    Source_range location() const;

    /// Dump the tree to stderr.
    void dump() const;

    Kind m_kind;
  };

  /// A vector of syntax nodes.
  using Syntax_seq = std::vector<Syntax*>;
  
  /// A span of syntax nodes.
  using Syntax_span = std::span<Syntax*>;

  /// A span of constant syntax nodes.
  using Const_syntax_span = std::span<Syntax const* const>;

  // Term structure
  //
  // The following classes provided basic structure for specific terms. The
  // intent is to simplify some algorithms that can be largely defined in
  // terms of the tree's structure, and not its interpretation.

  /// A node with a fixed number of operands.
  template<std::size_t N>
  struct Kary_syntax : Syntax
  {
    static_assert(N > 0);

    template<typename... Ts>
    Kary_syntax(Kind k, Ts*... ts)
      : Syntax(k), m_terms{ts...}
    {
      static_assert(sizeof...(Ts) == N);
    }

    /// Returns the nth operand.
    Syntax* operand(std::size_t n) const
    {
      return m_terms[n];
    }

    /// Returns the operands.
    Const_syntax_span operands() const
    {
      return m_terms;
    }

    /// Returns the operands.
    Syntax_span operands()
    {
      return m_terms;
    }

    Syntax* m_terms[N];
  };

  /// Specialization for unary nodes.
  template<>
  struct Kary_syntax<1> : Syntax
  {
    Kary_syntax(Kind k, Syntax* s)
      : Syntax(k), m_term(s)
    { }

    /// Returns the operand.
    Syntax* operand() const
    {
      return m_term;
    }

    /// Returns the operands.
    Const_syntax_span operands() const
    {
      return {&m_term, 1};
    }

    /// Returns the operands.
    Syntax_span operands()
    {
      return {&m_term, 1};
    }

    Syntax* m_term;
  };

  /// A unary expression has a single operand.
  struct Unary_syntax : Kary_syntax<1>
  {
    Unary_syntax(Kind k, Syntax* s)
      : Kary_syntax<1>(k, s)
    { }
  };

  /// A binary expression with two operands.
  struct Binary_syntax : Kary_syntax<2>
  {
    Binary_syntax(Kind k, Syntax* s0, Syntax* s1)
      : Kary_syntax<2>(k, s0, s1)
    { }
  };

  /// A ternary expression.
  struct Ternary_syntax : Kary_syntax<3>
  {
    Ternary_syntax(Kind k, Syntax* s0, Syntax* s1, Syntax* s2)
      : Kary_syntax<3>(k, s0, s1, s2)
    { }
  };

  /// A quaternary expression.
  struct Quaternary_syntax : Kary_syntax<4>
  {
    Quaternary_syntax(Kind k, Syntax* s0, Syntax* s1, Syntax* s2, Syntax* s3)
      : Kary_syntax<4>(k, s0, s1, s2, s3)
    { }
  };

  /// A term with an unspecified number of operands.
  struct Multiary_syntax : Syntax
  {
    Multiary_syntax(Kind k, Syntax_seq const& s)
      : Syntax(k), m_terms(s)
    { }

    Multiary_syntax(Kind k, Syntax_seq&& s)
      : Syntax(k), m_terms(std::move(s))
    { }

    /// Returns the nth operand.
    Syntax* operand(std::size_t n) const
    {
      return m_terms[n];
    }

    /// Returns the operands.
    Const_syntax_span operands() const
    {
      return m_terms;
    }

    /// Returns the operands.
    Syntax_span operands()
    {
      return m_terms;
    }

    Syntax_seq m_terms;
  };

  // Specific trees

  /// Any tree represented by a single token.
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

    /// Returns the spelling of the atom.
    std::string const& spelling() const
    {
      return m_tok.spelling();
    }

    Token m_tok;
  };

  /// Represents literal values.
  struct Literal_syntax : Atom_syntax
  {
    static constexpr Kind this_kind = Literal;

    Literal_syntax(Token tok)
      : Atom_syntax(this_kind, tok)
    { }
  };

  /// Represents user-defined names.
  struct Identifier_syntax : Atom_syntax
  {
    static constexpr Kind this_kind = Identifier;

    Identifier_syntax(Token tok)
      : Atom_syntax(this_kind, tok)
    { }
  };

  /// A sequence of delimited terms.
  ///
  /// TODO: This doesn't store the delimiters. I'm not sure if that's
  /// actually important.
  struct List_syntax : Multiary_syntax
  {
    static constexpr Kind this_kind = List;

    List_syntax(Syntax_seq const& s)
      : Multiary_syntax(this_kind, s)
    { }

    List_syntax(Syntax_seq&& s)
      : Multiary_syntax(this_kind, std::move(s))
    { }
  };

  /// A sequence of terms.
  struct Sequence_syntax : Multiary_syntax
  {
    static constexpr Kind this_kind = Sequence;

    Sequence_syntax(Syntax_seq const& s)
      : Multiary_syntax(this_kind, s)
    { }

    Sequence_syntax(Syntax_seq&& s)
      : Multiary_syntax(this_kind, std::move(s))
    { }
  };

  /// A term enclosed by a pair of tokens.
  struct Enclosure_syntax : Unary_syntax
  {
    static constexpr Kind this_kind = Enclosure;

    Enclosure_syntax(Token o, Token c, Syntax* t)
      : Unary_syntax(this_kind, t), m_open(o), m_close(c)
    { }

    /// Returns the opening token.
    Token open() const
    {
      return m_open;
    }

    /// Returns the closing token.
    Token close() const
    {
      return m_close;
    }

    /// Returns the inner term.
    Syntax* term() const
    {
      return m_term;
    }

    Token m_open;
    Token m_close;
  };

  /// A pair of terms.
  struct Pair_syntax : Binary_syntax
  {
    static constexpr Kind this_kind = Pair;

    Pair_syntax(Syntax* s0, Syntax* s1)
      : Binary_syntax(this_kind, s0, s1)
    { }
  };

  /// A triple of terms.
  struct Triple_syntax : Ternary_syntax
  {
    static constexpr Kind this_kind = Triple;

    Triple_syntax(Syntax* s0, Syntax* s1, Syntax* s2)
      : Ternary_syntax(this_kind, s0, s1, s2)
    { }
  };

  /// A unary prefix operator expression.
  struct Prefix_syntax : Unary_syntax
  {
    static constexpr Kind this_kind = Prefix;

    Prefix_syntax(Token tok, Syntax* s)
      : Unary_syntax(this_kind, s), m_op(tok)
    { }
  
    /// Returns the prefix operation (operator).
    Token operation() const
    {
      return m_op;
    }

    Token m_op;
  };

  /// Compound type constructors for arrays, templates, and functions.
  struct Constructor_syntax : Binary_syntax
  {
    Constructor_syntax(Kind k, Syntax* s, Syntax* r)
      : Binary_syntax(k, s, r)
    { }

    /// Returns the "constructor" of a constructor. Either an array bound
    /// or a parameter list.
    Syntax *constructor() const
    {
      return operand(0);
    }

    /// Returns the result type of the constructor.
    Syntax *result() const
    {
      return operand(1);
    }
  };

  /// Array type constructor.
  struct Array_syntax : Constructor_syntax
  {
    static constexpr Kind this_kind = Array;

    Array_syntax(Syntax* s, Syntax* t)
      : Constructor_syntax(this_kind, s, t)
    { }

    /// Returns the array bound.
    Syntax* bounds() const
    {
      return constructor();
    }

    using Constructor_syntax::Constructor_syntax;
  };

  /// Mapping type constructors (templates and functions).
  struct Mapping_syntax : Constructor_syntax
  {
    Mapping_syntax(Kind k, Syntax* p, Syntax* r)
      : Constructor_syntax(k, p, r)
    { }

    /// Returns the parameters.
    Syntax *parameters() const
    {
      return constructor();
    }
  };

  /// Function type constructor.
  struct Function_syntax : Mapping_syntax
  {
    static constexpr Kind this_kind = Function;

    Function_syntax(Syntax* p, Syntax* r)
      : Mapping_syntax(this_kind, p, r)
    { }
  };

  /// Template type constructor.
  struct Template_syntax : Mapping_syntax
  {
    static constexpr Kind this_kind = Template;

    Template_syntax(Syntax* p, Syntax* r)
      : Mapping_syntax(this_kind, p, r)
    { }
  };

  /// Unary postfix operators.
  struct Postfix_syntax : Unary_syntax
  {
    static constexpr Kind this_kind = Postfix;

    Postfix_syntax(Token tok, Syntax* s)
      : Unary_syntax(this_kind, s), m_op(tok)
    { }

    /// Returns the prefix operation (operator).
    Token operation() const
    {
      return m_op;
    }

    Token m_op;
};

  /// Applies arguments to a function, template, or array.
  struct Application_syntax : Binary_syntax
  {
    Application_syntax(Kind k, Syntax* e, Syntax* a)
      : Binary_syntax(k, e, a)
    { }

    /// Returns term being applied to arguments.
    Syntax* applicant() const
    {
      return m_terms[0];
    }

    /// Returns the arguments of the call.
    Syntax* arguments() const
    {
      return m_terms[1];
    }
  };

  /// Represents a function call.
  struct Call_syntax : Application_syntax
  {
    static constexpr Kind this_kind = Call;

    Call_syntax(Syntax* s0, Syntax* s1)
      : Application_syntax(this_kind, s0, s1)
    { }
  };

  /// Represents indexing into a table.
  struct Index_syntax : Application_syntax
  {
    static constexpr Kind this_kind = Index;

    Index_syntax(Syntax* s0, Syntax* s1)
      : Application_syntax(this_kind, s0, s1)
    { }
  };

  /// Infix binary operators.
  struct Infix_syntax : Binary_syntax
  {
    static constexpr Kind this_kind = Infix;

    Infix_syntax(Token t, Syntax* l, Syntax* r)
      : Binary_syntax(this_kind, l, r), m_op(t)
    { }

    /// Returns the infix operation (operator).
    Token operation() const
    {
      return m_op;
    }

    /// Returns the left-hand operand.
    Syntax* lhs() const
    {
      return m_terms[0];
    }

    /// Returns the right-hand operand.
    Syntax* rhs() const
    {
      return m_terms[1];
    }

    Token m_op;
  };

  /// Control structures. Control structures are primarily defined by two
  /// syntactic structures: head and body. The "head" part introduces variables,
  /// defines conditons, etc. The "body" is the main sub-expression.
  ///
  /// TODO: We might need to expand this for specific kinds of control
  /// structures. Lambda expressions don't readily fit the pattern unless we
  /// encode them cleverly using pairs.
  struct Control_syntax : Binary_syntax
  {
    static constexpr Kind this_kind = Control;

    Control_syntax(Token t, Syntax* c, Syntax* b)
      : Binary_syntax(this_kind, c, b), m_ctrl(t)
    { }

    /// Returns the infix operation (operator).
    Token control() const
    {
      return m_ctrl;
    }

    /// Returns the head of the control.
    Syntax* head() const
    {
      return m_terms[0];
    }

    /// Returns the body of the control.
    Syntax* body() const
    {
      return m_terms[1];
    }

    Token m_ctrl;
  };


  /// A definition or parameter. It has:
  ///
  ///   - a declarator (either identifier or list thereof)
  ///   - a type
  ///   - a constraint
  ///   - an initializer
  struct Declaration_syntax : Quaternary_syntax
  {
    static constexpr Kind this_kind = Declaration;

    Declaration_syntax(Syntax* d, Syntax* t, Syntax* c, Syntax* i)
      : Quaternary_syntax(this_kind, d, t, c, i)
    { }

    /// Returns the declarator.
    Syntax* declarator() const
    {
      return operand(0);
    }

    /// Returns the type.
    Syntax* type() const
    {
      return operand(1);
    }

    Syntax* constraint() const
    {
      return operand(2);
    }
    
    /// Returns the initializer.
    Syntax* initializer() const
    {
      return operand(3);
    }
  };

  /// The top-level container of terms.
  struct File_syntax : Unary_syntax
  {
    static constexpr Kind this_kind = File;

    File_syntax(Syntax* ds)
      : Unary_syntax(this_kind, ds)
    { }

    /// Returns the sequence of declarations in the file.
    Syntax* declarations() const
    {
      return operand();
    }
  };

  // Visitors

  enum Visitor_type
  {
    mutable_visitor,
    const_visitor
  };
  
  /// Defines the structure of visitors. This is a CRTP class. D is the derived
  /// implementation. MF is a metafunction that when used as MF<T>::type yields
  /// a (possibly cv-qualified) pointer to T. R is the result type, and Parms is
  /// a sequence of additional parameters.
  ///
  /// Stolen from clang.
  ///
  /// TODO: Factor a bunch of this stuff into a common library for visiting
  /// ASTs.
  template<typename D, Visitor_type V, typename R, typename... Parms>
  struct Syntax_visitor_base
  {
    // A helper to access the pointer type.
    template<typename T>
    using Fn = std::conditional_t<V == const_visitor,
                                  std::add_pointer<T const>,
                                  std::add_pointer<T>>;

    // Applies the function to generate the pointer.
    template<typename T>
    using Ptr = typename Fn<T>::type;

    /// Returns the derived class object.
    D* derived()
    {
      return static_cast<D*>(this);
    }

    // By default, returns a default-constructed `R`.
    R visit_Syntax(Ptr<Syntax> s, Parms... parms)
    {
      return R();
    }

    // Visit syntax of the particular kind. This includes visitors for
    // non-leaf bases (e.g., vector, constructor, and application).

#define def_syntax(T, B) \
    R visit_## T(Ptr<T ## _syntax> s, Parms... parms) \
    { \
      return derived()->visit_ ## B(s, parms...); \
    }
#define def_abstract(T, B) def_syntax(T, B)
#include <beaker/frontend/syntax.def>

    // Dispatch to one of the functions above.
    R visit(Ptr<Syntax> s, Parms... parms)
    {
      assert(s);
      switch (s->kind()) {
#define def_syntax(T, B) \
      case Syntax::T: \
        return derived()->visit_ ## T(static_cast<Ptr<T ## _syntax>>(s), parms...);
#include <beaker/frontend/syntax.def>
      default:
        break;
      }
      assert(false);
    }
  };

  /// Visits syntax.
  template<typename D, typename R, typename... Parms>
  struct Syntax_visitor : Syntax_visitor_base<D, mutable_visitor, R, Parms...>
  {
  };

  // Visits const syntax.
  template<typename D, typename R, typename... Parms>
  struct Const_syntax_visitor : Syntax_visitor_base<D, const_visitor, R, Parms...>
  {
  };

} // namespace beaker


#endif
