#pragma once

#include <memory>
#include <utility>
#include <stdexcept>

namespace ParaCL {

enum class BinaryOperator {
    Add,
    Sub,
    Mul,
    Div
};

enum class UnaryOperator {
    Plus,
    Minus
};

class Expression {
public:
    virtual int evaluate() const = 0;
    virtual ~Expression() = default;
};

class NumberExpression : public Expression {
public:
    explicit NumberExpression(int value) : value_(value) {}
    int evaluate() const override { return value_; }

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

    int evaluate() const override {
        const int left_value = left_->evaluate();
        const int right_value = right_->evaluate();

        switch (operation_) {
            case BinaryOperator::Add: return left_value + right_value;
            case BinaryOperator::Sub: return left_value - right_value;
            case BinaryOperator::Mul: return left_value * right_value;
            case BinaryOperator::Div:
                if (right_value == 0) {
                    throw std::domain_error("Division by zero");
                }
                return left_value / right_value;
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

    int evaluate() const override {
        switch (operation_) {
            case UnaryOperator::Plus:  return operand_->evaluate();
            case UnaryOperator::Minus: return -operand_->evaluate();
        }

        throw std::logic_error("Unknown unary operator");
    }

private:
    UnaryOperator operation_;
    std::unique_ptr<Expression> operand_;
};

} // namespace ParaCL
