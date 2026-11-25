#include "parser.hpp"

#include <charconv>
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace ParaCL {
namespace {

std::optional<BinaryOperator> relation_operator_for(TokenKind kind) {
    switch (kind) {
        case TokenKind::Less: return BinaryOperator::Less;
        case TokenKind::LessEqual: return BinaryOperator::LessEqual;
        case TokenKind::Greater: return BinaryOperator::Greater;
        case TokenKind::GreaterEqual: return BinaryOperator::GreaterEqual;
        default: return std::nullopt;
    }
}

std::optional<BinaryOperator> equality_operator_for(TokenKind kind) {
    switch (kind) {
        case TokenKind::Equal: return BinaryOperator::Equal;
        case TokenKind::NotEqual: return BinaryOperator::NotEqual;
        default: return std::nullopt;
    }
}

std::optional<BinaryOperator> addsub_operator_for(TokenKind kind) {
    switch (kind) {
        case TokenKind::Plus: return BinaryOperator::Add;
        case TokenKind::Minus: return BinaryOperator::Sub;
        default: return std::nullopt;
    }
}

std::optional<BinaryOperator> muldiv_operator_for(TokenKind kind) {
    switch (kind) {
        case TokenKind::Mul: return BinaryOperator::Mul;
        case TokenKind::Div: return BinaryOperator::Div;
        default: return std::nullopt;
    }
}

std::optional<UnaryOperator> unary_operator_for(TokenKind kind) {
    switch (kind) {
        case TokenKind::Plus: return UnaryOperator::Plus;
        case TokenKind::Minus: return UnaryOperator::Minus;
        case TokenKind::LogicalNot: return UnaryOperator::LogicalNot;
        default: return std::nullopt;
    }
}

} // namespace

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

std::unique_ptr<Program> Parser::parse_program() {
    std::vector<std::unique_ptr<Statement>> statements;
    while (current_token().kind != TokenKind::EndOfFile) {
        statements.push_back(parse_statement());
    }
    return std::make_unique<Program>(std::move(statements));
}

std::unique_ptr<Statement> Parser::parse_statement() {
    switch (current_token().kind) {
        case TokenKind::BlockBegin: return parse_block();
        case TokenKind::If: return parse_if();
        case TokenKind::While: return parse_while();
        case TokenKind::Output: return parse_output();
        case TokenKind::StatementEnd: return parse_empty_statement();
        default: return parse_expression_statement();
    }
}

std::unique_ptr<Statement> Parser::parse_block() {
    consume(TokenKind::BlockBegin);
    std::vector<std::unique_ptr<Statement>> statements;
    while (current_token().kind != TokenKind::BlockEnd && current_token().kind != TokenKind::EndOfFile) {
        statements.push_back(parse_statement());
    }
    consume(TokenKind::BlockEnd);
    return std::make_unique<BlockStatement>(std::move(statements));
}

std::unique_ptr<Statement> Parser::parse_if() {
    consume(TokenKind::If);
    consume(TokenKind::GroupBegin);
    auto condition = parse_expression();
    consume(TokenKind::GroupEnd);
    auto body = parse_statement();
    return std::make_unique<IfStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::parse_while() {
    consume(TokenKind::While);
    consume(TokenKind::GroupBegin);
    auto condition = parse_expression();
    consume(TokenKind::GroupEnd);
    auto body = parse_statement();
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

std::unique_ptr<Statement> Parser::parse_output() {
    consume(TokenKind::Output);
    auto expression = parse_expression();
    consume(TokenKind::StatementEnd);
    return std::make_unique<OutputStatement>(std::move(expression));
}

std::unique_ptr<Statement> Parser::parse_expression_statement() {
    auto expression = parse_expression();
    consume(TokenKind::StatementEnd);
    return std::make_unique<ExpressionStatement>(std::move(expression));
}

std::unique_ptr<Expression> Parser::parse_expression() {
    return parse_assignment();
}

std::unique_ptr<Statement> Parser::parse_empty_statement() {
    consume(TokenKind::StatementEnd);
    return std::make_unique<EmptyStatement>();
}

std::unique_ptr<Expression> Parser::parse_assignment() {
    if (current_token().kind == TokenKind::Identifier && next_token_kind() == TokenKind::Assign) {
        Token identifier = consume(TokenKind::Identifier);
        consume(TokenKind::Assign);
        auto expression = parse_assignment();
        return std::make_unique<AssignmentExpression>(std::move(identifier.text), std::move(expression));
    }
    return parse_logical_or();
}

std::unique_ptr<Expression> Parser::parse_logical_or() {
    auto first_expression = parse_logical_and();
    while (current_token().kind == TokenKind::LogicalOr) {
        advance();
        auto next_expression = parse_logical_and();
        first_expression = std::make_unique<BinaryExpression>(BinaryOperator::LogicalOr, std::move(first_expression), std::move(next_expression));
    }
    return first_expression;
}

std::unique_ptr<Expression> Parser::parse_logical_and() {
    auto first_expression = parse_equality();
    while (current_token().kind == TokenKind::LogicalAnd) {
        advance();
        auto next_expression = parse_equality();
        first_expression = std::make_unique<BinaryExpression>(BinaryOperator::LogicalAnd, std::move(first_expression), std::move(next_expression));
    }
    return first_expression;
}

std::unique_ptr<Expression> Parser::parse_equality() {
    auto first_expression = parse_relation();
    while (const auto operation = equality_operator_for(current_token().kind)) {
        advance();
        auto next_expression = parse_relation();
        first_expression = std::make_unique<BinaryExpression>(*operation, std::move(first_expression), std::move(next_expression));
    }
    return first_expression;
}

std::unique_ptr<Expression> Parser::parse_relation() {
    auto first_expression = parse_addsub();
    while (const auto operation = relation_operator_for(current_token().kind)) {
        advance();
        auto next_expression = parse_addsub();
        first_expression = std::make_unique<BinaryExpression>(*operation, std::move(first_expression), std::move(next_expression));
    }
    return first_expression;
}

std::unique_ptr<Expression> Parser::parse_addsub() {
    auto first_expression = parse_muldiv();
    while (const auto operation = addsub_operator_for(current_token().kind)) {
        advance();
        auto next_expression = parse_muldiv();
        first_expression = std::make_unique<BinaryExpression>(*operation, std::move(first_expression), std::move(next_expression));
    }
    return first_expression;
}

std::unique_ptr<Expression> Parser::parse_muldiv() {
    auto first_expression = parse_unary();
    while (const auto operation = muldiv_operator_for(current_token().kind)) {
        advance();
        auto next_expression = parse_unary();
        first_expression = std::make_unique<BinaryExpression>(*operation, std::move(first_expression), std::move(next_expression));
    }
    return first_expression;
}

std::unique_ptr<Expression> Parser::parse_unary() {
    if (const auto operation = unary_operator_for(current_token().kind)) {
        advance();
        return std::make_unique<UnaryExpression>(*operation, parse_primary());
    }
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
