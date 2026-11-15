#pragma once

namespace ParaCL {

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

} // namespace ParaCL
