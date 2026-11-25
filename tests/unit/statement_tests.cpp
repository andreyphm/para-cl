#include "ast.hpp"
#include "runtime_context.hpp"

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

TEST(OutputStatementTest, PrintsIntValuesWithNewlineThroughBaseInterface) {
    for (const int value : {0, 42, -7,
             std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
        SCOPED_TRACE(value);
        std::istringstream input;
        std::ostringstream output;
        RuntimeContext context{input, output};
        const OutputStatement print{std::make_unique<NumberExpression>(value)};
        const Statement& statement = print;

        statement.execute(context);

        EXPECT_EQ(output.str(), std::format("{}\n", value));
    }
}

TEST(OutputStatementTest, EvaluatesInputOnceOnEachExecution) {
    std::istringstream input{"10 -3 99"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    const OutputStatement statement{std::make_unique<InputExpression>()};

    statement.execute(context);
    statement.execute(context);

    EXPECT_EQ(output.str(), "10\n-3\n");
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(OutputStatementTest, PrintsAssignmentResultAndPreservesItsSideEffect) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    const OutputStatement statement{
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<BinaryExpression>(BinaryOperator::Add,
                std::make_unique<NumberExpression>(3),
                std::make_unique<NumberExpression>(2)))};

    statement.execute(context);

    EXPECT_EQ(output.str(), "5\n");
    EXPECT_EQ(context.get_variable("x"), 5);
}

TEST(OutputStatementTest, UsesVariablesAndOutputOfProvidedContext) {
    std::istringstream first_input;
    std::istringstream second_input;
    std::ostringstream first_output;
    std::ostringstream second_output;
    RuntimeContext first{first_input, first_output};
    RuntimeContext second{second_input, second_output};
    first.set_variable("x", 3);
    second.set_variable("x", 7);
    const OutputStatement statement{std::make_unique<VariableExpression>("x")};

    statement.execute(first);
    statement.execute(second);
    first.set_variable("x", -1);
    statement.execute(first);

    EXPECT_EQ(first_output.str(), "3\n-1\n");
    EXPECT_EQ(second_output.str(), "7\n");
}

TEST(OutputStatementTest, DoesNotPrintWhenExpressionFails) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    const OutputStatement unknown{std::make_unique<VariableExpression>("missing")};
    const OutputStatement division{
        std::make_unique<BinaryExpression>(BinaryOperator::Div,
            std::make_unique<NumberExpression>(1),
            std::make_unique<NumberExpression>(0))};

    EXPECT_THROW(unknown.execute(context), std::runtime_error);
    EXPECT_TRUE(output.str().empty());
    EXPECT_THROW(division.execute(context), std::domain_error);
    EXPECT_TRUE(output.str().empty());
}

TEST(OutputStatementTest, RejectsNullExpression) {
    EXPECT_THROW((OutputStatement{nullptr}), std::invalid_argument);
}

TEST(OutputStatementTest, ReportsOutputStreamFailure) {
    std::istringstream input;
    std::ostringstream output;
    output.setstate(std::ios::badbit);
    RuntimeContext context{input, output};
    const OutputStatement statement{std::make_unique<NumberExpression>(42)};

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

TEST(OutputStatementTest, ReportsFailureWhileWritingNumber) {
    std::istringstream input;
    LimitedOutputBuffer buffer{0};
    std::ostream output{&buffer};
    RuntimeContext context{input, output};
    const OutputStatement statement{std::make_unique<NumberExpression>(42)};

    EXPECT_THROW(statement.execute(context), std::runtime_error);
    EXPECT_TRUE(output.fail());
}

TEST(OutputStatementTest, ReportsFailureWhileWritingNewline) {
    std::istringstream input;
    LimitedOutputBuffer buffer{2};
    std::ostream output{&buffer};
    RuntimeContext context{input, output};
    const OutputStatement statement{std::make_unique<NumberExpression>(42)};

    EXPECT_THROW(statement.execute(context), std::runtime_error);
    EXPECT_TRUE(output.fail());
}

TEST(OutputStatementTest, DestroysOwnedExpressionThroughBaseStatementPointer) {
    int destruction_count = 0;
    {
        const std::unique_ptr<Statement> statement = std::make_unique<OutputStatement>(
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
    statements.push_back(std::make_unique<OutputStatement>(std::make_unique<VariableExpression>("x")));
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
    inner.push_back(std::make_unique<OutputStatement>(std::make_unique<VariableExpression>("x")));
    inner.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(4))));
    inner.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("inner", std::make_unique<NumberExpression>(3))));

    std::vector<std::unique_ptr<Statement>> outer;
    outer.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(2))));
    outer.push_back(std::make_unique<BlockStatement>(std::move(inner)));
    outer.push_back(std::make_unique<OutputStatement>(std::make_unique<VariableExpression>("x")));
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
    statements.push_back(std::make_unique<OutputStatement>(std::make_unique<VariableExpression>("local")));
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(3))));
    statements.push_back(std::make_unique<OutputStatement>(
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
    statements.push_back(std::make_unique<OutputStatement>(std::make_unique<VariableExpression>("local")));
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
        statements.push_back(std::make_unique<OutputStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count)));
        const std::unique_ptr<Statement> block = std::make_unique<BlockStatement>(std::move(statements));
        EXPECT_EQ(destruction_count, 0);
    }

    EXPECT_EQ(destruction_count, 2);
}

