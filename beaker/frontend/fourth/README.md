
The file extension for this language is `.bkr4`. The compiler can
also be run with `-language fourth`. The grammar is:

The biggest difference in the language is the treatment of pointer and array
types as template-like type constructors. Pointers and arrays are spelled
as:

```
ptr[t]
array[t][ext0][ext1]...
```

```
file:
    declaration-seq?

declaration-seq:
    declaration
    declaration-seq declaration

declaration:
    definition-declaration

definition-declaration:
    def declarator : type ;
    def declarator : = expression ;
    def declarator : type = expression ;

parameter:
    identifier : type
    identifier : type = expression
    identifeir : = expression
    : type
    : type = expression

declarator:
    postfix-expression

type-expression:
    implication-expression

expression:
    infix-expression

infix-expression:
    implication-expression

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
    const prefix-exprssion
    * prefix-expression
    & prefix-expression
    - prefix-expression
    + prefix-expression
    not prefix-expression

postfix-expression:
    primary-expression
    postfix-expression ( expression-list )
    postfix-expression [ expression-list ]
    postfix-expression .

primary-expression:
    literal
    identifier
    ( expression-list? )
    [ expression-list? ]
    id-expression

literal:
    integer
    int
    bool
    type
    ptr
    array

tuple-expression:
    paren-list

list-expression:
    paren-list

id-expression:
    identifier

paren-list:
    ( expression-list? )

bracket-list:
    [ expression-list? ]

expression-group:
    expression-list
    expression-group ; expression-list

expression-list:
    parameter-expression
    expression-list , parameter-expression

parameter-or-expression:
    parameter
    expression
``` 
