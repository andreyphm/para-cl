#include "lexer.hpp"

#include <cctype>
#include <stdexcept>
#include <string_view>

namespace ParaCL {
namespace {

struct TokenSpelling {
    std::string_view text;
    TokenKind kind;
};

constexpr TokenSpelling keyword_spellings[] = {
    {"if",    TokenKind::If},
    {"while", TokenKind::While},
    {"print", TokenKind::Output},
};

constexpr TokenSpelling operator_spellings[] = {
    {"==", TokenKind::Equal},
    {"!=", TokenKind::NotEqual},
    {"<=", TokenKind::LessEqual},
    {">=", TokenKind::GreaterEqual},
    {"&&", TokenKind::LogicalAnd},
    {"||", TokenKind::LogicalOr},

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
};

bool is_identifier_start(unsigned char symbol) {
    return std::isalpha(symbol) || symbol == '_';
}

bool is_identifier_symbol(unsigned char symbol) {
    return std::isalpha(symbol) || symbol == '_' || std::isdigit(symbol);
}

} // namespace

Token Lexer::next_token() {
    skip_spaces();
    if (at_end()) {
        return {TokenKind::EndOfFile, "", position_};
    }

    if (std::isdigit(current_symbol())) {
        return read_number();
    }

    if (is_identifier_start(current_symbol())) {
        return read_identifier_or_keyword();
    }

    throw std::runtime_error("Token recognition is not implemented");
}

void Lexer::advance() {
    if (current_symbol() == '\n') {
        position_.column = 1;
        ++position_.line;
    } else {
        ++position_.column;
    }
    ++index_;
}

bool Lexer::at_end() const {
    return index_ == text_.size();
}

unsigned char Lexer::current_symbol() const {
    return text_[index_];
}

void Lexer::skip_spaces() {
    while (!at_end() && std::isspace(current_symbol())) {
        advance();
    }
}

Token Lexer::read_number() {
    const std::size_t start_index = index_;
    const SourcePosition start_position = position_;
    advance();

    while (!at_end() && std::isdigit(current_symbol())) {
        advance();
    }

    return {TokenKind::Number, text_.substr(start_index, index_ - start_index), start_position};
}

Token Lexer::read_identifier_or_keyword() {
    const std::size_t start_index = index_;
    const SourcePosition start_position = position_;
    advance();

    while (!at_end() && is_identifier_symbol(current_symbol())) {
        advance();
    }

    const std::string name = text_.substr(start_index, index_ - start_index);
    for (const TokenSpelling spelling : keyword_spellings) {
        if (name == spelling.text) {
            return {spelling.kind, name, start_position};
        }
    }
    return {TokenKind::Identifier, name, start_position};
}

} // namespace ParaCL
