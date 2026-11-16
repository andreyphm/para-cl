#include "lexer.hpp"
#include "token_spellings.hpp"

#include <cctype>
#include <format>
#include <stdexcept>

namespace ParaCL {
namespace {

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

    return read_operator();
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
    for (const Detail::TokenSpelling spelling : Detail::keyword_spellings) {
        if (name == spelling.text) {
            return {spelling.kind, name, start_position};
        }
    }
    return {TokenKind::Identifier, name, start_position};
}

Token Lexer::read_operator() {
    const SourcePosition start_position = position_;
    const Detail::TokenSpelling* best_match = nullptr;

    for (const Detail::TokenSpelling& spelling : Detail::operator_spellings) {
        if ((!best_match || spelling.text.size() > best_match->text.size()) && text_.compare(index_, spelling.text.size(), spelling.text) == 0) {
            best_match = &spelling;
        }
    }

    if (!best_match) {
        throw std::runtime_error(std::format("Unknown symbol at line {}, column {}", start_position.line, start_position.column));
    }

    for (std::size_t i = 0; i < best_match->text.size(); ++i) {
        advance();
    }

    return {best_match->kind, std::string(best_match->text), start_position};
}

} // namespace ParaCL
