#include "parser.hpp"
#include "token_spellings.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <format>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ParaCL {
namespace {

using K = TokenKind;
using TokenPart = std::variant<TokenKind, std::string_view>;
using TokenParts = std::vector<TokenPart>;

std::string_view spelling_for(TokenKind kind) {
    for (const auto& spelling : Detail::keyword_spellings) {
        if (spelling.kind == kind) {
            return spelling.text;
        }
    }
    for (const auto& spelling : Detail::operator_spellings) {
        if (spelling.kind == kind) {
            return spelling.text;
        }
    }
    throw std::logic_error("Missing fixed token spelling in parser test");
}

std::vector<Token> make_tokens(const TokenParts& parts) {
    std::vector<Token> tokens;
    std::size_t column = 1;
    for (const auto& part : parts) {
        TokenKind kind;
        std::string text;
        if (const auto* fixed_kind = std::get_if<TokenKind>(&part)) {
            kind = *fixed_kind;
            text = spelling_for(kind);
        } else {
            const auto word = std::get<std::string_view>(part);
            if (word.empty()) {
                throw std::logic_error("Empty word in parser test token sequence");
            }
            kind = word.front() >= '0' && word.front() <= '9' ? K::Number : K::Identifier;
            text = word;
        }
        tokens.push_back({kind, std::move(text), {1, column}});
        column += tokens.back().text.size() + 1;
    }
    tokens.push_back({K::EndOfFile, "", {1, column}});
    return tokens;
}

std::unique_ptr<Program> parse(const TokenParts& parts) {
    Parser parser{make_tokens(parts)};
    return parser.parse_program();
}

struct SyntaxCase {
    const char* name;
    TokenParts parts;
    std::size_t error_index;
    std::optional<TokenKind> expected = std::nullopt;
};

class ParserSyntaxTest : public testing::TestWithParam<SyntaxCase> {};

TEST_P(ParserSyntaxTest, RejectsInvalidProgramAtExpectedPosition) {
    const auto& test = GetParam();
    auto tokens = make_tokens(test.parts);
    ASSERT_LT(test.error_index, tokens.size());
    const Token error_token = tokens[test.error_index];
    Parser parser{std::move(tokens)};
    try {
        parser.parse_program();
        FAIL() << "Expected syntax error";
    } catch (const std::invalid_argument& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find(std::format("at line {}, column {}",
            error_token.position.line, error_token.position.column)), std::string::npos);
        if (test.expected) {
            EXPECT_NE(message.find(std::format("Expected {}, got {}",
                token_kind_name(*test.expected), token_kind_name(error_token.kind))), std::string::npos);
        } else {
            EXPECT_NE(message.find("Expected primary expression"), std::string::npos);
        }
    }
}

