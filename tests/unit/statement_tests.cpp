#include "ast.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <format>
#include <ios>
#include <limits>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <utility>
#include <vector>

namespace ParaCL {
namespace {

TEST(ExpressionStatementTest, ExecutesAssignmentThroughBaseInterface) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const ExpressionStatement expression_statement{
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<NumberExpression>(3))};
    const Statement& statement = expression_statement;

    statement.execute(context);

    EXPECT_EQ(context.get_variable("x"), 3);
    EXPECT_TRUE(context_output.str().empty());
}

TEST(ExpressionStatementTest, UsesCurrentStateAndProvidedContext) {
    std::istringstream first_input;
    std::istringstream second_input;
    std::ostringstream first_output;
    RuntimeContext first{first_input, first_output};
    std::ostringstream second_output;
    RuntimeContext second{second_input, second_output};
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
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const ExpressionStatement expression_statement{std::make_unique<InputExpression>()};
    const Statement& statement = expression_statement;

    statement.execute(context);
    statement.execute(context);

    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
    EXPECT_TRUE(context_output.str().empty());
    EXPECT_THROW(statement.execute(context), std::runtime_error);
}

TEST(ExpressionStatementTest, DoesNotDependOnOutputStreamState) {
    std::istringstream input;
    std::ostringstream output;
    output.setstate(std::ios::badbit);
    RuntimeContext context{input, output};
    const ExpressionStatement statement{
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<NumberExpression>(3))};

    EXPECT_NO_THROW(statement.execute(context));
    EXPECT_EQ(context.get_variable("x"), 3);
}

TEST(ExpressionStatementTest, PropagatesUnknownVariableError) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
    const ExpressionStatement statement{std::make_unique<VariableExpression>("missing")};

    EXPECT_THROW(statement.execute(context), std::runtime_error);
}

