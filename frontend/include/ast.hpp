#pragma once

#include <memory>
#include <utility>
#include <vector>
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
    BinaryExpression(BinaryOperator operation, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);
    int evaluate(RuntimeContext& context) const override;
private:
    BinaryOperator operation_;
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(UnaryOperator operation, std::unique_ptr<Expression> operand);
    int evaluate(RuntimeContext& context) const override;
private:
    UnaryOperator operation_;
    std::unique_ptr<Expression> operand_;
};

class VariableExpression : public Expression {
public:
    explicit VariableExpression(std::string name) : name_(std::move(name)) {}
    int evaluate(RuntimeContext& context) const override { return context.get_variable(name_); }
private:
    std::string name_;
};

class InputExpression : public Expression {
public:
    int evaluate(RuntimeContext& context) const override;
};

class AssignmentExpression : public Expression {
public:
    AssignmentExpression(std::string name, std::unique_ptr<Expression> expression);
    int evaluate(RuntimeContext& context) const override;
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
    explicit ExpressionStatement(std::unique_ptr<Expression> expression);
    void execute(RuntimeContext& context) const override { expression_->evaluate(context); }
private:
    std::unique_ptr<Expression> expression_;
};

class OutputStatement : public Statement {
public:
    explicit OutputStatement(std::unique_ptr<Expression> expression);
    void execute(RuntimeContext& context) const override;
private:
    std::unique_ptr<Expression> expression_;
};

class BlockStatement : public Statement {
public:
    explicit BlockStatement(std::vector<std::unique_ptr<Statement>> statements);
    void execute(RuntimeContext& context) const override;
private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

class IfStatement : public Statement {
public:
    IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body);
    void execute(RuntimeContext& context) const override;
private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> body_;
};

class WhileStatement : public Statement {
public:
    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body);
    void execute(RuntimeContext& context) const override;
private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> body_;
};


class Program {
public:
    explicit Program(std::vector<std::unique_ptr<Statement>> statements);
    void execute(RuntimeContext& context) const;
private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

} // namespace ParaCL
