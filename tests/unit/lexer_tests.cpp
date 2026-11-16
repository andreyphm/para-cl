#include "lexer.hpp"
#include "token_spellings.hpp"

#include <gtest/gtest.h>

#include <string>
#include <stdexcept>
#include <format>
#include <initializer_list>
#include <sstream>
#include <string_view>
#include <cctype>

namespace ParaCL {
namespace {

std::string spelling_for(TokenKind kind) {
    for (const auto& spelling : Detail::keyword_spellings) {
        if (spelling.kind == kind) {
            return std::string(spelling.text);
        }
    }
    for (const auto& spelling : Detail::operator_spellings) {
        if (spelling.kind == kind) {
            return std::string(spelling.text);
        }
    }
    throw std::logic_error("Missing token spelling in test source");
}

std::string unknown_symbol() {
    for (int value = 1; value < 128; ++value) {
        const auto symbol = static_cast<unsigned char>(value);
        if (!std::ispunct(symbol) || symbol == '_') {
            continue;
        }
        const std::string candidate(1, static_cast<char>(symbol));
        bool recognized = false;
        for (const auto& spelling : Detail::operator_spellings) {
            if (candidate == spelling.text) {
                recognized = true;
                break;
            }
        }
        if (!recognized) {
            return candidate;
        }
    }
    throw std::logic_error("No unknown punctuation available for test");
}

void expect_token(const Token& token, TokenKind kind, const std::string& text,
                  std::size_t line, std::size_t column) {
    EXPECT_EQ(token.kind, kind);
    EXPECT_EQ(token.text, text);
    EXPECT_EQ(token.position.line, line);
    EXPECT_EQ(token.position.column, column);
}

void expect_fixed_sequence(std::initializer_list<TokenKind> kinds, bool adjacent) {
    std::ostringstream source;
    for (const TokenKind kind : kinds) {
        source << spelling_for(kind);
        if (!adjacent) {
            source << '\n' << ' ';
        }
    }
    Lexer lexer{source.str()};
    std::size_t line = 1;
    std::size_t column = 1;
    for (const TokenKind kind : kinds) {
        const std::string text = spelling_for(kind);
        expect_token(lexer.next_token(), kind, text, line, column);
        if (adjacent) {
            column += text.size();
        } else {
            ++line;
            column = 2;
        }
    }
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", line, column);
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

TEST(LexerTest, ReadsCompleteIdentifiersAndTracksPositions) {
    Lexer lexer{"count _value2 A9\n x"};
    expect_token(lexer.next_token(), TokenKind::Identifier, "count", 1, 1);
    expect_token(lexer.next_token(), TokenKind::Identifier, "_value2", 1, 7);
    expect_token(lexer.next_token(), TokenKind::Identifier, "A9", 1, 15);
    expect_token(lexer.next_token(), TokenKind::Identifier, "x", 2, 2);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 2, 3);
}

TEST(LexerTest, KeepsKeywordPrefixesInsideIdentifiers) {
    for (const TokenKind kind : {TokenKind::If, TokenKind::While, TokenKind::Output}) {
        const std::string keyword = spelling_for(kind);
        for (const std::string& name : {std::format("{}_count", keyword),
                                       std::format("{}2", keyword),
                                       std::format("_{}", keyword)}) {
            Lexer lexer{name};
            expect_token(lexer.next_token(), TokenKind::Identifier, name, 1, 1);
            expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 1, name.size() + 1);
        }
    }
}

TEST(LexerTest, RecognizesKeywordsAndTracksTheirPositions) {
    expect_fixed_sequence({TokenKind::If, TokenKind::While, TokenKind::Output}, false);
}

TEST(LexerTest, MatchesOnlyCompleteCaseSensitiveKeywords) {
    for (const TokenKind kind : {TokenKind::If, TokenKind::While, TokenKind::Output}) {
        std::string changed_case = spelling_for(kind);
        for (char& symbol : changed_case) {
            if (symbol >= 'a' && symbol <= 'z') {
                symbol = static_cast<char>(symbol - 'a' + 'A');
            } else if (symbol >= 'A' && symbol <= 'Z') {
                symbol = static_cast<char>(symbol - 'A' + 'a');
            }
        }
        Lexer lexer{changed_case};
        expect_token(lexer.next_token(), TokenKind::Identifier, changed_case, 1, 1);
        expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 1, changed_case.size() + 1);
    }
}