TEST(ExpressionStatementTest, PropagatesArithmeticErrorWithoutAssigning) {
    std::istringstream input;
    std::ostringstream context_output;
    RuntimeContext context{input, context_output};
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

TEST(PrintStatementTest, PrintsIntValuesWithNewlineThroughBaseInterface) {
    for (const int value : {0, 42, -7,
             std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
        SCOPED_TRACE(value);
        std::istringstream input;
        std::ostringstream output;
        RuntimeContext context{input, output};
        const PrintStatement print{std::make_unique<NumberExpression>(value)};
        const Statement& statement = print;

        statement.execute(context);

        EXPECT_EQ(output.str(), std::format("{}\n", value));
    }
}

TEST(PrintStatementTest, EvaluatesInputOnceOnEachExecution) {
    std::istringstream input{"10 -3 99"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    const PrintStatement statement{std::make_unique<InputExpression>()};

    statement.execute(context);
    statement.execute(context);

    EXPECT_EQ(output.str(), "10\n-3\n");
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(PrintStatementTest, PrintsAssignmentResultAndPreservesItsSideEffect) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    const PrintStatement statement{
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<BinaryExpression>(BinaryOperator::Add,
                std::make_unique<NumberExpression>(3),
                std::make_unique<NumberExpression>(2)))};

    statement.execute(context);

    EXPECT_EQ(output.str(), "5\n");
    EXPECT_EQ(context.get_variable("x"), 5);
}

TEST(PrintStatementTest, UsesVariablesAndOutputOfProvidedContext) {
    std::istringstream first_input;
    std::istringstream second_input;
    std::ostringstream first_output;
    std::ostringstream second_output;
    RuntimeContext first{first_input, first_output};
    RuntimeContext second{second_input, second_output};
    first.set_variable("x", 3);
    second.set_variable("x", 7);
    const PrintStatement statement{std::make_unique<VariableExpression>("x")};

    statement.execute(first);
    statement.execute(second);
    first.set_variable("x", -1);
    statement.execute(first);

    EXPECT_EQ(first_output.str(), "3\n-1\n");
    EXPECT_EQ(second_output.str(), "7\n");
}

TEST(PrintStatementTest, DoesNotPrintWhenExpressionFails) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    const PrintStatement unknown{std::make_unique<VariableExpression>("missing")};
    const PrintStatement division{
        std::make_unique<BinaryExpression>(BinaryOperator::Div,
            std::make_unique<NumberExpression>(1),
            std::make_unique<NumberExpression>(0))};

    EXPECT_THROW(unknown.execute(context), std::runtime_error);
    EXPECT_TRUE(output.str().empty());
    EXPECT_THROW(division.execute(context), std::domain_error);
    EXPECT_TRUE(output.str().empty());
}

TEST(PrintStatementTest, RejectsNullExpression) {
    EXPECT_THROW((PrintStatement{nullptr}), std::invalid_argument);
}

TEST(PrintStatementTest, ReportsOutputStreamFailure) {
    std::istringstream input;
    std::ostringstream output;
    output.setstate(std::ios::badbit);
    RuntimeContext context{input, output};
    const PrintStatement statement{std::make_unique<NumberExpression>(42)};

    EXPECT_THROW(statement.execute(context), std::runtime_error);
}

class LimitedOutputBuffer : public std::streambuf {
public:
    explicit LimitedOutputBuffer(std::size_t capacity) : remaining_(capacity) {}

protected:
    int_type overflow(int_type character) override {
        if (traits_type::eq_int_type(character, traits_type::eof())) {
            return traits_type::not_eof(character);
        }
        if (remaining_ == 0) {
            return traits_type::eof();
        }
        --remaining_;
        return character;
    }

private:
    std::size_t remaining_;
};

TEST(PrintStatementTest, ReportsFailureWhileWritingNumber) {
    std::istringstream input;
    LimitedOutputBuffer buffer{0};
    std::ostream output{&buffer};
    RuntimeContext context{input, output};
    const PrintStatement statement{std::make_unique<NumberExpression>(42)};

    EXPECT_THROW(statement.execute(context), std::runtime_error);
    EXPECT_TRUE(output.fail());
}

TEST(PrintStatementTest, ReportsFailureWhileWritingNewline) {
    std::istringstream input;
    LimitedOutputBuffer buffer{2};
    std::ostream output{&buffer};
    RuntimeContext context{input, output};
    const PrintStatement statement{std::make_unique<NumberExpression>(42)};

    EXPECT_THROW(statement.execute(context), std::runtime_error);
    EXPECT_TRUE(output.fail());
}

TEST(PrintStatementTest, DestroysOwnedExpressionThroughBaseStatementPointer) {
    int destruction_count = 0;
    {
        const std::unique_ptr<Statement> statement = std::make_unique<PrintStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count));
        EXPECT_EQ(destruction_count, 0);
    }

    EXPECT_EQ(destruction_count, 1);
}

TEST(BlockStatementTest, ExecutesEmptyBlockWithoutRemovingGlobalScope) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    context.set_variable("global", 42);
    const BlockStatement block{std::vector<std::unique_ptr<Statement>>{}};
    const Statement& statement = block;

    EXPECT_NO_THROW(statement.execute(context));
    EXPECT_EQ(context.get_variable("global"), 42);
    EXPECT_TRUE(output.str().empty());
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(BlockStatementTest, ExecutesStatementsInOrderAndRemovesLocalVariables) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    std::vector<std::unique_ptr<Statement>> statements;
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(3))));
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<BinaryExpression>(BinaryOperator::Add,
                std::make_unique<VariableExpression>("x"),
                std::make_unique<NumberExpression>(2)))));
    statements.push_back(std::make_unique<PrintStatement>(std::make_unique<VariableExpression>("x")));
    const BlockStatement block{std::move(statements)};

    block.execute(context);

    EXPECT_EQ(output.str(), "5\n");
    EXPECT_THROW(context.get_variable("x"), std::runtime_error);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(BlockStatementTest, PreservesUpdatesToExistingOuterVariables) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    context.set_variable("x", 1);
    std::vector<std::unique_ptr<Statement>> statements;
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(2))));
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("local", std::make_unique<NumberExpression>(3))));
    const BlockStatement block{std::move(statements)};

    block.execute(context);

    EXPECT_EQ(context.get_variable("x"), 2);
    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
}

