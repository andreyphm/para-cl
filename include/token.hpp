#pragma once

#include <cstddef>
#include <string>

namespace ParaCL {

enum class TokenKind {
    Number,
    Identifier,

    If,
    While,
    Input,
    Output,

    Plus,
    Minus,
    Mul,
    Div,

    Assign,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    LogicalAnd,
    LogicalOr,

    GroupBegin,
    GroupEnd,
    BlockBegin,
    BlockEnd,
    StatementEnd,

    EndOfFile,
};

struct SourcePosition {
    std::size_t line;
    std::size_t column;
};

struct Token {
    TokenKind kind;
    std::string text;
    SourcePosition position;
};

} // namespace ParaCL
