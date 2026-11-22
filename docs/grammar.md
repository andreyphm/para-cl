# ParaCL grammar

`Program` is the start rule. Names prefixed with `@` are token kinds. The
[token spellings](../include/token_spellings.hpp) give the source text for
keywords, operators, and delimiters. The lexer supplies `@EndOfFile` at the end of the source.

In the rules below, a comma means sequence, `|` separates alternatives,
`{ ... }` means zero or more repetitions, and `[ ... ]` marks an optional part.

```text
Program = { Statement }, @EndOfFile

Statement = Block
          | IfStatement
          | WhileStatement
          | OutputStatement
          | ExpressionStatement


Block = @BlockBegin, { Statement }, @BlockEnd

IfStatement = @If, @GroupBegin, Expression, @GroupEnd, Statement

WhileStatement = @While, @GroupBegin, Expression, @GroupEnd, Statement

OutputStatement = @Output, Expression, @StatementEnd

ExpressionStatement = Expression, @StatementEnd


Expression = Assignment

Assignment = ( @Identifier, @Assign, Assignment ) | LogicalOr

LogicalOr = LogicalAnd, { @LogicalOr, LogicalAnd }

LogicalAnd = Equality, { @LogicalAnd, Equality }

Equality = Relation, { ( @Equal | @NotEqual ), Relation }

Relation = AddSub, { ( @Less | @LessEqual | @Greater | @GreaterEqual ), AddSub }

AddSub = MulDiv, { ( @Plus | @Minus ), MulDiv }

MulDiv = Unary, { ( @Mul | @Div ), Unary }

Unary = [ @Plus | @Minus ], Primary

Primary = @Number
        | @Identifier
        | @Input
        | ( @GroupBegin, Expression, @GroupEnd )
```
