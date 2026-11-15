#include "ast.hpp"

#include <gtest/gtest.h>

#include <array>
#include <format>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace ParaCL {
namespace {

TEST(NumberExpressionTest, EvaluatesIntValuesThroughBaseInterface) {
    std::istringstream input;
    RuntimeContext context{input};
    const std::array values{0, 42, -17,
        std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};

    for (const int value : values) {
        SCOPED_TRACE(value);
        const NumberExpression number{value};
        const Expression& expression = number;
        EXPECT_EQ(expression.evaluate(context), value);
    }
}

struct BinaryCase {
    const char* name;
    BinaryOperator operation;
    int left;
    int right;
    int expected;
};

class BinaryExpressionTest : public testing::TestWithParam<BinaryCase> {};

TEST_P(BinaryExpressionTest, EvaluatesThroughBaseInterface) {
    std::istringstream input;
    RuntimeContext context{input};
    const auto& test_case = GetParam();
    const BinaryExpression binary{test_case.operation,
        std::make_unique<NumberExpression>(test_case.left),
        std::make_unique<NumberExpression>(test_case.right)};
    const Expression& expression = binary;

    EXPECT_EQ(expression.evaluate(context), test_case.expected);
}

INSTANTIATE_TEST_SUITE_P(Arithmetic, BinaryExpressionTest, testing::Values(
    BinaryCase{"AddPositive", BinaryOperator::Add, 2, 3, 5},
    BinaryCase{"AddMixedSigns", BinaryOperator::Add, -5, 2, -3},
    BinaryCase{"AddZero", BinaryOperator::Add, 4, 0, 4},
    BinaryCase{"AddAtMax", BinaryOperator::Add, std::numeric_limits<int>::max(), 0,
        std::numeric_limits<int>::max()},
    BinaryCase{"SubtractPositive", BinaryOperator::Sub, 2, 5, -3},
    BinaryCase{"SubtractNegative", BinaryOperator::Sub, -5, -2, -3},
    BinaryCase{"SubtractMaxFromZero", BinaryOperator::Sub, 0, std::numeric_limits<int>::max(),
        -std::numeric_limits<int>::max()},
    BinaryCase{"MultiplyPositive", BinaryOperator::Mul, 6, 7, 42},
    BinaryCase{"MultiplyMixedSigns", BinaryOperator::Mul, -6, 7, -42},
    BinaryCase{"MultiplyNegative", BinaryOperator::Mul, -6, -7, 42},
    BinaryCase{"MultiplyByZero", BinaryOperator::Mul, 42, 0, 0},
    BinaryCase{"DividePositive", BinaryOperator::Div, 7, 2, 3},
    BinaryCase{"DivideNegativeDividend", BinaryOperator::Div, -7, 2, -3},
    BinaryCase{"DivideNegativeDivisor", BinaryOperator::Div, 7, -2, -3},
    BinaryCase{"DivideNegativeOperands", BinaryOperator::Div, -7, -2, 3},
    BinaryCase{"DivideZero", BinaryOperator::Div, 0, 2, 0},
    BinaryCase{"DivideAtMin", BinaryOperator::Div, std::numeric_limits<int>::min(), 1,
        std::numeric_limits<int>::min()}),
    [](const testing::TestParamInfo<BinaryCase>& info) { return info.param.name; });

TEST(BinaryExpressionErrorTest, RejectsDivisionByZero) {
    std::istringstream input;
    RuntimeContext context{input};
    const BinaryExpression expression{BinaryOperator::Div,
        std::make_unique<NumberExpression>(7),
        std::make_unique<NumberExpression>(0)};

    EXPECT_THROW(expression.evaluate(context), std::domain_error);
}

TEST(BinaryExpressionErrorTest, RejectsNullOperands) {
    EXPECT_THROW((BinaryExpression{BinaryOperator::Add, nullptr,
        std::make_unique<NumberExpression>(1)}), std::invalid_argument);
    EXPECT_THROW((BinaryExpression{BinaryOperator::Add,
        std::make_unique<NumberExpression>(1), nullptr}), std::invalid_argument);
    EXPECT_THROW((BinaryExpression{BinaryOperator::Add, nullptr, nullptr}),
        std::invalid_argument);
}

TEST(BinaryExpressionErrorTest, RejectsUnknownOperator) {
    std::istringstream input;
    RuntimeContext context{input};
    const BinaryExpression expression{static_cast<BinaryOperator>(99),
        std::make_unique<NumberExpression>(1),
        std::make_unique<NumberExpression>(2)};

    EXPECT_THROW(expression.evaluate(context), std::logic_error);
}

TEST(ExpressionTreeTest, EvaluatesNestedArithmetic) {
    std::istringstream input;
    RuntimeContext context{input};
    const BinaryExpression expression{BinaryOperator::Add,
        std::make_unique<NumberExpression>(2),
        std::make_unique<BinaryExpression>(BinaryOperator::Mul,
            std::make_unique<NumberExpression>(3),
            std::make_unique<NumberExpression>(4))};

    EXPECT_EQ(expression.evaluate(context), 14);
}

TEST(UnaryExpressionTest, EvaluatesBothOperators) {
    std::istringstream input;
    RuntimeContext context{input};
    for (const int value : {0, 5, -5, std::numeric_limits<int>::max()}) {
        SCOPED_TRACE(value);
        const UnaryExpression plus{UnaryOperator::Plus,
            std::make_unique<NumberExpression>(value)};
        const UnaryExpression minus{UnaryOperator::Minus,
            std::make_unique<NumberExpression>(value)};
        const Expression& positive = plus;
        const Expression& negative = minus;

        EXPECT_EQ(positive.evaluate(context), value);
        EXPECT_EQ(negative.evaluate(context), -value);
    }
}

TEST(UnaryExpressionTest, EvaluatesNestedNegation) {
    std::istringstream input;
    RuntimeContext context{input};
    const UnaryExpression expression{UnaryOperator::Minus,
        std::make_unique<UnaryExpression>(UnaryOperator::Minus,
            std::make_unique<NumberExpression>(5))};

    EXPECT_EQ(expression.evaluate(context), 5);
}

TEST(UnaryExpressionTest, RejectsNullOperand) {
    EXPECT_THROW((UnaryExpression{UnaryOperator::Minus, nullptr}),
        std::invalid_argument);
}

TEST(UnaryExpressionTest, RejectsUnknownOperator) {
    std::istringstream input;
    RuntimeContext context{input};
    const UnaryExpression expression{static_cast<UnaryOperator>(99),
        std::make_unique<NumberExpression>(1)};

    EXPECT_THROW(expression.evaluate(context), std::logic_error);
}

TEST(VariableExpressionTest, ReadsUpdatedValueThroughNestedExpressions) {
    std::istringstream input;
    RuntimeContext context{input};
    const BinaryExpression expression{BinaryOperator::Add,
        std::make_unique<UnaryExpression>(UnaryOperator::Minus,
            std::make_unique<VariableExpression>("x")),
        std::make_unique<NumberExpression>(2)};

    context.set_variable("x", 3);
    EXPECT_EQ(expression.evaluate(context), -1);
    context.set_variable("x", 7);
    EXPECT_EQ(expression.evaluate(context), -5);
}

TEST(VariableExpressionTest, UsesContextProvidedForEachEvaluation) {
    std::istringstream first_input;
    std::istringstream second_input;
    RuntimeContext first{first_input};
    RuntimeContext second{second_input};
    first.set_variable("x", 3);
    second.set_variable("x", 7);
    const VariableExpression variable{"x"};
    const Expression& expression = variable;

    EXPECT_EQ(expression.evaluate(first), 3);
    EXPECT_EQ(expression.evaluate(second), 7);
}

TEST(VariableExpressionTest, OwnsNameIndependentlyOfOriginalString) {
    std::istringstream input;
    RuntimeContext context{input};
    context.set_variable("x", 42);
    std::string name = "x";
    const VariableExpression expression{name};
    name.assign(100, 'y');

    EXPECT_EQ(expression.evaluate(context), 42);
}

TEST(VariableExpressionTest, ReportsUnknownName) {
    std::istringstream input;
    RuntimeContext context{input};
    const VariableExpression expression{"missing"};

    EXPECT_THROW(expression.evaluate(context), std::runtime_error);
}

TEST(InputExpressionTest, ReadsNextValueOnEveryEvaluation) {
    std::istringstream input{"  10\n-3\t+4 0"};
    RuntimeContext context{input};
    const InputExpression read;
    const Expression& expression = read;

    EXPECT_EQ(expression.evaluate(context), 10);
    EXPECT_EQ(expression.evaluate(context), -3);
    EXPECT_EQ(expression.evaluate(context), 4);
    EXPECT_EQ(expression.evaluate(context), 0);
    EXPECT_THROW(expression.evaluate(context), std::runtime_error);
}

TEST(InputExpressionTest, ReadsIntLimits) {
    std::istringstream input{std::format("{} {}",
        std::numeric_limits<int>::min(), std::numeric_limits<int>::max())};
    RuntimeContext context{input};
    const InputExpression expression;

    EXPECT_EQ(expression.evaluate(context), std::numeric_limits<int>::min());
    EXPECT_EQ(expression.evaluate(context), std::numeric_limits<int>::max());
}

TEST(InputExpressionTest, RejectsInvalidOrMissingInput) {
    const InputExpression expression;
    for (const std::string text : {"", " \n\t", "abc", "+", "-"}) {
        SCOPED_TRACE(text);
        std::istringstream input{text};
        RuntimeContext context{input};
        EXPECT_THROW(expression.evaluate(context), std::runtime_error);
    }
}

TEST(InputExpressionTest, RejectsValuesOutsideIntRange) {
    const std::string above_max = std::format("{}0", std::numeric_limits<int>::max());
    const std::string below_min = std::format("{}0", std::numeric_limits<int>::min());
    const InputExpression expression;

    for (const auto& text : {above_max, below_min}) {
        SCOPED_TRACE(text);
        std::istringstream input{text};
        RuntimeContext context{input};
        EXPECT_THROW(expression.evaluate(context), std::runtime_error);
    }
}

TEST(InputExpressionTest, UsesFormattedStreamNumericPrefixRules) {
    std::istringstream input{"12abc"};
    RuntimeContext context{input};
    const InputExpression expression;

    EXPECT_EQ(expression.evaluate(context), 12);
    EXPECT_EQ(input.peek(), 'a');
    EXPECT_THROW(expression.evaluate(context), std::runtime_error);
}

TEST(ExpressionTreeTest, EvaluatesInputOperandsOnceFromLeftToRight) {
    std::istringstream input{"10 3 99"};
    RuntimeContext context{input};
    const BinaryExpression expression{BinaryOperator::Sub,
        std::make_unique<InputExpression>(), std::make_unique<InputExpression>()};

    EXPECT_EQ(expression.evaluate(context), 7);
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(ExpressionTreeTest, PropagatesOperandErrors) {
    std::istringstream input;
    RuntimeContext context{input};
    const BinaryExpression expression{BinaryOperator::Add,
        std::make_unique<NumberExpression>(1),
        std::make_unique<VariableExpression>("missing")};

    EXPECT_THROW(expression.evaluate(context), std::runtime_error);
}

class TrackedExpression : public Expression {
public:
    explicit TrackedExpression(int& destruction_count) : destruction_count_(destruction_count) {}
    ~TrackedExpression() override { ++destruction_count_; }
    int evaluate(RuntimeContext&) const override { return 1; }

private:
    int& destruction_count_;
};

TEST(ExpressionOwnershipTest, DestroysNestedTreeThroughBasePointer) {
    int destruction_count = 0;
    {
        const std::unique_ptr<Expression> expression = std::make_unique<BinaryExpression>(
            BinaryOperator::Add,
            std::make_unique<TrackedExpression>(destruction_count),
            std::make_unique<UnaryExpression>(UnaryOperator::Minus,
                std::make_unique<TrackedExpression>(destruction_count)));
        EXPECT_EQ(destruction_count, 0);
    }
    EXPECT_EQ(destruction_count, 2);
}

TEST(ExpressionOwnershipTest, ReleasesOperandWhenConstructorRejectsOtherOperand) {
    int destruction_count = 0;
    EXPECT_THROW((BinaryExpression{BinaryOperator::Add,
        std::make_unique<TrackedExpression>(destruction_count), nullptr}),
        std::invalid_argument);
    EXPECT_EQ(destruction_count, 1);
    EXPECT_THROW((BinaryExpression{BinaryOperator::Add, nullptr,
        std::make_unique<TrackedExpression>(destruction_count)}),
        std::invalid_argument);
    EXPECT_EQ(destruction_count, 2);
}

} // namespace
} // namespace ParaCL
