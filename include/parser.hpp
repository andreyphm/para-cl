#pragma once

#include <memory>
#include <utility>
#include <string>

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"

namespace ParaCL {

class Parser {
public:
    explicit Parser(std::string text) : lexer_(std::move(text)), current_token_(lexer_.next_token()) {}
    std::unique_ptr<Program> parse_program();

private:
    Lexer lexer_;
    Token current_token_;

    void advance() { current_token_ = lexer_.next_token(); }

    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_primary();
    std::unique_ptr<Expression> parse_number();
    std::unique_ptr<Expression> parse_identifier();
    std::unique_ptr<Expression> parse_input();

    std::string report_position() const;
};

} // namespace ParaCL
