#include "ast.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <sstream>
#include <stdexcept>

namespace ParaCL {
namespace {

TEST(ExpressionStatementTest, ExecutesAssignmentThroughBaseInterface) {
    std::istringstream input;
    RuntimeContext context{input};
    const ExpressionStatement expression_statement{
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<NumberExpression>(3))};
    const Statement& statement = expression_statement;

    statement.execute(context);

    EXPECT_EQ(context.get_variable("x"), 3);
}

TEST(ExpressionStatementTest, UsesCurrentStateAndProvidedContext) {
    std::istringstream first_input;
    std::istringstream second_input;
    RuntimeContext first{first_input};
    RuntimeContext second{second_input};
    first.set_variable("x", 3);
    second.set_variable("x", 8);
    const ExpressionStatement statement{
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<BinaryExpression>(BinaryOperator::Add,
                std::make_unique<VariableExpression>("x"),
                std::make_unique<NumberExpression>(2)))};

    statement.execute(first);
    EXPECT_EQ(first.get_variable("x"), 5);
    statement.execute(second);
    EXPECT_EQ(second.get_variable("x"), 10);
    statement.execute(first);
    EXPECT_EQ(first.get_variable("x"), 7);
    EXPECT_EQ(second.get_variable("x"), 10);
}

TEST(ExpressionStatementTest, EvaluatesInputOnceAndDiscardsItsValue) {
    std::istringstream input{"10 -3 99"};
    RuntimeContext context{input};
    const ExpressionStatement expression_statement{std::make_unique<InputExpression>()};
    const Statement& statement = expression_statement;

    statement.execute(context);
    statement.execute(context);

    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
    EXPECT_THROW(statement.execute(context), std::runtime_error);
}

TEST(ExpressionStatementTest, PropagatesUnknownVariableError) {
    std::istringstream input;
    RuntimeContext context{input};
    const ExpressionStatement statement{std::make_unique<VariableExpression>("missing")};

    EXPECT_THROW(statement.execute(context), std::runtime_error);
}

TEST(ExpressionStatementTest, PropagatesArithmeticErrorWithoutAssigning) {
    std::istringstream input;
    RuntimeContext context{input};
    context.set_variable("x", 42);
    const ExpressionStatement statement{
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<BinaryExpression>(BinaryOperator::Div,
                std::make_unique<NumberExpression>(1),
                std::make_unique<NumberExpression>(0)))};

    EXPECT_THROW(statement.execute(context), std::domain_error);
    EXPECT_EQ(context.get_variable("x"), 42);
}

TEST(ExpressionStatementTest, RejectsNullExpression) {
    EXPECT_THROW((ExpressionStatement{nullptr}), std::invalid_argument);
}

class DestructionProbeExpression : public Expression {
public:
    explicit DestructionProbeExpression(int& destruction_count)
        : destruction_count_(destruction_count) {}
    ~DestructionProbeExpression() override { ++destruction_count_; }
    int evaluate(RuntimeContext&) const override { return 1; }

private:
    int& destruction_count_;
};

TEST(ExpressionStatementTest, DestroysOwnedExpressionThroughBaseStatementPointer) {
    int destruction_count = 0;
    {
        const std::unique_ptr<Statement> statement = std::make_unique<ExpressionStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count));
        EXPECT_EQ(destruction_count, 0);
    }

    EXPECT_EQ(destruction_count, 1);
}

} // namespace
} // namespace ParaCL