const SyntaxCase syntax_cases[] = {
    {"EmptyOutput", {K::Output, K::StatementEnd}, 1},
    {"MissingTerminator", {"1"}, 1, K::StatementEnd},
    {"MissingOutputTerminator", {K::Output, "1"}, 2, K::StatementEnd},
    {"MissingRightOperand", {"1", K::Plus, K::StatementEnd}, 2},
    {"MissingLeftOperand", {K::Mul, "1", K::StatementEnd}, 0},
    {"MissingOrOperand", {"1", K::LogicalOr}, 2},
    {"MissingAndOperand", {"0", K::LogicalAnd, K::StatementEnd}, 2},
    {"MissingEqualityOperand", {"1", K::Equal, K::StatementEnd}, 2},
    {"MissingRelationOperand", {"1", K::GreaterEqual, K::StatementEnd}, 2},
    {"EmptyGroup", {K::GroupBegin, K::GroupEnd, K::StatementEnd}, 1},
    {"UnclosedGroupAtTerminator", {K::GroupBegin, "1", K::StatementEnd}, 2, K::GroupEnd},
    {"UnclosedGroupAtEndOfFile", {K::GroupBegin, "1"}, 2, K::GroupEnd},
    {"ExtraGroupEnd", {"1", K::GroupEnd, K::StatementEnd}, 1, K::StatementEnd},
    {"UnclosedBlock", {K::BlockBegin}, 1, K::BlockEnd},
    {"ExtraBlockEnd", {K::BlockEnd}, 0},
    {"MissingTerminatorInsideBlock", {K::BlockBegin, "1", K::BlockEnd}, 2, K::StatementEnd},
    {"EmptyIfCondition", {K::If, K::GroupBegin, K::GroupEnd, K::BlockBegin, K::BlockEnd}, 2},
    {"MissingIfGroupBegin", {K::If, "1", K::GroupEnd, K::BlockBegin, K::BlockEnd}, 1, K::GroupBegin},
    {"MissingIfGroupEnd", {K::If, K::GroupBegin, "1", K::BlockBegin, K::BlockEnd}, 3, K::GroupEnd},
    {"MissingIfBody", {K::If, K::GroupBegin, "1", K::GroupEnd}, 4},
    {"UnmatchedElse", {K::Else, K::StatementEnd}, 0},
    {"MissingElseBody", {K::If, K::GroupBegin, "1", K::GroupEnd, K::StatementEnd, K::Else}, 6},
    {"EmptyWhileCondition", {K::While, K::GroupBegin, K::GroupEnd, K::BlockBegin, K::BlockEnd}, 2},
    {"MissingWhileGroupBegin", {K::While, "0", K::GroupEnd, K::BlockBegin, K::BlockEnd}, 1, K::GroupBegin},
    {"MissingWhileGroupEnd", {K::While, K::GroupBegin, "0", K::BlockBegin, K::BlockEnd}, 3, K::GroupEnd},
    {"MissingWhileBody", {K::While, K::GroupBegin, "0", K::GroupEnd}, 4},
    {"NumberAssignmentTarget", {"1", K::Assign, "2", K::StatementEnd}, 1, K::StatementEnd},
    {"GroupedAssignmentTarget", {K::GroupBegin, "x", K::GroupEnd, K::Assign, "1", K::StatementEnd}, 3, K::StatementEnd},
    {"UnaryAssignmentTarget", {K::Minus, "x", K::Assign, "1", K::StatementEnd}, 2, K::StatementEnd},
    {"MissingAssignmentValue", {"x", K::Assign, K::StatementEnd}, 2},
    {"RepeatedAssignmentToken", {"x", K::Assign, K::Assign, "1", K::StatementEnd}, 2},
    {"DoubleUnaryPlus", {K::Plus, K::Plus, "1", K::StatementEnd}, 1},
    {"DoubleUnaryMinus", {K::Minus, K::Minus, "1", K::StatementEnd}, 1},
    {"DoubleLogicalNot", {K::LogicalNot, K::LogicalNot, "1", K::StatementEnd}, 1},
    {"MissingLogicalNotOperand", {K::LogicalNot, K::StatementEnd}, 1},
    {"InfixLogicalNot", {K::Output, "1", K::LogicalNot, "2", K::StatementEnd}, 2, K::StatementEnd},
    {"PlusMinusUnary", {K::Plus, K::Minus, "1", K::StatementEnd}, 1},
    {"MinusPlusUnary", {K::Minus, K::Plus, "1", K::StatementEnd}, 1},
    {"AdjacentNumbers", {"1", "2", K::StatementEnd}, 1, K::StatementEnd},
    {"AdjacentNames", {"x", "y", K::StatementEnd}, 1, K::StatementEnd},
    {"OutputExtraExpression", {K::Output, "1", "2", K::StatementEnd}, 2, K::StatementEnd},
    {"KeywordInsideExpression", {"x", K::Assign, K::While, K::StatementEnd}, 2},
    {"MissingTerminatorBeforeOutput", {"x", K::Assign, "1", K::Output, "2", K::StatementEnd}, 3, K::StatementEnd},
    {"InvalidSkippedBranch", {K::If, K::GroupBegin, "0", K::GroupEnd, K::Output, K::StatementEnd}, 5},
    {"InvalidShortCircuitOperand", {"1", K::LogicalOr, K::GroupBegin, K::GroupEnd, K::StatementEnd}, 3},
};

INSTANTIATE_TEST_SUITE_P(Syntax, ParserSyntaxTest, testing::ValuesIn(syntax_cases),
    [](const testing::TestParamInfo<SyntaxCase>& info) { return info.param.name; });

TEST(ParserNumberTest, RejectsOutOfRangeLiteralWithItsPosition) {
    const std::string too_large = std::format("{}0", std::numeric_limits<int>::max());
    const TokenParts parts{K::Output, std::string_view{too_large}, K::StatementEnd};
    const auto tokens = make_tokens(parts);
    try {
        Parser{tokens}.parse_program();
        FAIL() << "Expected out-of-range literal";
    } catch (const std::out_of_range& error) {
        EXPECT_NE(std::string(error.what()).find(std::format("at line {}, column {}",
            tokens[1].position.line, tokens[1].position.column)), std::string::npos);
    }
}

TEST(ParserAcceptanceTest, ParsesStatementAlternativesWithoutExecution) {
    struct StatementCase {
        const char* name;
        TokenParts parts;
    };
    const StatementCase cases[] = {
        {"EmptyProgram", {}},
        {"EmptyStatement", {K::StatementEnd}},
        {"EmptyBlock", {K::BlockBegin, K::BlockEnd}},
        {"EmptyStatementInsideBlock", {K::BlockBegin, K::StatementEnd, K::BlockEnd}},
        {"EmptyStatementAfterBlock", {K::BlockBegin, K::BlockEnd, K::StatementEnd}},
        {"NestedBlocks", {K::BlockBegin, K::BlockBegin, K::BlockEnd, K::BlockEnd}},
        {"Output", {K::Output, K::Input, K::StatementEnd}},
        {"IfWithExpressionBody", {K::If, K::GroupBegin, "1", K::GroupEnd,
            "x", K::Assign, "2", K::StatementEnd}},
        {"IfWithBlockBody", {K::If, K::GroupBegin, "1", K::GroupEnd, K::BlockBegin, K::BlockEnd}},
        {"IfWithEmptyStatementBody", {K::If, K::GroupBegin, "0", K::GroupEnd, K::StatementEnd}},
        {"NestedIf", {K::If, K::GroupBegin, "1", K::GroupEnd,
            K::If, K::GroupBegin, "1", K::GroupEnd, K::Output, "7", K::StatementEnd}},
        {"WhileWithExpressionBody", {K::While, K::GroupBegin, "1", K::GroupEnd,
            K::Input, K::StatementEnd}},
        {"WhileWithBlockBody", {K::While, K::GroupBegin, "1", K::GroupEnd, K::BlockBegin, K::BlockEnd}},
        {"WhileWithEmptyStatementBody", {K::While, K::GroupBegin, "0", K::GroupEnd, K::StatementEnd}},
    };
    for (const auto& test : cases) {
        SCOPED_TRACE(test.name);
        EXPECT_NE(parse(test.parts), nullptr);
    }
}