TEST(IfStatementTest, ExecutesBodyForNonzeroConditionThroughBaseInterface) {
    for (const int value : {1, -2}) {
        SCOPED_TRACE(value);
        std::istringstream input;
        std::ostringstream output;
        RuntimeContext context{input, output};
        const IfStatement conditional{std::make_unique<NumberExpression>(value),
            std::make_unique<OutputStatement>(std::make_unique<NumberExpression>(42))};
        const Statement& statement = conditional;

        statement.execute(context);

        EXPECT_EQ(output.str(), "42\n");
    }
}

TEST(IfStatementTest, ZeroConditionSkipsBodyAndItsErrors) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    const IfStatement statement{std::make_unique<NumberExpression>(0),
        std::make_unique<OutputStatement>(
            std::make_unique<BinaryExpression>(BinaryOperator::Div,
                std::make_unique<NumberExpression>(1), std::make_unique<NumberExpression>(0)))};

    EXPECT_NO_THROW(statement.execute(context));
    EXPECT_TRUE(output.str().empty());
}

TEST(IfStatementTest, ZeroConditionDoesNotConsumeBodyInput) {
    std::istringstream input{"99"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    const IfStatement statement{std::make_unique<NumberExpression>(0),
        std::make_unique<ExpressionStatement>(std::make_unique<InputExpression>())};

    statement.execute(context);

    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(IfStatementTest, EvaluatesConditionOnceAndPreservesItsSideEffects) {
    std::istringstream input{"1 0 99"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    const IfStatement statement{
        std::make_unique<AssignmentExpression>("condition", std::make_unique<InputExpression>()),
        std::make_unique<OutputStatement>(std::make_unique<VariableExpression>("condition"))};

    statement.execute(context);
    EXPECT_EQ(output.str(), "1\n");
    EXPECT_EQ(context.get_variable("condition"), 1);
    statement.execute(context);
    EXPECT_EQ(output.str(), "1\n");
    EXPECT_EQ(context.get_variable("condition"), 0);
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(IfStatementTest, ConditionErrorPreventsBodyExecution) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    const IfStatement statement{std::make_unique<VariableExpression>("missing"),
        std::make_unique<OutputStatement>(std::make_unique<NumberExpression>(42))};

    EXPECT_THROW(statement.execute(context), std::runtime_error);
    EXPECT_TRUE(output.str().empty());
}

TEST(IfStatementTest, ExecutesOnlyTheSelectedBranchAndEvaluatesTheConditionOnce) {
    std::istringstream input{"0 -2 99"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    const IfStatement statement{std::make_unique<InputExpression>(),
        std::make_unique<OutputStatement>(std::make_unique<NumberExpression>(1)),
        std::make_unique<OutputStatement>(std::make_unique<NumberExpression>(2))};

    statement.execute(context);
    statement.execute(context);

    EXPECT_EQ(output.str(), "2\n1\n");
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(IfStatementTest, PropagatesBodyErrorAndUnwindsBlockScope) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    context.set_variable("global", 42);
    std::vector<std::unique_ptr<Statement>> body;
    body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("local", std::make_unique<NumberExpression>(2))));
    body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<BinaryExpression>(BinaryOperator::Div,
            std::make_unique<NumberExpression>(1), std::make_unique<NumberExpression>(0))));
    const IfStatement statement{std::make_unique<NumberExpression>(1),
        std::make_unique<BlockStatement>(std::move(body))};

    EXPECT_THROW(statement.execute(context), std::domain_error);
    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
    EXPECT_EQ(context.get_variable("global"), 42);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(IfStatementTest, DoesNotCreateScopeForSingleStatementBody) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    const IfStatement statement{std::make_unique<NumberExpression>(1),
        std::make_unique<ExpressionStatement>(
            std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(42)))};

    statement.execute(context);

    EXPECT_EQ(context.get_variable("x"), 42);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(IfStatementTest, BlockBodyRemovesLocalsAndUpdatesOuterVariables) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    context.set_variable("x", 1);
    std::vector<std::unique_ptr<Statement>> body;
    body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("local", std::make_unique<NumberExpression>(2))));
    body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(3))));
    const IfStatement statement{std::make_unique<NumberExpression>(1),
        std::make_unique<BlockStatement>(std::move(body))};

    statement.execute(context);

    EXPECT_EQ(context.get_variable("x"), 3);
    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(IfStatementTest, RejectsNullConditionOrBodyAndReleasesOtherNode) {
    int destruction_count = 0;
    EXPECT_THROW((IfStatement{nullptr,
        std::make_unique<ExpressionStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count))}),
        std::invalid_argument);
    EXPECT_EQ(destruction_count, 1);
    EXPECT_THROW((IfStatement{
        std::make_unique<DestructionProbeExpression>(destruction_count), nullptr}),
        std::invalid_argument);
    EXPECT_EQ(destruction_count, 2);
    EXPECT_THROW((IfStatement{nullptr, nullptr}), std::invalid_argument);
}

