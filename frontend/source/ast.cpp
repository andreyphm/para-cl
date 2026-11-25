#include "ast.hpp"

#include <stdexcept>
#include <utility>

#include "runtime_context.hpp"

namespace ParaCL {

void Expression::accept(AstVisitor&) const {
    throw std::runtime_error("Visiting this expression is not implemented");
}

void NumberExpression::accept(AstVisitor& visitor) const {
    visitor.visit_number(value_);
}

BinaryExpression::BinaryExpression(BinaryOperator operation, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
    : operation_(operation), left_(std::move(left)), right_(std::move(right)) {
    if (!left_ || !right_) {
        throw std::invalid_argument("Binary expression operands must not be null");
    }
}

int BinaryExpression::evaluate(RuntimeContext& context) const {
    const int left_value = left_->evaluate(context);

    switch (operation_) {
        case BinaryOperator::Add: return left_value + right_->evaluate(context);
        case BinaryOperator::Sub: return left_value - right_->evaluate(context);
        case BinaryOperator::Mul: return left_value * right_->evaluate(context);
        case BinaryOperator::Div: {
            const int right_value = right_->evaluate(context);
            if (right_value == 0) {
                throw std::domain_error("Division by zero");
            }
            return left_value / right_value;
        }

        case BinaryOperator::Equal:        return left_value == right_->evaluate(context);
        case BinaryOperator::NotEqual:     return left_value != right_->evaluate(context);
        case BinaryOperator::Less:         return left_value <  right_->evaluate(context);
        case BinaryOperator::LessEqual:    return left_value <= right_->evaluate(context);
        case BinaryOperator::Greater:      return left_value >  right_->evaluate(context);
        case BinaryOperator::GreaterEqual: return left_value >= right_->evaluate(context);
        case BinaryOperator::LogicalAnd:   return left_value && right_->evaluate(context);
        case BinaryOperator::LogicalOr:    return left_value || right_->evaluate(context);
    }

    throw std::logic_error("Unknown binary operator");
}

UnaryExpression::UnaryExpression(UnaryOperator operation, std::unique_ptr<Expression> operand)
    : operation_(operation), operand_(std::move(operand)) {
    if (!operand_) {
        throw std::invalid_argument("Operand must not be null");
    }
}

int UnaryExpression::evaluate(RuntimeContext& context) const {
    switch (operation_) {
        case UnaryOperator::Plus:  return operand_->evaluate(context);
        case UnaryOperator::Minus: return -operand_->evaluate(context);
        case UnaryOperator::LogicalNot: return !operand_->evaluate(context);
    }

    throw std::logic_error("Unknown unary operator");
}

int VariableExpression::evaluate(RuntimeContext& context) const {
    return context.get_variable(name_);
}

int InputExpression::evaluate(RuntimeContext& context) const {
    int value{};
    if (!(context.input >> value)) {
        throw std::runtime_error("Failed to read int value");
    }
    return value;
}

AssignmentExpression::AssignmentExpression(std::string name, std::unique_ptr<Expression> expression)
    : name_(std::move(name)), expression_(std::move(expression)) {
    if (!expression_) {
        throw std::invalid_argument("Assignment expression must not be null");
    }
}

int AssignmentExpression::evaluate(RuntimeContext& context) const {
    const int value = expression_->evaluate(context);
    context.set_variable(name_, value);
    return value;
}

ExpressionStatement::ExpressionStatement(std::unique_ptr<Expression> expression) : expression_(std::move(expression)) {
    if (!expression_) {
        throw std::invalid_argument("Expression must not be null");
    }
}

OutputStatement::OutputStatement(std::unique_ptr<Expression> expression) : expression_(std::move(expression)) {
    if (!expression_) {
        throw std::invalid_argument("Expression must not be null");
    }
}

void OutputStatement::execute(RuntimeContext& context) const {
    context.output << expression_->evaluate(context) << '\n';
    if (!context.output) {
        throw std::runtime_error("Failed to output int value");
    }
}

BlockStatement::BlockStatement(std::vector<std::unique_ptr<Statement>> statements) : statements_(std::move(statements)) {
    for (const auto& statement : statements_) {
        if (!statement) {
            throw std::invalid_argument("Statement must not be null");
        }
    }
}

void BlockStatement::execute(RuntimeContext& context) const {
    ScopeOwner scope{context};
    for (const auto& statement : statements_) {
        statement->execute(context);
    }
}

void BlockStatement::accept(AstVisitor& visitor) const {
    visitor.enter_block();
    for (const auto& statement : statements_) {
        statement->accept(visitor);
    }
    visitor.leave_block();
}

IfStatement::IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> then_branch,
                         std::unique_ptr<Statement> else_branch)
    : condition_(std::move(condition)), then_branch_(std::move(then_branch)),
      else_branch_(std::move(else_branch)) {
    if (!condition_) {
        throw std::invalid_argument("'if' condition must not be null");
    }
    if (!then_branch_) {
        throw std::invalid_argument("'if' body must not be null");
    }
}

void IfStatement::execute(RuntimeContext& context) const {
    if (condition_->evaluate(context)) {
        then_branch_->execute(context);
    } else if (else_branch_) {
        else_branch_->execute(context);
    }
}

WhileStatement::WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body)
    : condition_(std::move(condition)), body_(std::move(body)) {
    if (!condition_) {
        throw std::invalid_argument("'while' condition must not be null");
    }
    if (!body_) {
        throw std::invalid_argument("'while' body must not be null");
    }
}

void WhileStatement::execute(RuntimeContext& context) const {
    while (condition_->evaluate(context)) {
        body_->execute(context);
    }
}

Program::Program(std::vector<std::unique_ptr<Statement>> statements) : statements_(std::move(statements)) {
    for (const auto& statement : statements_) {
        if (!statement) {
            throw std::invalid_argument("Statement must not be null");
        }
    }
}

void Program::execute(RuntimeContext& context) const {
    for (const auto& statement : statements_) {
        statement->execute(context);
    }
}

void Program::accept(AstVisitor& visitor) const {
    for (const auto& statement : statements_) {
        statement->accept(visitor);
    }
}

} // namespace ParaCL
