#include "ast.hpp"

#include <gtest/gtest.h>

#include <array>
#include <format>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ParaCL {
namespace {

TEST(NumberExpressionTest, EvaluatesIntValuesThroughBaseInterface) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
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
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
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

INSTANTIATE_TEST_SUITE_P(Comparisons, BinaryExpressionTest, testing::Values(
    BinaryCase{"EqualLess", BinaryOperator::Equal, -2, 3, 0},
    BinaryCase{"EqualSame", BinaryOperator::Equal, -2, -2, 1},
    BinaryCase{"EqualGreater", BinaryOperator::Equal, 3, -2, 0},
    BinaryCase{"NotEqualLess", BinaryOperator::NotEqual, -2, 3, 1},
    BinaryCase{"NotEqualSame", BinaryOperator::NotEqual, -2, -2, 0},
    BinaryCase{"NotEqualGreater", BinaryOperator::NotEqual, 3, -2, 1},
    BinaryCase{"LessLess", BinaryOperator::Less, -2, 3, 1},
    BinaryCase{"LessSame", BinaryOperator::Less, -2, -2, 0},
    BinaryCase{"LessGreater", BinaryOperator::Less, 3, -2, 0},
    BinaryCase{"LessEqualLess", BinaryOperator::LessEqual, -2, 3, 1},
    BinaryCase{"LessEqualSame", BinaryOperator::LessEqual, -2, -2, 1},
    BinaryCase{"LessEqualGreater", BinaryOperator::LessEqual, 3, -2, 0},
    BinaryCase{"GreaterLess", BinaryOperator::Greater, -2, 3, 0},
    BinaryCase{"GreaterSame", BinaryOperator::Greater, -2, -2, 0},
    BinaryCase{"GreaterGreater", BinaryOperator::Greater, 3, -2, 1},
    BinaryCase{"GreaterEqualLess", BinaryOperator::GreaterEqual, -2, 3, 0},
    BinaryCase{"GreaterEqualSame", BinaryOperator::GreaterEqual, -2, -2, 1},
    BinaryCase{"GreaterEqualGreater", BinaryOperator::GreaterEqual, 3, -2, 1}),
    [](const testing::TestParamInfo<BinaryCase>& info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(Logic, BinaryExpressionTest, testing::Values(
    BinaryCase{"AndBothZero", BinaryOperator::LogicalAnd, 0, 0, 0},
    BinaryCase{"AndLeftZero", BinaryOperator::LogicalAnd, 0, 7, 0},
    BinaryCase{"AndRightZero", BinaryOperator::LogicalAnd, 7, 0, 0},
    BinaryCase{"AndBothNonzero", BinaryOperator::LogicalAnd, 7, 2, 1},
    BinaryCase{"AndNegative", BinaryOperator::LogicalAnd, -7, -2, 1},
    BinaryCase{"OrBothZero", BinaryOperator::LogicalOr, 0, 0, 0},
    BinaryCase{"OrLeftZero", BinaryOperator::LogicalOr, 0, 7, 1},
    BinaryCase{"OrRightZero", BinaryOperator::LogicalOr, 7, 0, 1},
    BinaryCase{"OrBothNonzero", BinaryOperator::LogicalOr, 7, 2, 1},
    BinaryCase{"OrNegative", BinaryOperator::LogicalOr, 0, -2, 1}),
    [](const testing::TestParamInfo<BinaryCase>& info) { return info.param.name; });

class RecordingExpression : public Expression {
public:
    RecordingExpression(int value, int marker, std::vector<int>& calls)
        : value_(value), marker_(marker), calls_(calls) {}

    int evaluate(RuntimeContext&) const override {
        calls_.push_back(marker_);
        return value_;
    }

private:
    int value_;
    int marker_;
    std::vector<int>& calls_;
};

TEST(BinaryExpressionOrderTest, EvaluatesRequiredOperandsOnceFromLeftToRight) {
    for (const auto operation : {BinaryOperator::Add, BinaryOperator::Sub,
             BinaryOperator::Mul, BinaryOperator::Div, BinaryOperator::Equal,
             BinaryOperator::NotEqual, BinaryOperator::Less, BinaryOperator::LessEqual,
             BinaryOperator::Greater, BinaryOperator::GreaterEqual,
             BinaryOperator::LogicalAnd, BinaryOperator::LogicalOr}) {
        SCOPED_TRACE(static_cast<int>(operation));
        std::istringstream input;
        std::ostringstream context_output;
        RuntimeContext context{input, context_output};
        std::vector<int> calls;
        const int left_value = operation == BinaryOperator::LogicalOr ? 0 : 6;
        const BinaryExpression expression{operation,
            std::make_unique<RecordingExpression>(left_value, 1, calls),
            std::make_unique<RecordingExpression>(2, 2, calls)};

        (void)expression.evaluate(context);
        EXPECT_EQ(calls, (std::vector<int>{1, 2}));
    }
}

TEST(LogicalExpressionTest, ShortCircuitPreservesUnreadInput) {
    for (const auto operation : {BinaryOperator::LogicalAnd, BinaryOperator::LogicalOr}) {
        SCOPED_TRACE(static_cast<int>(operation));
        const int left_value = operation == BinaryOperator::LogicalAnd ? 0 : -2;
        std::istringstream input{std::format("{} 99", left_value)};
        std::ostringstream context_output;
        RuntimeContext context{input, context_output};
        const BinaryExpression expression{operation,
            std::make_unique<InputExpression>(), std::make_unique<InputExpression>()};

        EXPECT_EQ(expression.evaluate(context), left_value != 0);
        EXPECT_EQ(InputExpression{}.evaluate(context), 99);
    }
}

TEST(LogicalExpressionTest, ShortCircuitSkipsDivisionByZero) {
    for (const auto operation : {BinaryOperator::LogicalAnd, BinaryOperator::LogicalOr}) {
        SCOPED_TRACE(static_cast<int>(operation));
        std::istringstream input;
        std::ostringstream context_output;
        RuntimeContext context{input, context_output};
        const int left_value = operation == BinaryOperator::LogicalAnd ? 0 : -2;
        const BinaryExpression expression{operation,
            std::make_unique<NumberExpression>(left_value),
            std::make_unique<BinaryExpression>(BinaryOperator::Div,
                std::make_unique<NumberExpression>(1),
                std::make_unique<NumberExpression>(0))};

        EXPECT_EQ(expression.evaluate(context), left_value != 0);
    }
}

TEST(LogicalExpressionTest, EvaluatesRequiredRightOperandAndPropagatesItsError) {
    for (const auto operation : {BinaryOperator::LogicalAnd, BinaryOperator::LogicalOr}) {
        SCOPED_TRACE(static_cast<int>(operation));
        std::istringstream input;
        std::ostringstream context_output;
        RuntimeContext context{input, context_output};
        const int left_value = operation == BinaryOperator::LogicalAnd ? -2 : 0;
        const BinaryExpression expression{operation,
            std::make_unique<NumberExpression>(left_value),
            std::make_unique<BinaryExpression>(BinaryOperator::Div,
                std::make_unique<NumberExpression>(1),
                std::make_unique<NumberExpression>(0))};

        EXPECT_THROW(expression.evaluate(context), std::domain_error);
    }
}

TEST(BinaryExpressionOrderTest, LeftOperandErrorPreventsRightOperandInput) {
    for (const auto operation : {BinaryOperator::Add, BinaryOperator::Sub,
             BinaryOperator::Mul, BinaryOperator::Div, BinaryOperator::Equal,
             BinaryOperator::NotEqual, BinaryOperator::Less, BinaryOperator::LessEqual,
             BinaryOperator::Greater, BinaryOperator::GreaterEqual,
             BinaryOperator::LogicalAnd, BinaryOperator::LogicalOr}) {
        SCOPED_TRACE(static_cast<int>(operation));
        std::istringstream input{"99"};
        std::ostringstream context_output;
        RuntimeContext context{input, context_output};
        const BinaryExpression expression{operation,
            std::make_unique<VariableExpression>("missing"),
            std::make_unique<InputExpression>()};

        EXPECT_THROW(expression.evaluate(context), std::runtime_error);
        EXPECT_EQ(InputExpression{}.evaluate(context), 99);
    }
}

TEST(BinaryExpressionErrorTest, RejectsDivisionByZero) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
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
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const BinaryExpression expression{static_cast<BinaryOperator>(99),
        std::make_unique<NumberExpression>(1),
        std::make_unique<NumberExpression>(2)};

    EXPECT_THROW(expression.evaluate(context), std::logic_error);
}

TEST(ExpressionTreeTest, EvaluatesNestedArithmetic) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const BinaryExpression expression{BinaryOperator::Add,
        std::make_unique<NumberExpression>(2),
        std::make_unique<BinaryExpression>(BinaryOperator::Mul,
            std::make_unique<NumberExpression>(3),
            std::make_unique<NumberExpression>(4))};

    EXPECT_EQ(expression.evaluate(context), 14);
}

TEST(UnaryExpressionTest, EvaluatesUnaryOperators) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    for (const int value : {0, 5, -5, std::numeric_limits<int>::max()}) {
        SCOPED_TRACE(value);
        const UnaryExpression plus{UnaryOperator::Plus,
            std::make_unique<NumberExpression>(value)};
        const UnaryExpression minus{UnaryOperator::Minus,
            std::make_unique<NumberExpression>(value)};
        const UnaryExpression logical_not{UnaryOperator::LogicalNot,
            std::make_unique<NumberExpression>(value)};
        const Expression& positive = plus;
        const Expression& negative = minus;
        const Expression& negated = logical_not;

        EXPECT_EQ(positive.evaluate(context), value);
        EXPECT_EQ(negative.evaluate(context), -value);
        EXPECT_EQ(negated.evaluate(context), value == 0 ? 1 : 0);
    }
}

