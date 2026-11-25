#pragma once

#include <memory>
#include <utility>
#include <vector>
#include <string>

namespace ParaCL {

class RuntimeContext;
class AstVisitor;

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
    Minus,
    LogicalNot
};

class Expression {
public:
    virtual int evaluate(RuntimeContext&) const = 0;
    virtual void accept(AstVisitor& visitor) const;
    virtual ~Expression() = default;
};

class NumberExpression : public Expression {
public:
    explicit NumberExpression(int value) : value_(value) {}
    int evaluate(RuntimeContext&) const override { return value_; }
    void accept(AstVisitor& visitor) const override;
private:
    int value_;
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(BinaryOperator operation, std::unique_ptr<Expression> left, std::unique_ptr<Expression> right);
    int evaluate(RuntimeContext& context) const override;
    void accept(AstVisitor& visitor) const override;
private:
    BinaryOperator operation_;
    std::unique_ptr<Expression> left_;
    std::unique_ptr<Expression> right_;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(UnaryOperator operation, std::unique_ptr<Expression> operand);
    int evaluate(RuntimeContext& context) const override;
    void accept(AstVisitor& visitor) const override;
private:
    UnaryOperator operation_;
    std::unique_ptr<Expression> operand_;
};

class VariableExpression : public Expression {
public:
    explicit VariableExpression(std::string name) : name_(std::move(name)) {}
    int evaluate(RuntimeContext& context) const override;
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


class Statement;

class AstVisitor {
public:
    virtual void visit_empty() = 0;
    virtual void visit_output(const Expression& expression) = 0;
    virtual void visit_expression_statement(const Expression& expression) = 0;
    virtual void visit_if(const Expression& condition, const Statement& then_branch, const Statement* else_branch) = 0;
    virtual void visit_while(const Expression& condition, const Statement& body) = 0;
    virtual void visit_number(int number) = 0;
    virtual void visit_unary(UnaryOperator operation, const Expression& operand) = 0;
    virtual void visit_binary(BinaryOperator operation, const Expression& left, const Expression& right) = 0;
    virtual void enter_block() = 0;
    virtual void leave_block() = 0;
    virtual ~AstVisitor() = default;
};

class Statement {
public:
    virtual void execute(RuntimeContext&) const = 0;
    virtual void accept(AstVisitor& visitor) const = 0;
    virtual ~Statement() = default;
};

class ExpressionStatement : public Statement {
public:
    explicit ExpressionStatement(std::unique_ptr<Expression> expression);
    void execute(RuntimeContext& context) const override { expression_->evaluate(context); }
    void accept(AstVisitor& visitor) const override { visitor.visit_expression_statement(*expression_); }
private:
    std::unique_ptr<Expression> expression_;
};

class OutputStatement : public Statement {
public:
    explicit OutputStatement(std::unique_ptr<Expression> expression);
    void execute(RuntimeContext& context) const override;
    void accept(AstVisitor& visitor) const override { visitor.visit_output(*expression_); }
private:
    std::unique_ptr<Expression> expression_;
};

class BlockStatement : public Statement {
public:
    explicit BlockStatement(std::vector<std::unique_ptr<Statement>> statements);
    void execute(RuntimeContext& context) const override;
    void accept(AstVisitor& visitor) const override;
private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

class IfStatement : public Statement {
public:
    IfStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> then_branch,
                std::unique_ptr<Statement> else_branch = nullptr);
    void execute(RuntimeContext& context) const override;
    void accept(AstVisitor& visitor) const override {
        visitor.visit_if(*condition_, *then_branch_, else_branch_.get());
    }
private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> then_branch_;
    std::unique_ptr<Statement> else_branch_;
};

class WhileStatement : public Statement {
public:
    WhileStatement(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> body);
    void execute(RuntimeContext& context) const override;
    void accept(AstVisitor& visitor) const override { visitor.visit_while(*condition_, *body_); }
private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement> body_;
};

class EmptyStatement : public Statement {
public:
    void execute(RuntimeContext&) const override {}
    void accept(AstVisitor& visitor) const override { visitor.visit_empty(); }
};


class Program {
public:
    explicit Program(std::vector<std::unique_ptr<Statement>> statements);
    void execute(RuntimeContext& context) const;
    void accept(AstVisitor& visitor) const;
private:
    std::vector<std::unique_ptr<Statement>> statements_;
};

} // namespace ParaCL