TEST(ParserNumberTest, RejectsMalformedNumberTokensFromExternalTokenVector) {
    for (const std::string text : {"", "bad", "12x"}) {
        SCOPED_TRACE(text);
        const std::vector<Token> tokens{
            {K::Number, text, {4, 9}}, {K::StatementEnd, "", {4, 12}}, {K::EndOfFile, "", {4, 13}}};
        try {
            Parser{tokens}.parse_program();
            FAIL() << "Expected invalid number literal";
        } catch (const std::invalid_argument& error) {
            EXPECT_NE(std::string(error.what()).find("Invalid number literal at line 4, column 9"),
                std::string::npos);
        }
    }
}

TEST(ParserDiagnosticTest, ReportsExpectedAndActualKindsWithTokenPosition) {
    const std::vector<Token> tokens{
        {K::Output, std::string(spelling_for(K::Output)), {3, 2}},
        {K::GroupBegin, std::string(spelling_for(K::GroupBegin)), {3, 10}},
        {K::Number, "2", {4, 5}},
        {K::StatementEnd, std::string(spelling_for(K::StatementEnd)), {4, 9}},
        {K::EndOfFile, "", {5, 1}}};
    try {
        Parser{tokens}.parse_program();
        FAIL() << "Expected missing group end";
    } catch (const std::invalid_argument& error) {
        EXPECT_EQ(std::string(error.what()), std::format("Expected {}, got {} at line 4, column 9",
            token_kind_name(K::GroupEnd), token_kind_name(K::StatementEnd)));
    }
}

TEST(ParserSizeTest, ParsesLongExpression) {
    constexpr int count = 512;
    TokenParts parts{K::Output, "1"};
    for (int i = 1; i < count; ++i) {
        parts.insert(parts.end(), {K::Plus, "1"});
    }
    parts.push_back(K::StatementEnd);
    EXPECT_NE(parse(parts), nullptr);
}

TEST(ParserSizeTest, ParsesManyIndependentStatements) {
    constexpr int count = 1024;
    TokenParts parts{"x", K::Assign, "0", K::StatementEnd};
    for (int i = 0; i < count; ++i) {
        parts.insert(parts.end(), {"x", K::Assign, "x", K::Plus, "1", K::StatementEnd});
    }
    parts.insert(parts.end(), {K::Output, "x", K::StatementEnd});
    EXPECT_NE(parse(parts), nullptr);
}

TEST(ParserSizeTest, ParsesNestedGroupsAndBlocks) {
    constexpr int depth = 32;
    TokenParts parts;
    for (int i = 0; i < depth; ++i) {
        parts.push_back(K::BlockBegin);
    }
    parts.push_back(K::Output);
    for (int i = 0; i < depth; ++i) {
        parts.push_back(K::GroupBegin);
    }
    parts.push_back("42");
    for (int i = 0; i < depth; ++i) {
        parts.push_back(K::GroupEnd);
    }
    parts.push_back(K::StatementEnd);
    for (int i = 0; i < depth; ++i) {
        parts.push_back(K::BlockEnd);
    }
    EXPECT_NE(parse(parts), nullptr);
}

TEST(ParserConstructionTest, AcceptsEmptyAndNonemptyTokenSequences) {
    EXPECT_NO_THROW((Parser{make_tokens({})}));
    EXPECT_NO_THROW((Parser{make_tokens({"42"})}));
    EXPECT_NO_THROW((Parser{make_tokens({"name"})}));
}

TEST(ParserConstructionTest, RejectsMissingEndOfFile) {
    EXPECT_THROW((Parser{std::vector<Token>{}}), std::invalid_argument);
    std::vector<Token> tokens{{TokenKind::Number, "42", {1, 1}}};
    EXPECT_THROW((Parser{tokens}), std::invalid_argument);
}

TEST(ParserConstructionTest, RejectsTokensAfterEndOfFile) {
    std::vector<Token> tokens{
        {TokenKind::EndOfFile, "", {1, 1}},
        {TokenKind::Number, "42", {1, 1}},
        {TokenKind::EndOfFile, "", {1, 3}}};
    EXPECT_THROW((Parser{tokens}), std::invalid_argument);
    tokens.erase(tokens.begin() + 1);
    EXPECT_THROW((Parser{tokens}), std::invalid_argument);
}

} // namespace
} // namespace ParaCL