TEST(UnaryExpressionTest, EvaluatesNestedNegation) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
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
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const UnaryExpression expression{static_cast<UnaryOperator>(99),
        std::make_unique<NumberExpression>(1)};

    EXPECT_THROW(expression.evaluate(context), std::logic_error);
}

TEST(VariableExpressionTest, ReadsUpdatedValueThroughNestedExpressions) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
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
    std::ostringstream first_output;
    RuntimeContext first{first_input, first_output};
    std::ostringstream second_output;
    RuntimeContext second{second_input, second_output};
    first.set_variable("x", 3);
    second.set_variable("x", 7);
    const VariableExpression variable{"x"};
    const Expression& expression = variable;

    EXPECT_EQ(expression.evaluate(first), 3);
    EXPECT_EQ(expression.evaluate(second), 7);
}

TEST(VariableExpressionTest, OwnsNameIndependentlyOfOriginalString) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    context.set_variable("x", 42);
    std::string name = "x";
    const VariableExpression expression{name};
    name.assign(100, 'y');

    EXPECT_EQ(expression.evaluate(context), 42);
}

TEST(VariableExpressionTest, ReportsUnknownName) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const VariableExpression expression{"missing"};

    EXPECT_THROW(expression.evaluate(context), std::runtime_error);
}

TEST(AssignmentExpressionTest, CreatesVariableAndReturnsValueThroughBaseInterface) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const AssignmentExpression assignment{"x", std::make_unique<NumberExpression>(3)};
    const Expression& expression = assignment;

    EXPECT_EQ(expression.evaluate(context), 3);
    EXPECT_EQ(context.get_variable("x"), 3);
}

