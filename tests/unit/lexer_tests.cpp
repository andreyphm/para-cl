#include "lexer.hpp"

#include <gtest/gtest.h>

#include <string>

namespace ParaCL {
namespace {

void expect_token(const Token& token, TokenKind kind, const std::string& text,
                  std::size_t line, std::size_t column) {
    EXPECT_EQ(token.kind, kind);
    EXPECT_EQ(token.text, text);
    EXPECT_EQ(token.position.line, line);
    EXPECT_EQ(token.position.column, column);
}

TEST(LexerTest, ReturnsStableEndOfFileForEmptySource) {
    Lexer lexer{""};
    for (int i = 0; i < 3; ++i) {
        expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 1, 1);
    }
}

TEST(LexerTest, SkipsAllStandardWhitespace) {
    Lexer lexer{" \t\r\n\v\f"};
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 2, 3);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 2, 3);
}

TEST(LexerTest, ReadsZeroAtEndOfSource) {
    Lexer lexer{"0"};
    expect_token(lexer.next_token(), TokenKind::Number, "0", 1, 1);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 1, 2);
}

TEST(LexerTest, PreservesDigitsWithoutIntConversion) {
    const std::string digits = "000123456789012345678901234567890";
    Lexer lexer{digits};
    expect_token(lexer.next_token(), TokenKind::Number, digits, 1, 1);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 1, digits.size() + 1);
}

TEST(LexerTest, TracksPositionsAcrossNumbersAndLines) {
    Lexer lexer{" 12\t345\r\n 6\n\n78 "};
    expect_token(lexer.next_token(), TokenKind::Number, "12", 1, 2);
    expect_token(lexer.next_token(), TokenKind::Number, "345", 1, 5);
    expect_token(lexer.next_token(), TokenKind::Number, "6", 2, 2);
    expect_token(lexer.next_token(), TokenKind::Number, "78", 4, 1);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 4, 4);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 4, 4);
}

TEST(LexerTest, ReturnedTokenSurvivesLexerDestruction) {
    const Token token = [] {
        Lexer lexer{std::string(100, '7')};
        return lexer.next_token();
    }();
    expect_token(token, TokenKind::Number, std::string(100, '7'), 1, 1);
}

} // namespace
} // namespace ParaCL
