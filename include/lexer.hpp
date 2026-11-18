#pragma once

#include <cstddef>
#include <optional>
#include <utility>
#include <string>
#include <vector>

#include "token.hpp"

namespace ParaCL {

class Lexer {
public:
    explicit Lexer(std::string text) : text_(std::move(text)) {}

    Token next_token();

private:
    std::string text_;
    std::size_t index_ = 0;
    SourcePosition position_{1, 1};

    void advance();
    bool at_end() const;
    unsigned char current_symbol() const;
    void skip_spaces_and_comments();

    Token read_number();
    Token read_identifier_or_keyword();
    std::optional<Token> try_read_operator();
};

std::vector<Token> tokenize(std::string text);

} // namespace ParaCL
