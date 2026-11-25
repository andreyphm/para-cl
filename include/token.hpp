#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace ParaCL {

enum class TokenKind {
    Number,
    Identifier,

    If,
    Else,
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
    LogicalNot,

    GroupBegin,
    GroupEnd,
    BlockBegin,
    BlockEnd,
    StatementEnd,

    EndOfFile,
};

std::string_view token_kind_name(TokenKind kind);

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
