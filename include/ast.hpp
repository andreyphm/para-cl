#pragma once

#include <memory>
#include <utility>
#include <vector>
#include <stdexcept>
#include <string>

#include "runtime_context.hpp"

namespace ParaCL {

enum class BinaryOperator {
    Add,
    Sub,
    Mul,
    Div,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    LogicalAnd,
    LogicalOr
};

enum class UnaryOperator {
    Plus,
    Minus
};

class Expression {
public:
    virtual int evaluate(RuntimeContext&) const = 0;
    virtual ~Expression() = default;
};

class NumberExpression : public Expression {
public:
    explicit NumberExpression(int value) : value_(value) {}
    int evaluate(RuntimeContext&) const override { return value_; }

private:
    int value_;
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(BinaryOperator operation, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right)
                    : operation_(operation), left_(std::move(left)), right_(std::move(right)) {
        if (!left_ || !right_) {
            throw std::invalid_argument("Binary expression operands must not be null");
        }
    }

    int evaluate(RuntimeContext& context) const override {
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

private:
    BinaryOperator operation_;
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(UnaryOperator operation, std::unique_ptr<Expression> operand)
                   : operation_(operation), operand_(std::move(operand)) {
        if (!operand_) {
            throw std::invalid_argument("Operand must not be null");
        }
    }

    int evaluate(RuntimeContext& context) const override {
        switch (operation_) {
            case UnaryOperator::Plus:  return operand_->evaluate(context);
            case UnaryOperator::Minus: return -operand_->evaluate(context);
        }

        throw std::logic_error("Unknown unary operator");
    }

private:
    UnaryOperator operation_;
    std::unique_ptr<Expression> operand_;
};

class VariableExpression : public Expression {
public:
    explicit VariableExpression(std::string name) : name_(std::move(name)) {}

    int evaluate(RuntimeContext& context) const override {
        return context.get_variable(name_);
    }

private:
    std::string name_;
};

class InputExpression : public Expression {
public:
    int evaluate(RuntimeContext& context) const override {
        int value{};
        if (!(context.input >> value)) {
            throw std::runtime_error("Failed to read int value");
        }
        return value;
    }
};

class AssignmentExpression : public Expression {
public:
    AssignmentExpression(std::string name, std::unique_ptr<Expression> expression)
                        : name_(std::move(name)), expression_(std::move(expression)) {
        if (!expression_) {
            throw std::invalid_argument("Assignment expression must not be null");
        }
    }

    int evaluate(RuntimeContext& context) const override {
        const int value = expression_->evaluate(context);
        context.set_variable(name_, value);
        return value;
    }

private:
    std::string name_;
    std::unique_ptr<Expression> expression_;
};

class Statement {
public:
    virtual void execute(RuntimeContext&) const = 0;
    virtual ~Statement() = default;
};

class ExpressionStatement : public Statement {
public:
    explicit ExpressionStatement(std::unique_ptr<Expression> expression) : expression_(std::move(expression)) {
        if (!expression_) {
            throw std::invalid_argument("Expression must not be null");
        }
    }

    void execute(RuntimeContext& context) const override {
        expression_->evaluate(context);
    }

private:
    std::unique_ptr<Expression> expression_;
};

class PrintStatement : public Statement {
public:
    explicit PrintStatement(std::unique_ptr<Expression> expression) : expression_(std::move(expression)) {
        if (!expression_) {
            throw std::invalid_argument("Expression must not be null");
        }
    }

    void execute(RuntimeContext& context) const override {
        context.output << expression_->evaluate(context) << '\n';
        if (!context.output) {
            throw std::runtime_error("Failed to output int value");
        }
    }

private:
    std::unique_ptr<Expression> expression_;
};

class BlockStatement : public Statement {
public:
    explicit BlockStatement(std::vector<std::unique_ptr<Statement>> statements) : statements_(std::move(statements)) {
        for (const auto& statement : statements_) {
            if (!statement) {
                throw std::invalid_argument("Statement must not be null");
            }
        }
    }

    void execute(RuntimeContext& context) const override {
        ScopeOwner scope{context};
        for (const auto& statement : statements_) {
            statement->execute(context);
        }
    }

private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

} // namespace ParaCL