TEST(AssignmentExpressionTest, ReadsPreviousValueBeforeUpdatingVariable) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    context.set_variable("x", 3);
    const AssignmentExpression expression{"x",
        std::make_unique<BinaryExpression>(BinaryOperator::Add,
            std::make_unique<VariableExpression>("x"),
            std::make_unique<NumberExpression>(2))};

    EXPECT_EQ(expression.evaluate(context), 5);
    EXPECT_EQ(context.get_variable("x"), 5);
    EXPECT_EQ(expression.evaluate(context), 7);
    EXPECT_EQ(context.get_variable("x"), 7);
}

TEST(AssignmentExpressionTest, EvaluatesRightHandSideOncePerAssignment) {
    std::istringstream input{"10 -3 99"};
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const AssignmentExpression expression{"x", std::make_unique<InputExpression>()};

    EXPECT_EQ(expression.evaluate(context), 10);
    EXPECT_EQ(context.get_variable("x"), 10);
    EXPECT_EQ(expression.evaluate(context), -3);
    EXPECT_EQ(context.get_variable("x"), -3);
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(AssignmentExpressionTest, SupportsChainedAssignments) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const AssignmentExpression expression{"x",
        std::make_unique<AssignmentExpression>("y",
            std::make_unique<NumberExpression>(3))};

    EXPECT_EQ(expression.evaluate(context), 3);
    EXPECT_EQ(context.get_variable("x"), 3);
    EXPECT_EQ(context.get_variable("y"), 3);
}

TEST(AssignmentExpressionTest, UsesAssignmentResultInsideArithmetic) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const AssignmentExpression expression{"x",
        std::make_unique<BinaryExpression>(BinaryOperator::Add,
            std::make_unique<AssignmentExpression>("y",
                std::make_unique<NumberExpression>(3)),
            std::make_unique<NumberExpression>(2))};

    EXPECT_EQ(expression.evaluate(context), 5);
    EXPECT_EQ(context.get_variable("x"), 5);
    EXPECT_EQ(context.get_variable("y"), 3);
}

