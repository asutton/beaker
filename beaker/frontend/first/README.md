
The file extension for this language is `.bkr` or `.bkr`. The compiler can
also be run with `-language first`.

Note that array syntax is kind overly verbose in this model. We don't get
the nice bracketing that we'd have in C/C++.

```
def a : array[3, 4] int; # Ok
def a : array[3] array[4] int; # Valid, but weird.
```

The full grammar is.

```
file:
    declaration-seq?

declaration-seq:
    declaration
    declaration-seq declaration

declaration:
    definition-declaration

definition-declaration:
    def declarator-list type-clause ;
    def declarator-list : initializer-clause
    def declarator-list type-clause initializer-clause

type-clause;
    : type

initializer-clause:
    = expression ;
    { statement-seq }

parameter:
    identifier : type
    identifier : type = expression
    identifeir : = expression
    : type
    : type = expression

declarator-list:
    declarator
    declarator-list , declarator

declarator:
    postfix-expression

type-expression:
    prefix-expression

expression:
    infix-expression

infix-expression:
    assignment-expression:

assignment-expression:
    implication-expression
    implication-expression = assignment-expression

implication-expression:
    logical-or-expression
    logical-or-expression -> implication-expression

logical-or-expression:
    logical-and-expression
    logical-or-expression or logical-and-expression

logical-and-expression:
    equality-expression
    logical-and-expression and equality-expression

equality-expression:
    relational-expression
    equality-expression == relational-expression
    equality-expression != relational-expression

relational-expression:
    additive-expression
    relational-expression < additive-expression
    relational-expression > additive-expression
    relational-expression <= additive-expression
    relational-expression >= additive-expression

additive-expression:
    multiplicative-expression
    additive-expression < multiplicative-expression
    additive-expression > multiplicative-expression

multiplicative-expression:
    prefix-exprssion
    multiplicative-expression < prefix-exprssion
    multiplicative-expression > prefix-exprssion

prefix-expression:
    postfix-expression
    array [ expression-list? ] prefix-expression
    templ [ expression-group? ] prefix-expression
    func ( expression-group? ) prefix-expression
    const prefix-exprssion
    ^ prefix-expression
    - prefix-expression
    + prefix-expression
    not prefix-expression

postfix-expression:
    primary-expression
    postfix-expression ( expression-list )
    postfix-expression [ expression-list ]
    postfix-expression .
    postfix-expression ^

primary-expression:
    literal
    ( expression-list )
    id-expression

id-expression:
    identifier

expression-group:
    expression-list
    expression-group ; expression-list

expression-list:
    parameter-expression
    expression-list , parameter-or-expression

parameter-or-expression:
    parameter
    expression

statement-seq
    statement
    statement-seq statement

statement:
    declaration-statement
    expression-statement
``` 
