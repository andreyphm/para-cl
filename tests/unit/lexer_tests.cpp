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

TEST(TokenizeTest, ReturnsOnlyEndOfFileForEmptyOrCommentOnlySource) {
    const auto empty = tokenize("");
    ASSERT_EQ(empty.size(), 1u);
    expect_token(empty.front(), TokenKind::EndOfFile, "", 1, 1);

    const auto comments = tokenize("// comment\n  // another\n");
    ASSERT_EQ(comments.size(), 1u);
    expect_token(comments.front(), TokenKind::EndOfFile, "", 3, 1);
}

TEST(TokenizeTest, PreservesTokenOrderTextPositionsAndFinalEndOfFile) {
    const std::string source = std::format("007\nname\n{}\n{}\n{}\n{} // tail\n",
        spelling_for(TokenKind::Input), spelling_for(TokenKind::GroupBegin),
        spelling_for(TokenKind::GroupEnd), spelling_for(TokenKind::StatementEnd));
    const auto tokens = tokenize(source);
    ASSERT_EQ(tokens.size(), 7u);
    expect_token(tokens[0], TokenKind::Number, "007", 1, 1);
    expect_token(tokens[1], TokenKind::Identifier, "name", 2, 1);
    expect_token(tokens[2], TokenKind::Input, spelling_for(TokenKind::Input), 3, 1);
    expect_token(tokens[3], TokenKind::GroupBegin, spelling_for(TokenKind::GroupBegin), 4, 1);
    expect_token(tokens[4], TokenKind::GroupEnd, spelling_for(TokenKind::GroupEnd), 5, 1);
    expect_token(tokens[5], TokenKind::StatementEnd, spelling_for(TokenKind::StatementEnd), 6, 1);
    expect_token(tokens[6], TokenKind::EndOfFile, "", 7, 1);
}

TEST(TokenizeTest, ReportsLexicalErrorAfterValidTokens) {
    try {
        tokenize(std::format("123\n{}", unknown_symbol()));
        FAIL() << "Expected lexical error";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("line 2, column 1"), std::string::npos);
    }
}

TEST(TokenizeTest, RetainsTokensAcrossVectorGrowth) {
    constexpr std::size_t count = 4096;
    std::ostringstream source;
    for (std::size_t i = 0; i < count; ++i) {
        source << i << '\n';
    }
    const auto tokens = tokenize(source.str());
    ASSERT_EQ(tokens.size(), count + 1);
    for (std::size_t i = 0; i < count; ++i) {
        expect_token(tokens[i], TokenKind::Number, std::format("{}", i), i + 1, 1);
    }
    expect_token(tokens.back(), TokenKind::EndOfFile, "", count + 1, 1);
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
    expect_fixed_sequence({TokenKind::If, TokenKind::Else, TokenKind::While, TokenKind::Output}, false);
}

TEST(LexerTest, MatchesOnlyCompleteCaseSensitiveKeywords) {
    for (const TokenKind kind : {TokenKind::If, TokenKind::Else, TokenKind::While, TokenKind::Output}) {
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
        TokenKind::GreaterEqual, TokenKind::LogicalAnd, TokenKind::LogicalOr, TokenKind::LogicalNot,
        TokenKind::Input, TokenKind::GroupBegin, TokenKind::GroupEnd,
        TokenKind::BlockBegin, TokenKind::BlockEnd, TokenKind::StatementEnd,
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
                           TokenKind::GreaterEqual, TokenKind::LogicalNot, TokenKind::NotEqual,
                           TokenKind::LogicalAnd, TokenKind::LogicalOr}, true);
}

TEST(LexerTest, ReadsOperatorsBetweenOtherTokenKindsAcrossLines) {
    Token expected[] = {
        {TokenKind::Identifier, "x", {1, 0}},
        {TokenKind::Assign, spelling_for(TokenKind::Assign), {1, 0}},
        {TokenKind::Input, spelling_for(TokenKind::Input), {1, 0}},
        {TokenKind::StatementEnd, spelling_for(TokenKind::StatementEnd), {1, 0}},
        {TokenKind::Output, spelling_for(TokenKind::Output), {2, 0}},
        {TokenKind::GroupBegin, spelling_for(TokenKind::GroupBegin), {2, 0}},
        {TokenKind::Minus, spelling_for(TokenKind::Minus), {2, 0}},
        {TokenKind::Number, "42", {2, 0}},
        {TokenKind::GroupEnd, spelling_for(TokenKind::GroupEnd), {2, 0}},
        {TokenKind::StatementEnd, spelling_for(TokenKind::StatementEnd), {2, 0}},
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

TEST(LexerTest, SkipsCommentAtEndOfSourceAndReturnsStableEndOfFile) {
    for (const std::string& source : {std::string("//"), std::string("// comment")}) {
        Lexer lexer{source};
        expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 1, source.size() + 1);
        expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 1, source.size() + 1);
    }
}

TEST(LexerTest, SkipsTrailingCommentWithoutNewlineAfterToken) {
    const std::string source = "12// trailing comment";
    Lexer lexer{source};
    expect_token(lexer.next_token(), TokenKind::Number, "12", 1, 1);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 1, source.size() + 1);
}

TEST(LexerTest, SkipsConsecutiveCommentsAndWhitespaceAcrossLines) {
    Lexer lexer{" \t// first\r\n// second\n \n 7"};
    expect_token(lexer.next_token(), TokenKind::Number, "7", 4, 2);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 4, 3);
}

TEST(LexerTest, KeepsTokensSeparateAcrossComments) {
    Lexer lexer{"12// comment\n34//\nname// end"};
    expect_token(lexer.next_token(), TokenKind::Number, "12", 1, 1);
    expect_token(lexer.next_token(), TokenKind::Number, "34", 2, 1);
    expect_token(lexer.next_token(), TokenKind::Identifier, "name", 3, 1);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 3, 11);
}

TEST(LexerTest, IgnoresTokenSpellingsAndInvalidBytesInsideComments) {
    const std::string source = std::format("// {} {} {} {}\n42",
        spelling_for(TokenKind::Output), spelling_for(TokenKind::Equal),
        unknown_symbol(), std::string{ '\0', static_cast<char>(0xff) });
    Lexer lexer{source};
    expect_token(lexer.next_token(), TokenKind::Number, "42", 2, 1);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 2, 3);
}

TEST(LexerTest, PreservesDivisionBeforeAndAfterComment) {
    const std::string division = spelling_for(TokenKind::Div);
    Lexer lexer{std::format("8 {} 2// ignored\n{}", division, division)};
    expect_token(lexer.next_token(), TokenKind::Number, "8", 1, 1);
    expect_token(lexer.next_token(), TokenKind::Div, division, 1, 3);
    expect_token(lexer.next_token(), TokenKind::Number, "2", 1, division.size() + 4);
    expect_token(lexer.next_token(), TokenKind::Div, division, 2, 1);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 2, division.size() + 1);
}

TEST(LexerTest, SkipsLongCommentWithRepeatedCommentMarkers) {
    Lexer lexer{std::format("//{}\n42", std::string(100000, '/'))};
    expect_token(lexer.next_token(), TokenKind::Number, "42", 2, 1);
    expect_token(lexer.next_token(), TokenKind::EndOfFile, "", 2, 3);
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
