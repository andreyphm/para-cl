#include "token.hpp"

#include <stdexcept>

namespace ParaCL {

std::string_view token_kind_name(TokenKind kind) {
    switch (kind) {
        case TokenKind::Number: return "Number";
        case TokenKind::Identifier: return "Identifier";
        case TokenKind::If: return "If";
        case TokenKind::While: return "While";
        case TokenKind::Input: return "Input";
        case TokenKind::Output: return "Output";
        case TokenKind::Plus: return "Plus";
        case TokenKind::Minus: return "Minus";
        case TokenKind::Mul: return "Mul";
        case TokenKind::Div: return "Div";
        case TokenKind::Assign: return "Assign";
        case TokenKind::Equal: return "Equal";
        case TokenKind::NotEqual: return "NotEqual";
        case TokenKind::Less: return "Less";
        case TokenKind::LessEqual: return "LessEqual";
        case TokenKind::Greater: return "Greater";
        case TokenKind::GreaterEqual: return "GreaterEqual";
        case TokenKind::LogicalAnd: return "LogicalAnd";
        case TokenKind::LogicalOr: return "LogicalOr";
        case TokenKind::GroupBegin: return "GroupBegin";
        case TokenKind::GroupEnd: return "GroupEnd";
        case TokenKind::BlockBegin: return "BlockBegin";
        case TokenKind::BlockEnd: return "BlockEnd";
        case TokenKind::StatementEnd: return "StatementEnd";
        case TokenKind::EndOfFile: return "EndOfFile";
    }

    throw std::logic_error("Unknown token kind");
}

} // namespace ParaCL
