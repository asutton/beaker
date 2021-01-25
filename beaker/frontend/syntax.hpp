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

  /// A unary expression has a single operand.
  struct Unary_syntax : Syntax
  {
    Unary_syntax(Kind k, Syntax* s)
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

  /// A binary expression with two operands.
  struct Binary_syntax : Syntax
  {
    Binary_syntax(Kind k, Syntax* s0, Syntax* s1)
      : Syntax(k), m_terms{s0, s1}
    { }

    /// Returns the first operand.
    Syntax* first() const
    {
      return m_terms[0];
    }

    /// Returns the second operand.
    Syntax* second() const
    {
      return m_terms[1];
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

    Syntax* m_terms[2];
  };

  /// A ternary expression with three operands.
  struct Ternary_syntax : Syntax
  {
    Ternary_syntax(Kind k, Syntax* s0, Syntax* s1, Syntax* s2)
      : Syntax(k), m_terms{s0, s1, s2}
    { }

    /// Returns the first operand.
    Syntax* first() const
    {
      return m_terms[0];
    }

    /// Returns the second operand.
    Syntax* second() const
    {
      return m_terms[1];
    }

    /// Returns the third operand.
    Syntax* third() const
    {
      return m_terms[2];
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

    Syntax* m_terms[3];
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

  /// A sequence of delimited terms.
  ///
  /// TODO: This doesn't store the delimiters. I'm not sure if that's
  /// actually important.
  struct List_syntax : Multiary_syntax
  {
    static constexpr Kind this_kind = list_kind;

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
    static constexpr Kind this_kind = sequence_kind;

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
    static constexpr Kind this_kind = enclosure_kind;

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

  /// A unary prefix operator expression.
  struct Prefix_syntax : Unary_syntax
  {
    static constexpr Kind this_kind = prefix_kind;

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

  /// Compound type constructors.
  struct Constructor_syntax : Binary_syntax
  {
    Constructor_syntax(Kind k, Token tok, Syntax* s, Syntax* r)
      : Binary_syntax(k, s, r), m_ctor(tok)
    { }

    /// Returns the kind of constructor.
    Token type() const
    {
      return m_ctor;
    }

    /// Returns the "specifier" of a constructor. The interpretation of
    /// this operand depends on the kind of constructor.
    Syntax *specifier() const
    {
      return first();
    }

    /// Returns the result type of the constructor.
    Syntax *result() const
    {
      return second();
    }

    Token m_ctor;
  };

  /// Array type constructor.
  struct Array_syntax : Constructor_syntax
  {
    static constexpr Kind this_kind = array_kind;

    Array_syntax(Token tok, Syntax* s, Syntax* t)
      : Constructor_syntax(this_kind, tok, s, t)
    { }

    /// Returns the array bound.
    Syntax* bound() const
    {
      return specifier();
    }

    using Constructor_syntax::Constructor_syntax;
  };

  /// Mapping type constructors.
  struct Mapping_syntax : Constructor_syntax
  {
    Mapping_syntax(Kind k, Token tok, Syntax* p, Syntax* r)
      : Constructor_syntax(k, tok, p, r)
    { }

    /// Returns the parameters.
    Syntax *parameters() const
    {
      return specifier();
    }
  };

  /// Function type constructor.
  struct Function_syntax : Mapping_syntax
  {
    static constexpr Kind this_kind = function_kind;

    Function_syntax(Token tok, Syntax* p, Syntax* r)
      : Mapping_syntax(this_kind, tok, p, r)
    { }
  };

  /// Template type constructor.
  struct Template_syntax : Mapping_syntax
  {
    static constexpr Kind this_kind = template_kind;

    Template_syntax(Token tok, Syntax* p, Syntax* r)
      : Mapping_syntax(this_kind, tok, p, r)
    { }
  };

  /// Unary postfix operators.
  struct Postfix_syntax : Unary_syntax
  {
    static constexpr Kind this_kind = postfix_kind;

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
    static constexpr Kind this_kind = call_kind;

    Call_syntax(Syntax* s0, Syntax* s1)
      : Application_syntax(this_kind, s0, s1)
    { }
  };

  /// Represents indexing into a table.
  struct Index_syntax : Application_syntax
  {
    static constexpr Kind this_kind = index_kind;

    Index_syntax(Syntax* s0, Syntax* s1)
      : Application_syntax(this_kind, s0, s1)
    { }
  };

  /// Infix binary operators.
  struct Infix_syntax : Binary_syntax
  {
    static constexpr Kind this_kind = infix_kind;

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

  /// A declaration.
  struct Declaration_syntax : Ternary_syntax
  {
    static constexpr Kind this_kind = declaration_kind;

    Declaration_syntax(Token tok, Syntax* d, Syntax* t, Syntax* i)
      : Ternary_syntax(this_kind, d, t, i), m_tok(tok)
    { }

    /// Returns the token introducing the declaration.
    Token introducer() const
    {
      return m_tok;
    }

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
    
    /// Returns the initializer.
    Syntax* initializer() const
    {
      return operand(2);
    }

    Token m_tok;
  };

  /// The top-level container of terms.
  struct File_syntax : Unary_syntax
  {
    static constexpr Kind this_kind = file_kind;

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
    R visit_syntax(Ptr<Syntax> s, Parms... parms)
    {
      return R();
    }

    // Visit syntax of the particular kind. This includes visitors for
    // non-leaf bases (e.g., vector, constructor, and application).

    R visit_unary(Ptr<Unary_syntax> s, Parms... parms)
    {
      return derived()->visit_syntax(s, parms...);
    }

    R visit_binary(Ptr<Binary_syntax> s, Parms... parms)
    {
      return derived()->visit_syntax(s, parms...);
    }

    R visit_ternary(Ptr<Ternary_syntax> s, Parms... parms)
    {
      return derived()->visit_syntax(s, parms...);
    }

    R visit_multiary(Ptr<Multiary_syntax> s, Parms... parms)
    {
      return derived()->visit_syntax(s, parms...);
    }

    R visit_atom(Ptr<Atom_syntax> s, Parms... parms)
    {
      return derived()->visit_syntax(s, parms...);
    }

    R visit_literal(Ptr<Literal_syntax> s, Parms... parms)
    {
      return derived()->visit_atom(s, parms...);
    }

    R visit_identifier(Ptr<Identifier_syntax> s, Parms... parms)
    {
      return derived()->visit_atom(s, parms...);
    }

    R visit_list(Ptr<List_syntax> s, Parms... parms)
    {
      return derived()->visit_multiary(s, parms...);
    }

    R visit_sequence(Ptr<Sequence_syntax> s, Parms... parms)
    {
      return derived()->visit_multiary(s, parms...);
    }

    R visit_enclosure(Ptr<Enclosure_syntax> s, Parms... parms)
    {
      return derived()->visit_unary(s, parms...);
    }

    R visit_prefix(Ptr<Prefix_syntax> s, Parms... parms)
    {
      return derived()->visit_unary(s, parms...);
    }

    R visit_constructor(Ptr<Constructor_syntax> s, Parms... parms)
    {
      return derived()->visit_binary(s, parms...);
    }

    R visit_array(Ptr<Array_syntax> s, Parms... parms)
    {
      return derived()->visit_constructor(s, parms...);
    }

    R visit_function(Ptr<Function_syntax> s, Parms... parms)
    {
      return derived()->visit_constructor(s, parms...);
    }

    R visit_template(Ptr<Template_syntax> s, Parms... parms)
    {
      return derived()->visit_constructor(s, parms...);
    }

    R visit_postfix(Ptr<Postfix_syntax> s, Parms... parms)
    {
      return derived()->visit_unary(s, parms...);
    }

    R visit_application(Ptr<Application_syntax> s, Parms... parms)
    {
      return derived()->visit_binary(s, parms...);
    }

    R visit_call(Ptr<Call_syntax> s, Parms... parms)
    {
      return derived()->visit_application(s, parms...);
    }

    R visit_index(Ptr<Index_syntax> s, Parms... parms)
    {
      return derived()->visit_application(s, parms...);
    }

    R visit_infix(Ptr<Infix_syntax> s, Parms... parms)
    {
      return derived()->visit_binary(s, parms...);
    }

    R visit_declaration(Ptr<Declaration_syntax> s, Parms... parms)
    {
      return derived()->visit_ternary(s, parms...);
    }

    R visit_file(Ptr<File_syntax> s, Parms... parms)
    {
      return derived()->visit_unary(s, parms...);
    }

    // Dispatch to one of the functions above.
    R visit(Ptr<Syntax> s, Parms... parms)
    {
      assert(s);
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
