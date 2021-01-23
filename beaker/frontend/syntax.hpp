#ifndef BEAKER_FRONTEND_SYNTAX_HPP
#define BEAKER_FRONTEND_SYNTAX_HPP

#include <beaker/frontend/token.hpp>

#include <span>

namespace beaker::cst
{
  /// The base class of all concrete syntax trees.
  struct Tree
  {

  };

  /// Any tree defined by a single atom.
  struct Atom : Tree
  {
    Token token;
  };

  /// Represents literal values.
  struct Literal : Atom
  {
  };

  struct Identifier : Atom
  {
  };

  /// A sequence of delimited terms.
  ///
  /// TODO: This doesn't store the delimiters. I'm not sure if that's
  /// actually important.
  struct List : Tree
  {
    std::span<Tree*> terms;
  };

  /// A sequence of terms.
  struct Seq : Tree
  {
    std::span<Tree*> terms;
  };

  /// A term enclosed by a pair of tokens.
  struct Enclosure : Tree
  {
    Token open;
    Token close;
    Tree* term;
  };

  /// A unary prefix exprssion.
  struct Prefix : Tree
  {
    Token op;
    Tree* term;
  };

  /// Compound type constructors.
  struct Constructor : Tree
  {
    Token ctor;
    Tree* args;
    Tree* term;
  };

  /// Array type constructor.
  struct Array : Constructor
  {
  };

  /// Function type constructor.
  struct Function : Constructor
  {
  };

  /// Template type constructor.
  struct Template : Constructor
  {
  };

  /// Unary postfix operators.
  struct Postfix : Tree
  {
    Tree* term;
    Token op;
  };

  /// A postfix operator applied to an array, function, or template.
  /// This "eliminates" its compound term, yielding a single value.
  struct Destructor : Tree
  {
    Tree* term;
    Tree* args;
  };

  struct Call : Destructor
  {
  };

  struct Index : Destructor
  {
  };

  struct Infix : Tree
  {
    Token op;
    Tree* lhs;
    Tree* rhs;
  };

  /// A declaration.
  struct Decl : Tree
  {
    Tree* decl;
    Tree* type;
    Tree* init;
  };

  struct File : Tree
  {
  };

} // namespace beaker::cst


#endif