TEST(BlockStatementTest, NestedBlockCanReadAndUpdateEnclosingBlockVariable) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    std::vector<std::unique_ptr<Statement>> inner;
    inner.push_back(std::make_unique<PrintStatement>(std::make_unique<VariableExpression>("x")));
    inner.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(4))));
    inner.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("inner", std::make_unique<NumberExpression>(3))));

    std::vector<std::unique_ptr<Statement>> outer;
    outer.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(2))));
    outer.push_back(std::make_unique<BlockStatement>(std::move(inner)));
    outer.push_back(std::make_unique<PrintStatement>(std::make_unique<VariableExpression>("x")));
    const BlockStatement block{std::move(outer)};

    block.execute(context);

    EXPECT_EQ(output.str(), "2\n4\n");
    EXPECT_THROW(context.get_variable("x"), std::runtime_error);
    EXPECT_THROW(context.get_variable("inner"), std::runtime_error);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(BlockStatementTest, ErrorStopsExecutionAndRemovesScopeWithoutUndoingEarlierEffects) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    context.set_variable("x", 1);
    std::vector<std::unique_ptr<Statement>> statements;
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("local", std::make_unique<NumberExpression>(2))));
    statements.push_back(std::make_unique<PrintStatement>(std::make_unique<VariableExpression>("local")));
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(3))));
    statements.push_back(std::make_unique<PrintStatement>(
        std::make_unique<BinaryExpression>(BinaryOperator::Div,
            std::make_unique<NumberExpression>(1), std::make_unique<NumberExpression>(0))));
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(99))));
    const BlockStatement block{std::move(statements)};

    EXPECT_THROW(block.execute(context), std::domain_error);
    EXPECT_EQ(context.get_variable("x"), 3);
    EXPECT_EQ(output.str(), "2\n");
    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(BlockStatementTest, UnwindsAllNestedBlockScopesOnError) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    context.set_variable("global", 42);
    std::vector<std::unique_ptr<Statement>> inner;
    inner.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("inner", std::make_unique<NumberExpression>(2))));
    inner.push_back(std::make_unique<ExpressionStatement>(std::make_unique<VariableExpression>("missing")));

    std::vector<std::unique_ptr<Statement>> outer;
    outer.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("outer", std::make_unique<NumberExpression>(1))));
    outer.push_back(std::make_unique<BlockStatement>(std::move(inner)));
    const BlockStatement block{std::move(outer)};

    EXPECT_THROW(block.execute(context), std::runtime_error);
    EXPECT_THROW(context.get_variable("outer"), std::runtime_error);
    EXPECT_THROW(context.get_variable("inner"), std::runtime_error);
    EXPECT_EQ(context.get_variable("global"), 42);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(BlockStatementTest, CreatesFreshLocalScopeOnEveryExecution) {
    std::istringstream input{"10 -3 99"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    std::vector<std::unique_ptr<Statement>> statements;
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("local", std::make_unique<InputExpression>())));
    statements.push_back(std::make_unique<PrintStatement>(std::make_unique<VariableExpression>("local")));
    const BlockStatement block{std::move(statements)};

    block.execute(context);
    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
    block.execute(context);
    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
    EXPECT_EQ(output.str(), "10\n-3\n");
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(BlockStatementTest, RejectsNullAtAnyPositionAndReleasesOtherStatements) {
    for (int null_position = 0; null_position < 3; ++null_position) {
        SCOPED_TRACE(null_position);
        int destruction_count = 0;
        std::vector<std::unique_ptr<Statement>> statements;
        for (int i = 0; i < 3; ++i) {
            if (i == null_position) {
                statements.push_back(nullptr);
            } else {
                statements.push_back(std::make_unique<ExpressionStatement>(
                    std::make_unique<DestructionProbeExpression>(destruction_count)));
            }
        }

        EXPECT_THROW((BlockStatement{std::move(statements)}), std::invalid_argument);
        EXPECT_EQ(destruction_count, 2);
    }
}

TEST(BlockStatementTest, DestroysOwnedStatementsThroughBasePointer) {
    int destruction_count = 0;
    {
        std::vector<std::unique_ptr<Statement>> statements;
        statements.push_back(std::make_unique<ExpressionStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count)));
        statements.push_back(std::make_unique<PrintStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count)));
        const std::unique_ptr<Statement> block = std::make_unique<BlockStatement>(std::move(statements));
        EXPECT_EQ(destruction_count, 0);
    }

    EXPECT_EQ(destruction_count, 2);
}

} // namespace
} // namespace ParaCL
