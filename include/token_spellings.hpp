#pragma once

#include "token.hpp"

#include <string_view>

namespace ParaCL::Detail {

struct TokenSpelling {
    std::string_view text;
    TokenKind kind;
};

inline constexpr TokenSpelling keyword_spellings[] = {
    {"if", TokenKind::If},
    {"while", TokenKind::While},
    {"print", TokenKind::Output},
};

inline constexpr TokenSpelling operator_spellings[] = {
    {"+", TokenKind::Plus},
    {"-", TokenKind::Minus},
    {"*", TokenKind::Mul},
    {"/", TokenKind::Div},
    {"=", TokenKind::Assign},
    {"<", TokenKind::Less},
    {">", TokenKind::Greater},
    {"?", TokenKind::Input},
    {"(", TokenKind::LeftParen},
    {")", TokenKind::RightParen},
    {"{", TokenKind::LeftBrace},
    {"}", TokenKind::RightBrace},
    {";", TokenKind::Semicolon},

    {"==", TokenKind::Equal},
    {"!=", TokenKind::NotEqual},
    {"<=", TokenKind::LessEqual},
    {">=", TokenKind::GreaterEqual},
    {"&&", TokenKind::LogicalAnd},
    {"||", TokenKind::LogicalOr},
};

} // namespace ParaCL::Detail
