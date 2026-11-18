#include "parser.hpp"

#include <charconv>
#include <format>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace ParaCL {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {
    if (tokens_.empty() || tokens_.back().kind != TokenKind::EndOfFile) {
        throw std::invalid_argument("Token sequence must end with EndOfFile");
    }
    for (std::size_t i = 0; i < tokens_.size() - 1; ++i) {
        if (tokens_[i].kind == TokenKind::EndOfFile) {
            throw std::invalid_argument("EndOfFile must be the last token");
        }
    }
}

std::unique_ptr<Expression> Parser::parse_expression() {
    return parse_assignment();
}

std::unique_ptr<Expression> Parser::parse_assignment() {
    if (current_token().kind == TokenKind::Identifier && next_token_kind() == TokenKind::Assign) {
        Token identifier = consume(TokenKind::Identifier);
        consume(TokenKind::Assign);
        auto expression = parse_assignment();
        return std::make_unique<AssignmentExpression>(std::move(identifier.text), std::move(expression));
    }

    // Will be replaced with parse_logical_or() when this priority level is implemented.
    return parse_primary();
}

std::unique_ptr<Expression> Parser::parse_primary() {
    switch (current_token().kind) {
        case TokenKind::Number: return parse_number();
        case TokenKind::Identifier: return parse_identifier();
        case TokenKind::Input: return parse_input();
        case TokenKind::GroupBegin: return parse_grouped_expression();

        default:
            throw std::invalid_argument(std::format("Expected primary expression {}", report_position()));
    }
}

std::unique_ptr<Expression> Parser::parse_number() {
    const std::string& text = current_token().text;
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
    const Token token = consume(TokenKind::Identifier);
    return std::make_unique<VariableExpression>(token.text);
}

std::unique_ptr<Expression> Parser::parse_input() {
    std::unique_ptr<Expression> input_ptr = std::make_unique<InputExpression>();
    consume(TokenKind::Input);
    return input_ptr;
}

std::unique_ptr<Expression> Parser::parse_grouped_expression() {
    consume(TokenKind::GroupBegin);
    std::unique_ptr<Expression> expression = parse_expression();
    consume(TokenKind::GroupEnd);
    return expression;
}

const Token& Parser::current_token() const {
    return tokens_[index_];
}

void Parser::advance() {
    if (current_token().kind != TokenKind::EndOfFile) {
        ++index_;
    }
}

TokenKind Parser::next_token_kind() const {
    if (index_ < tokens_.size() - 1) {
        return tokens_[index_ + 1].kind;
    }
    return TokenKind::EndOfFile;
}

Token Parser::consume(TokenKind expected) {
    if (expected != current_token().kind) {
        throw std::invalid_argument(std::format("Expected {}, got {} {}",
            token_kind_name(expected), token_kind_name(current_token().kind), report_position()));
    }
    Token token = current_token();
    advance();
    return token;
}

std::string Parser::report_position() const {
    return std::format("at line {}, column {}", current_token().position.line, current_token().position.column);
}

}