TEST(IfStatementTest, ReleasesConditionAndBodyThroughBasePointer) {
    int destruction_count = 0;
    {
        const std::unique_ptr<Statement> statement = std::make_unique<IfStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count),
            std::make_unique<ExpressionStatement>(
                std::make_unique<DestructionProbeExpression>(destruction_count)));
        EXPECT_EQ(destruction_count, 0);
    }

    EXPECT_EQ(destruction_count, 2);
}

TEST(WhileStatementTest, ZeroConditionSkipsBodyAndItsErrors) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    const WhileStatement statement{std::make_unique<NumberExpression>(0),
        std::make_unique<OutputStatement>(
            std::make_unique<BinaryExpression>(BinaryOperator::Div,
                std::make_unique<NumberExpression>(1), std::make_unique<NumberExpression>(0)))};

    EXPECT_NO_THROW(statement.execute(context));
    EXPECT_TRUE(output.str().empty());
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(WhileStatementTest, ReevaluatesConditionAfterEachBodyExecution) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    context.set_variable("x", 3);
    std::vector<std::unique_ptr<Statement>> body;
    body.push_back(std::make_unique<OutputStatement>(std::make_unique<VariableExpression>("x")));
    body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<BinaryExpression>(BinaryOperator::Sub,
                std::make_unique<VariableExpression>("x"), std::make_unique<NumberExpression>(1)))));
    const WhileStatement loop{
        std::make_unique<BinaryExpression>(BinaryOperator::Greater,
            std::make_unique<VariableExpression>("x"), std::make_unique<NumberExpression>(0)),
        std::make_unique<BlockStatement>(std::move(body))};
    const Statement& statement = loop;

    statement.execute(context);

    EXPECT_EQ(context.get_variable("x"), 0);
    EXPECT_EQ(output.str(), "3\n2\n1\n");
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(WhileStatementTest, EvaluatesConditionOncePerIterationAndOnceToStop) {
    std::istringstream input{"2 -1 0 99"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    const WhileStatement statement{std::make_unique<InputExpression>(),
        std::make_unique<OutputStatement>(std::make_unique<NumberExpression>(42))};

    statement.execute(context);

    EXPECT_EQ(output.str(), "42\n42\n");
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(WhileStatementTest, DoesNotCreateScopeForConditionOrSingleStatementBody) {
    std::istringstream input{"2 -1 0"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    const WhileStatement statement{
        std::make_unique<AssignmentExpression>("condition", std::make_unique<InputExpression>()),
        std::make_unique<ExpressionStatement>(
            std::make_unique<AssignmentExpression>("last",
                std::make_unique<VariableExpression>("condition")))};

    statement.execute(context);

    EXPECT_EQ(context.get_variable("condition"), 0);
    EXPECT_EQ(context.get_variable("last"), -1);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(WhileStatementTest, ConditionErrorPreventsBodyExecution) {
    std::istringstream input{"99"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    const WhileStatement statement{std::make_unique<VariableExpression>("missing"),
        std::make_unique<ExpressionStatement>(std::make_unique<InputExpression>())};

    EXPECT_THROW(statement.execute(context), std::runtime_error);
    EXPECT_EQ(InputExpression{}.evaluate(context), 99);
}

TEST(WhileStatementTest, BodyErrorStopsLoopAndUnwindsIterationScope) {
    std::istringstream input{"1 1 0 99"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    context.set_variable("global", 42);
    std::vector<std::unique_ptr<Statement>> body;
    body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("local", std::make_unique<NumberExpression>(2))));
    body.push_back(std::make_unique<OutputStatement>(
        std::make_unique<BinaryExpression>(BinaryOperator::Div,
            std::make_unique<NumberExpression>(1), std::make_unique<NumberExpression>(0))));
    body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("global", std::make_unique<NumberExpression>(99))));
    const WhileStatement statement{std::make_unique<InputExpression>(),
        std::make_unique<BlockStatement>(std::move(body))};

    EXPECT_THROW(statement.execute(context), std::domain_error);
    EXPECT_EQ(InputExpression{}.evaluate(context), 1);
    EXPECT_EQ(context.get_variable("global"), 42);
    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
    EXPECT_TRUE(output.str().empty());
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(WhileStatementTest, NestedLoopsUpdateOuterStateAndRemoveLocalCounters) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    context.set_variable("x", 2);
    context.set_variable("count", 0);
    std::vector<std::unique_ptr<Statement>> inner_body;
    inner_body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("count",
            std::make_unique<BinaryExpression>(BinaryOperator::Add,
                std::make_unique<VariableExpression>("count"), std::make_unique<NumberExpression>(1)))));
    inner_body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("y",
            std::make_unique<BinaryExpression>(BinaryOperator::Sub,
                std::make_unique<VariableExpression>("y"), std::make_unique<NumberExpression>(1)))));

    std::vector<std::unique_ptr<Statement>> outer_body;
    outer_body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("y", std::make_unique<NumberExpression>(3))));
    outer_body.push_back(std::make_unique<WhileStatement>(
        std::make_unique<BinaryExpression>(BinaryOperator::Greater,
            std::make_unique<VariableExpression>("y"), std::make_unique<NumberExpression>(0)),
        std::make_unique<BlockStatement>(std::move(inner_body))));
    outer_body.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x",
            std::make_unique<BinaryExpression>(BinaryOperator::Sub,
                std::make_unique<VariableExpression>("x"), std::make_unique<NumberExpression>(1)))));
    const WhileStatement statement{
        std::make_unique<BinaryExpression>(BinaryOperator::Greater,
            std::make_unique<VariableExpression>("x"), std::make_unique<NumberExpression>(0)),
        std::make_unique<BlockStatement>(std::move(outer_body))};

    statement.execute(context);

    EXPECT_EQ(context.get_variable("count"), 6);
    EXPECT_EQ(context.get_variable("x"), 0);
    EXPECT_THROW(context.get_variable("y"), std::runtime_error);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(WhileStatementTest, ExecutesFibonacciExampleFromTask) {
    for (const auto& [iterations, expected] : {
             std::pair{0, 1}, std::pair{1, 1}, std::pair{2, 2},
             std::pair{10, 89}, std::pair{20, 10946}}) {
        SCOPED_TRACE(iterations);
        std::istringstream input{std::format("{}", iterations)};
        std::ostringstream output;
        RuntimeContext context{input, output};
        const ExpressionStatement initialize_first{
            std::make_unique<AssignmentExpression>("fst", std::make_unique<NumberExpression>(0))};
        const ExpressionStatement initialize_second{
            std::make_unique<AssignmentExpression>("snd", std::make_unique<NumberExpression>(1))};
        const ExpressionStatement initialize_iterations{
            std::make_unique<AssignmentExpression>("iters", std::make_unique<InputExpression>())};

        std::vector<std::unique_ptr<Statement>> body;
        body.push_back(std::make_unique<ExpressionStatement>(
            std::make_unique<AssignmentExpression>("tmp", std::make_unique<VariableExpression>("fst"))));
        body.push_back(std::make_unique<ExpressionStatement>(
            std::make_unique<AssignmentExpression>("fst", std::make_unique<VariableExpression>("snd"))));
        body.push_back(std::make_unique<ExpressionStatement>(
            std::make_unique<AssignmentExpression>("snd",
                std::make_unique<BinaryExpression>(BinaryOperator::Add,
                    std::make_unique<VariableExpression>("snd"), std::make_unique<VariableExpression>("tmp")))));
        body.push_back(std::make_unique<ExpressionStatement>(
            std::make_unique<AssignmentExpression>("iters",
                std::make_unique<BinaryExpression>(BinaryOperator::Sub,
                    std::make_unique<VariableExpression>("iters"), std::make_unique<NumberExpression>(1)))));
        const WhileStatement loop{
            std::make_unique<BinaryExpression>(BinaryOperator::Greater,
                std::make_unique<VariableExpression>("iters"), std::make_unique<NumberExpression>(0)),
            std::make_unique<BlockStatement>(std::move(body))};
        const OutputStatement print{std::make_unique<VariableExpression>("snd")};

        initialize_first.execute(context);
        initialize_second.execute(context);
        initialize_iterations.execute(context);
        loop.execute(context);
        print.execute(context);

        EXPECT_EQ(output.str(), std::format("{}\n", expected));
        EXPECT_EQ(context.get_variable("iters"), 0);
        EXPECT_THROW(context.get_variable("tmp"), std::runtime_error);
        EXPECT_THROW(context.leave_scope(), std::logic_error);
    }
}

