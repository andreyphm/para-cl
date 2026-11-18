#include "parser.hpp"
#include "lexer.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

namespace ParaCL {
namespace {

TEST(ParserConstructionTest, AcceptsTokenizedEmptyAndNonemptySources) {
    EXPECT_NO_THROW((Parser{tokenize("")}));
    EXPECT_NO_THROW((Parser{tokenize("42")}));
    EXPECT_NO_THROW((Parser{tokenize("name")}));
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
