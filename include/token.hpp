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

    LeftParen,
    RightParen,
    LeftBrace,
    RightBrace,
    Semicolon,

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

}