TEST(AssignmentExpressionTest, KeepsPreviousValueWhenRightHandSideFails) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    context.set_variable("x", 42);
    const AssignmentExpression expression{"x",
        std::make_unique<BinaryExpression>(BinaryOperator::Div,
            std::make_unique<NumberExpression>(1),
            std::make_unique<NumberExpression>(0))};

    EXPECT_THROW(expression.evaluate(context), std::domain_error);
    EXPECT_EQ(context.get_variable("x"), 42);
}

TEST(AssignmentExpressionTest, DoesNotCreateVariableWhenRightHandSideFails) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const AssignmentExpression expression{"x", std::make_unique<VariableExpression>("x")};

    EXPECT_THROW(expression.evaluate(context), std::runtime_error);
    EXPECT_THROW(context.get_variable("x"), std::runtime_error);
}

TEST(AssignmentExpressionTest, UpdatesExistingVariableInOuterScope) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    context.set_variable("x", 1);
    {
        const ScopeOwner scope{context};
        const AssignmentExpression outer{"x", std::make_unique<NumberExpression>(2)};
        const AssignmentExpression local{"y", std::make_unique<NumberExpression>(3)};
        EXPECT_EQ(outer.evaluate(context), 2);
        EXPECT_EQ(local.evaluate(context), 3);
    }

    EXPECT_EQ(context.get_variable("x"), 2);
    EXPECT_THROW(context.get_variable("y"), std::runtime_error);
}

TEST(AssignmentExpressionTest, ShortCircuitSkipsAssignmentSideEffects) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    context.set_variable("x", 42);
    for (const auto operation : {BinaryOperator::LogicalAnd, BinaryOperator::LogicalOr}) {
        SCOPED_TRACE(static_cast<int>(operation));
        const int left_value = operation == BinaryOperator::LogicalAnd ? 0 : 1;
        const BinaryExpression expression{operation,
            std::make_unique<NumberExpression>(left_value),
            std::make_unique<AssignmentExpression>("x",
                std::make_unique<NumberExpression>(3))};

        EXPECT_EQ(expression.evaluate(context), left_value);
        EXPECT_EQ(context.get_variable("x"), 42);
    }
}

TEST(AssignmentExpressionTest, OwnsNameIndependentlyOfOriginalString) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    std::string name = "x";
    const AssignmentExpression expression{name, std::make_unique<NumberExpression>(3)};
    name.assign(100, 'y');

    EXPECT_EQ(expression.evaluate(context), 3);
    EXPECT_EQ(context.get_variable("x"), 3);
    EXPECT_THROW(context.get_variable(name), std::runtime_error);
}

TEST(AssignmentExpressionTest, RejectsNullRightHandSide) {
    EXPECT_THROW((AssignmentExpression{"x", nullptr}), std::invalid_argument);
}

TEST(InputExpressionTest, ReadsNextValueOnEveryEvaluation) {
    std::istringstream input{"  10\n-3\t+4 0"};
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
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
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const InputExpression expression;

    EXPECT_EQ(expression.evaluate(context), std::numeric_limits<int>::min());
    EXPECT_EQ(expression.evaluate(context), std::numeric_limits<int>::max());
}

TEST(InputExpressionTest, RejectsInvalidOrMissingInput) {
    const InputExpression expression;
    for (const std::string text : {"", " \n\t", "abc", "+", "-"}) {
        SCOPED_TRACE(text);
        std::istringstream input{text};
        std::ostringstream context_output;
        RuntimeContext context{input, context_output};
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
        std::ostringstream context_output;
        RuntimeContext context{input, context_output};
        EXPECT_THROW(expression.evaluate(context), std::runtime_error);
    }
}

TEST(InputExpressionTest, UsesFormattedStreamNumericPrefixRules) {
    std::istringstream input{"12abc"};
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const InputExpression expression;

    EXPECT_EQ(expression.evaluate(context), 12);
    EXPECT_EQ(input.peek(), 'a');
    EXPECT_THROW(expression.evaluate(context), std::runtime_error);
}

TEST(ExpressionTreeTest, EvaluatesInputOperandsOnceFromLeftToRight) {
    std::istringstream input{"10 3 99"};
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const BinaryExpression expression{BinaryOperator::Sub,
        std::make_unique<InputExpression>(), std::make_unique<InputExpression>()};

    EXPECT_EQ(expression.evaluate(context), 7);
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(ExpressionTreeTest, PropagatesOperandErrors) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
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
