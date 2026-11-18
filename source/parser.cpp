#include "parser.hpp"

#include <charconv>
#include <format>
#include <memory>
#include <stdexcept>
#include <system_error>

namespace ParaCL {

std::unique_ptr<Expression> Parser::parse_primary() {
    switch (current_token_.kind) {
        case TokenKind::Number: return parse_number();
        case TokenKind::Identifier: return parse_identifier();
        case TokenKind::Input: return parse_input();
        default:
            throw std::invalid_argument(std::format("Expected primary expression {}", report_position()));
    }
}

std::unique_ptr<Expression> Parser::parse_number() {
    const std::string& text = current_token_.text;
    const char* const end = text.data() + text.size();
    int value{};
    const auto [ptr, err] = std::from_chars(text.data(), end, value);

    if (err == std::errc::result_out_of_range) {
        throw std::out_of_range(std::format("Number literal is outside the int range {}", report_position()));
    }
    if (err == std::errc::invalid_argument || ptr != end) {
        throw std::invalid_argument(std::format("Invalid number literal {}", report_position()));
    }

    advance();
    return std::make_unique<NumberExpression>(value);
}

std::unique_ptr<Expression> Parser::parse_identifier() {
    std::unique_ptr<Expression> variable_ptr = std::make_unique<VariableExpression>(current_token_.text);
    advance();
    return variable_ptr;
}

std::unique_ptr<Expression> Parser::parse_input() {
    std::unique_ptr<Expression> input_ptr = std::make_unique<InputExpression>();
    advance();
    return input_ptr;
}

std::string Parser::report_position() const {
    return std::format("at line {}, column {}", current_token_.position.line, current_token_.position.column);
}

}