TEST(WhileStatementTest, RejectsNullConditionOrBodyAndReleasesOtherNode) {
    int destruction_count = 0;
    EXPECT_THROW((WhileStatement{nullptr,
        std::make_unique<ExpressionStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count))}),
        std::invalid_argument);
    EXPECT_EQ(destruction_count, 1);
    EXPECT_THROW((WhileStatement{
        std::make_unique<DestructionProbeExpression>(destruction_count), nullptr}),
        std::invalid_argument);
    EXPECT_EQ(destruction_count, 2);
    EXPECT_THROW((WhileStatement{nullptr, nullptr}), std::invalid_argument);
}

TEST(WhileStatementTest, ReleasesConditionAndBodyThroughBasePointer) {
    int destruction_count = 0;
    {
        const std::unique_ptr<Statement> statement = std::make_unique<WhileStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count),
            std::make_unique<ExpressionStatement>(
                std::make_unique<DestructionProbeExpression>(destruction_count)));
        EXPECT_EQ(destruction_count, 0);
    }

    EXPECT_EQ(destruction_count, 2);
}

TEST(ProgramTest, AllowsEmptyProgram) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    const Program program{std::vector<std::unique_ptr<Statement>>{}};

    EXPECT_NO_THROW(program.execute(context));
    EXPECT_TRUE(output.str().empty());
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(ProgramTest, ExecutesInOrderAndKeepsVariablesInGlobalScope) {
    std::istringstream input{"17 23"};
    std::ostringstream output;
    RuntimeContext context{input, output};
    std::vector<std::unique_ptr<Statement>> statements;
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<InputExpression>())));
    statements.push_back(std::make_unique<OutputStatement>(
        std::make_unique<VariableExpression>("x")));
    const Program program{std::move(statements)};

    program.execute(context);
    EXPECT_EQ(context.get_variable("x"), 17);
    program.execute(context);
    EXPECT_EQ(context.get_variable("x"), 23);
    EXPECT_EQ(output.str(), "17\n23\n");
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(ProgramTest, StopsOnErrorWithoutRollingBackEarlierStatements) {
    std::istringstream input;
    std::ostringstream output;
    RuntimeContext context{input, output};
    std::vector<std::unique_ptr<Statement>> statements;
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<AssignmentExpression>("x", std::make_unique<NumberExpression>(7))));
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<VariableExpression>("missing")));
    statements.push_back(std::make_unique<OutputStatement>(
        std::make_unique<NumberExpression>(99)));
    const Program program{std::move(statements)};

    EXPECT_THROW(program.execute(context), std::runtime_error);
    EXPECT_EQ(context.get_variable("x"), 7);
    EXPECT_TRUE(output.str().empty());
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(ProgramTest, RejectsNullStatementAndReleasesOwnedNodes) {
    int destruction_count = 0;
    std::vector<std::unique_ptr<Statement>> statements;
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<DestructionProbeExpression>(destruction_count)));
    statements.push_back(nullptr);
    statements.push_back(std::make_unique<ExpressionStatement>(
        std::make_unique<DestructionProbeExpression>(destruction_count)));

    EXPECT_THROW((Program{std::move(statements)}), std::invalid_argument);
    EXPECT_EQ(destruction_count, 2);
}

TEST(ProgramTest, ReleasesOwnedStatementsOnDestruction) {
    int destruction_count = 0;
    {
        std::vector<std::unique_ptr<Statement>> statements;
        statements.push_back(std::make_unique<ExpressionStatement>(
            std::make_unique<DestructionProbeExpression>(destruction_count)));
        const Program program{std::move(statements)};
        EXPECT_EQ(destruction_count, 0);
    }
    EXPECT_EQ(destruction_count, 1);
}

} // namespace
} // namespace ParaCL