TEST(LexerTest, RecognizesEveryOperatorAndDelimiterAtEndOfSource) {
    const TokenKind kinds[] = {
        TokenKind::Plus, TokenKind::Minus, TokenKind::Mul, TokenKind::Div,
        TokenKind::Assign, TokenKind::Equal, TokenKind::NotEqual,
        TokenKind::Less, TokenKind::LessEqual, TokenKind::Greater,
        TokenKind::GreaterEqual, TokenKind::LogicalAnd, TokenKind::LogicalOr,
        TokenKind::Input, TokenKind::LeftParen, TokenKind::RightParen,
        TokenKind::LeftBrace, TokenKind::RightBrace, TokenKind::Semicolon,
    };
    for (const TokenKind kind : kinds) {
        const std::string text = spelling_for(kind);
        SCOPED_TRACE(text);
        Lexer lexer{text};
        expect_token(lexer.next_token(), kind, text, 1, 1);
        expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 1, text.size() + 1);
    }
}

TEST(LexerTest, ChoosesLongestOperatorInAdjacentSequence) {
    expect_fixed_sequence({TokenKind::Equal, TokenKind::Assign, TokenKind::LessEqual,
                           TokenKind::GreaterEqual, TokenKind::NotEqual,
                           TokenKind::LogicalAnd, TokenKind::LogicalOr}, true);
}

TEST(LexerTest, ReadsOperatorsBetweenOtherTokenKindsAcrossLines) {
    Token expected[] = {
        {TokenKind::Identifier, "x", {1, 0}},
        {TokenKind::Assign, spelling_for(TokenKind::Assign), {1, 0}},
        {TokenKind::Input, spelling_for(TokenKind::Input), {1, 0}},
        {TokenKind::Semicolon, spelling_for(TokenKind::Semicolon), {1, 0}},
        {TokenKind::Output, spelling_for(TokenKind::Output), {2, 0}},
        {TokenKind::LeftParen, spelling_for(TokenKind::LeftParen), {2, 0}},
        {TokenKind::Minus, spelling_for(TokenKind::Minus), {2, 0}},
        {TokenKind::Number, "42", {2, 0}},
        {TokenKind::RightParen, spelling_for(TokenKind::RightParen), {2, 0}},
        {TokenKind::Semicolon, spelling_for(TokenKind::Semicolon), {2, 0}},
    };
    std::ostringstream source;
    std::size_t line = 1;
    std::size_t column = 1;
    for (Token& token : expected) {
        if (token.position.line != line) {
            source << '\n';
            line = token.position.line;
            column = 1;
        }
        token.position.column = column;
        source << token.text;
        column += token.text.size();
    }
    Lexer lexer{source.str()};
    for (const Token& token : expected) {
        expect_token(lexer.next_token(), token.kind, token.text,
                     token.position.line, token.position.column);
    }
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", line, column);
}

TEST(LexerTest, RejectsUnknownSymbolsAndIncompleteOperatorsWithoutAdvancing) {
    const std::string invalid_symbols[] = {unknown_symbol(), std::string(1, '\0'),
                                          std::string(1, static_cast<char>(0xff))};
    for (const auto& symbol : invalid_symbols) {
        Lexer lexer{symbol};
        EXPECT_THROW(lexer.next_token(), std::runtime_error);
        EXPECT_THROW(lexer.next_token(), std::runtime_error);
    }

    for (const auto& spelling : Detail::operator_spellings) {
        for (std::size_t length = 1; length < spelling.text.size(); ++length) {
            const std::string prefix(spelling.text.substr(0, length));
            bool has_complete_operator = false;
            for (const auto& other : Detail::operator_spellings) {
                if (prefix.starts_with(other.text)) {
                    has_complete_operator = true;
                    break;
                }
            }
            if (!has_complete_operator) {
                SCOPED_TRACE(prefix);
                Lexer lexer{prefix};
                EXPECT_THROW(lexer.next_token(), std::runtime_error);
                EXPECT_THROW(lexer.next_token(), std::runtime_error);
            }
        }
    }
}

TEST(LexerTest, ReportsUnknownSymbolPosition) {
    Lexer lexer{std::format("12\n {}", unknown_symbol())};
    expect_token(lexer.next_token(), TokenKind::Number, "12", 1, 1);
    try {
        lexer.next_token();
        FAIL() << "Expected an unknown symbol error";
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("line 2"), std::string::npos);
        EXPECT_NE(message.find("column 2"), std::string::npos);
    }
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
