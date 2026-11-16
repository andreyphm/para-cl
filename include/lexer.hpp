#pragma once

#include <cctype>
#include <cstddef>
#include <utility>
#include <stdexcept>
#include <string>

#include "token.hpp"

namespace ParaCL {

class Lexer {
public:
    explicit Lexer(std::string text) : text_(std::move(text)) {}

    Token next_token() {
        while (!at_end() && std::isspace(current_symbol())) {
            advance();
        }

        if (at_end()) {
            return {TokenKind::EndOfFile, "", position_};
        }

        if (std::isdigit(current_symbol())) {
            const std::size_t start_index = index_;
            const SourcePosition start_position = position_;
            advance();

            while (!at_end() && std::isdigit(current_symbol())) {
                advance();
            }

            return {TokenKind::Number, text_.substr(start_index, index_ - start_index), start_position};
        }

        throw std::runtime_error("Token recognition is not implemented");
    }

private:
    std::string text_;
    std::size_t index_ = 0;
    SourcePosition position_{1, 1};

    void advance() {
        if (current_symbol() == '\n') {
            position_.column = 1;
            ++position_.line;
        } else {
            ++position_.column;
        }
        ++index_;
    }

    bool at_end() const {
        return index_ == text_.size();
    }

    unsigned char current_symbol() const {
        return text_[index_];
    }
};

} // namespace ParaCL
