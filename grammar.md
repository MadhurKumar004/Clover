```ebng
<!--
    This file contains the grammar support by this compiler.
    I tried writing the grammar as to be unambiguious, but if you spot any ambiguity please file a issue of the github or even better-fix it.

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
    char ret break continue
    if else elif do loop
    true false as safe extern
    struct

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
    == != < > <= >=
    && || ! 
    & | ^ ~ << >>
    += -= *= /= %= &= |= ^= <<= >>=
    -> . , ; : ::
    * &

<Program> ::= (<function> | <structDecl> | <constDecl> | <externDecl>)* EOF

<function> ::= <type> <identifier> '(' <paramList>? ')' <block>

<structDecl> ::= 'struct' <identifier> '{' <fieldList> '}'

<fieldList> ::= (<identifier> ':' <type> ';')*

<paramList> ::= <param> (',' <param>)*

<param> ::= <identifier> ':' <type>

<externDecl> ::= 'extern' <type> <identifier> '(' <paramList>? ')' ';'

<constDecl> ::= 'const' <identifier> : <type> '=' <expr> ';'

<block> ::= '{' <statement>* '}'

<statement> ::= <exprStmt>
            | <ifStmt>
            | <doStmt>
            | <loopStmt>
            | <returnStmt>
            | <declStmt>
            | <breakStmt>
            | <continueStmt>
            | <unsafeStmt>

<doStmt> ::= 'do' <expr> <block>

<loopStmt> ::= 'loop' <block>

<breakStmt> ::= 'break' ';'

<continueStmt> ::= 'continue' ';'

<unsafeStmt> ::= 'unsafe' <block>

<ifStmt> ::= 'if' <expr> <block> ('elif' <expr> <block>)* ( 'else' <block> )?

<declStmt> ::= <varDecl>

<varDecl> ::= 'const'? <identifier> ':' <type> ('=' <expr>)? ';'

<exprStmt> ::= <expr> ';'

<returnStmt> ::= 'ret' <expr>? ';'

<expr> ::= <assignment>

<assignment> ::= <postfixExpr> '=' <assignment>
                | <disjunction>

<disjunction> ::= <conjunction> ( '||' <conjunction>)*

<conjunction> ::= <equality> ("&&" <equality>)*

<equality> ::= <comparision> ( '==' | '!=') <comparision>)*

<comparision> ::= <bitwiseOrExpr> ( ('<' | '>' | '<=' | '>=') <bitwiseOrExpr>)*

<bitwiseOrExpr> ::= <bitwiseXorExpr> ('|' <bitwiseXorExpr>)*

<bitwiseXorExpr> ::= <bitwiseAndExpr> ('^' <bitwiseAndExpr>)*

<bitwiseAndExpr> ::= <shiftExpr> ('&' <shiftExpr>)*

<shiftExpr> ::= <additiveExpr> (('<<' | '>>') <additiveExpr>)*

<additiveExpr> ::= <multiplicativeExpr> (('+' | '-') <multiplicativeExpr>)*

<multiplicativeExpr> ::= <castExpr> (('*' | '/' | '%') <castExpr>)*

<castExpr> ::= <prefixExpr> 'as' <type>
            | <prefixExpr>

<prefixExpr> ::= ('!' | '-' | '~' | '*' | '&')* <postfixExpr>

<postfixExpr> ::= <primaryExpr>
                | <postfixExpr> '(' <argList>? ')'
                | <postfixExpr> '[' <expr> ']'
                | <postfixExpr> '.' <identifier>
                | <postfixExpr> '.*' <identifier>
<argList> ::= <expr> (',' ,<expr>)*

<primaryExpr> ::= <identifier>
                | <numberLiteral>
                | <charLiteral>
                | <boolLiteral>
                | <stringLiteral>
                | '(' <expr> ')'

<numberLiteral> ::= <decimalLiteral> <numSuffix>?
                | <hexLiteral> <numSuffix>?
                | <binaryLiteral> <numSuffix>?
                | <octalLiteral> <numSuffix>?

<decimalLiteral> ::= ('0'..'9')+ ('.' ('0'..'9')+)?

<hexLiteral> ::= '0x' ('0'..'9' | 'a'..'f' | 'A'..'F')+

<binaryLiteral> ::= '0b' ('0' | '1')+

<octalLiteral> ::= '0o' ('0'..'7')+

<numSuffix> ::= 'i8' | 'i16' ..

<boolLiteral> ::= 'true' | 'false'

<charLiteral> ::= '\'' <charChar> '\''

<charChar> ::= <escapeSeq> | any char except '\''

<escapeSeq> ::= '\n' | '\t' | '\r' | '\\' | '\'' | '\0'

<stringLiteral> ::= '"' <strChar>* '"'

<strChar> ::= <escapeSeq> | any char except '"'

<type> ::= <builtinType>
        | <userdefType>
        | '*' <type>
        | '*' 'const' <type>

<builtinType> ::=
            i1 i8 i16 i32 i64 i128
            u8 u16 u32 u64 u128
            f16 f32 f64 f128
            char

<identifier> ::= ('a' .. 'z' | 'A' .. 'Z' ) + ('a' .. 'z' | 'A' .. 'Z' | '0' .. '9')*

<numberLiteral> ::= ('0'..'9')+('.' ('0'..'9')+)?
```
