#pragma once

#include <memory>
#include <cstddef>
#include <string>
#include <vector>

#include "ast.hpp"
#include "token.hpp"

namespace ParaCL {

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parse_program();

private:
    std::vector<Token> tokens_;
    std::size_t index_ = 0;

    const Token& current_token() const;
    void advance();
    TokenKind next_token_kind() const;
    Token consume(TokenKind expected);

    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<Statement> parse_block();
    std::unique_ptr<Statement> parse_if();
    std::unique_ptr<Statement> parse_while();
    std::unique_ptr<Statement> parse_output();
    std::unique_ptr<Statement> parse_expression_statement();

    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_assignment();
    std::unique_ptr<Expression> parse_logical_or();
    std::unique_ptr<Expression> parse_logical_and();
    std::unique_ptr<Expression> parse_equality();
    std::unique_ptr<Expression> parse_relation();
    std::unique_ptr<Expression> parse_addsub();
    std::unique_ptr<Expression> parse_muldiv();
    std::unique_ptr<Expression> parse_unary();
    std::unique_ptr<Expression> parse_primary();
    std::unique_ptr<Expression> parse_number();
    std::unique_ptr<Expression> parse_identifier();
    std::unique_ptr<Expression> parse_input();
    std::unique_ptr<Expression> parse_grouped_expression();

    std::string report_position() const;
};

} // namespace ParaCL
