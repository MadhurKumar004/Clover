<!--
    This file contains the grammar support by this compiler.
    I tried writing the grammar as to be unambiguious, but if you spot any ambiguity please file a issue of the github or even better fix it.

    The grammer represents a low level statically typed langauge. That expect everything from a user and user to know what he is doing. Rather then making
    assumption by the compiler.
-->

Token :
    keyword
    identifier
    contant
    string-literal
    puntuator

keyword :
    i1 i8 i16 i32 i64 i128
    u8 u16 u32 u64 u128
    f16 f32 f64 f128
    char
    fn if else elif do loop

identifier :
    identifier nondigit
    identifier digit

    nondigit :
        - a b c d e f g h
        i j k l m n o p q
        r s t u v w x y z
        (same for capital)

    digit :
        1 2 3 4 5 6 7 8 9 0


constant :
    integer constant
    float constant
    string constant
    enumeration constant

string literal :
    encoding-prefix "s char sequence"

puntuator:
    {} [] ()
    - + / * % =
    ..


<SelectedFile> :: (<function>) EOF

<function> :: <type> <identifier> LPAREN <paramDecl> RPAREN <block>

<blocK> :: '{' <statement> '}'

<statement> :: <expr> ';'
            | <ifStmt>
            | <doStmt>
            | <returnStmt>
            | <assignment>
            | <declStmt>

<doStmt> :: 'do' <expr> <block>

<ifStmt> :: 'if' <expr> <block> ( 'else' <block> )?

<declStmt> :: <varDecl> ';'

<varDecl> :: <identifier> ':' <type> ('=' <expr>)?

<assignment> :: <expr> '=' <expr> ';'

<returnStmt> :: 'ret' <expr>? ';'

<expr> :: <disjunction>

<disjunction> :: <conjunction> ( '||' <conjunction>)?

<conjunction> :: <equality> ("&&" <equality>)?

<equalitly> :: <comparision> ( '==' <comparision>)?

<comparision> :: <additiveExpr> ( ('<' | '>') <addExpr>)?

<additiveExpr> :: <multiplicativeExpr> (('+' | '-') <multiplicativeExpr>)?

<multiplicativeExpr> :: <prefixExpr> (('*' | '/') <prefixExpr)?

<prefixExpr> :: ('!')* <postfixExpr>

<postfixExpr> :: <primaryExpr>

<primaryExpr> :: <type>
                | <numberLiteral>


<type> :: <builtinType>
        | <userdefType>

<builtinType> ::
            i1 i8 i16 i32 i64 i128
            u8 u16 u32 u64 u128
            f16 f32 f64 f128
            char

<identifier> :: ('a' .. 'z' | 'A' .. 'Z' ) + ('a' .. 'z' | 'A' .. 'Z' | '0' .. '9')*

<numberLiteral> :: ('0'..'9')+('.' ('0'..'9')+)?
