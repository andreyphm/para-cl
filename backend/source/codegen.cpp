#include "codegen.hpp"
#include "ast.hpp"

#include <stdexcept>
#include <string>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/raw_ostream.h>

namespace ParaCL {
namespace {

class LLVMGenerator final : public AstVisitor {
public:
    LLVMGenerator(llvm::Module& module, llvm::IRBuilder<>& builder) : module_(module), builder_(builder) {}

    void visit_empty() override {}
    void enter_block() override {}
    void leave_block() override {}

    void visit_output(const Expression& expression) override {
        expression.accept(*this);
        auto* output_func_type = llvm::FunctionType::get(builder_.getVoidTy(), {builder_.getInt32Ty()}, false);
        const auto output_func = module_.getOrInsertFunction("paracl_output", output_func_type);
        builder_.CreateCall(output_func, {expression_value_});
    }

    void visit_input() override {
        auto* input_func_type = llvm::FunctionType::get(builder_.getInt32Ty(), false);
        const auto input_func = module_.getOrInsertFunction("paracl_input", input_func_type);
        expression_value_ = builder_.CreateCall(input_func);
    }

    void visit_number(int value) override {
        expression_value_ = builder_.getInt32(value);
    }

    void visit_unary(UnaryOperator operation, const Expression& operand) override {
        operand.accept(*this);
        switch (operation) {
            case UnaryOperator::Plus:
                return;
            case UnaryOperator::Minus:
                expression_value_ = builder_.CreateNeg(expression_value_);
                return;
            case UnaryOperator::LogicalNot: {
                auto* is_zero = builder_.CreateICmpEQ(expression_value_, builder_.getInt32(0));
                expression_value_ = builder_.CreateZExt(is_zero, builder_.getInt32Ty());
                return;
            }
        }
        throw std::logic_error("Unknown unary operator");
    }

    void visit_expression_statement(const Expression& expression) override {
        expression.accept(*this);
    }

    void visit_binary(BinaryOperator operation, const Expression& left, const Expression& right) override {
        left.accept(*this);
        auto* left_value = expression_value_;
        if (operation == BinaryOperator::LogicalAnd || operation == BinaryOperator::LogicalOr) {
            expression_value_ = emit_short_circuit(operation, left_value, right);
            return;
        }
        right.accept(*this);
        auto* right_value = expression_value_;

        llvm::CmpInst::Predicate predicate;
        switch (operation) {
            case BinaryOperator::Add:
                expression_value_ = builder_.CreateAdd(left_value, right_value);
                return;
            case BinaryOperator::Sub:
                expression_value_ = builder_.CreateSub(left_value, right_value);
                return;
            case BinaryOperator::Mul:
                expression_value_ = builder_.CreateMul(left_value, right_value);
                return;
            case BinaryOperator::Div:
                expression_value_ = builder_.CreateSDiv(left_value, right_value);
                return;
            case BinaryOperator::Equal:
                predicate = llvm::CmpInst::ICMP_EQ;
                break;
            case BinaryOperator::NotEqual:
                predicate = llvm::CmpInst::ICMP_NE;
                break;
            case BinaryOperator::Less:
                predicate = llvm::CmpInst::ICMP_SLT;
                break;
            case BinaryOperator::LessEqual:
                predicate = llvm::CmpInst::ICMP_SLE;
                break;
            case BinaryOperator::Greater:
                predicate = llvm::CmpInst::ICMP_SGT;
                break;
            case BinaryOperator::GreaterEqual:
                predicate = llvm::CmpInst::ICMP_SGE;
                break;
            default:
                throw std::runtime_error("LLVM IR generation for this binary operator is not implemented");
        }
        auto* comparison = builder_.CreateICmp(predicate, left_value, right_value);
        expression_value_ = builder_.CreateZExt(comparison, builder_.getInt32Ty());
    }

    void visit_if(const Expression& condition, const Statement& then_branch, const Statement* else_branch) override {
        condition.accept(*this);
        auto* is_true = builder_.CreateICmpNE(expression_value_, builder_.getInt32(0));
        auto* function = builder_.GetInsertBlock()->getParent();
        auto& context = module_.getContext();
        auto* then_block = llvm::BasicBlock::Create(context, "if.then", function);
        auto* else_block = else_branch ? llvm::BasicBlock::Create(context, "if.else", function) : nullptr;
        auto* end_block = llvm::BasicBlock::Create(context, "if.end", function);
        builder_.CreateCondBr(is_true, then_block, else_block ? else_block : end_block);

        builder_.SetInsertPoint(then_block);
        then_branch.accept(*this);
        builder_.CreateBr(end_block);

        if (else_branch) {
            builder_.SetInsertPoint(else_block);
            else_branch->accept(*this);
            builder_.CreateBr(end_block);
        }
        builder_.SetInsertPoint(end_block);
    }

    void visit_while(const Expression& condition, const Statement& body) override {
        auto* function = builder_.GetInsertBlock()->getParent();
        auto& context = module_.getContext();
        auto* condition_block = llvm::BasicBlock::Create(context, "while.cond", function);
        auto* body_block = llvm::BasicBlock::Create(context, "while.body", function);
        auto* end_block = llvm::BasicBlock::Create(context, "while.end", function);
        builder_.CreateBr(condition_block);

        builder_.SetInsertPoint(condition_block);
        condition.accept(*this);
        auto* is_true = builder_.CreateICmpNE(expression_value_, builder_.getInt32(0));
        builder_.CreateCondBr(is_true, body_block, end_block);

        builder_.SetInsertPoint(body_block);
        body.accept(*this);
        builder_.CreateBr(condition_block);

        builder_.SetInsertPoint(end_block);
    }

private:
    llvm::Value* emit_short_circuit(BinaryOperator operation, llvm::Value* left, const Expression& right) {
        const bool is_or = (operation == BinaryOperator::LogicalOr);
        auto* left_block = builder_.GetInsertBlock();
        auto* function = left_block->getParent();
        auto* rhs = llvm::BasicBlock::Create(module_.getContext(), is_or ? "or.rhs" : "and.rhs", function);
        auto* merge = llvm::BasicBlock::Create(module_.getContext(), is_or ? "or.merge" : "and.merge", function);

        auto* left_is_true = builder_.CreateICmpNE(left, builder_.getInt32(0));
        if (is_or) {
            builder_.CreateCondBr(left_is_true, merge, rhs);
        } else {
            builder_.CreateCondBr(left_is_true, rhs, merge);
        }

        builder_.SetInsertPoint(rhs);
        right.accept(*this);
        auto* right_is_true = builder_.CreateICmpNE(expression_value_, builder_.getInt32(0));
        auto* right_end = builder_.GetInsertBlock();

        builder_.CreateBr(merge);
        builder_.SetInsertPoint(merge);
        auto* result = builder_.CreatePHI(builder_.getInt1Ty(), 2, is_or ? "or.result" : "and.result");
        result->addIncoming(builder_.getInt1(is_or), left_block);
        result->addIncoming(right_is_true, right_end);

        return builder_.CreateZExt(result, builder_.getInt32Ty());
    }

    llvm::Module& module_;
    llvm::IRBuilder<>& builder_;
    llvm::Value* expression_value_ = nullptr;
};

} // namespace

void emit_llvm_ir(const Program& program, std::ostream& output) {
    llvm::LLVMContext context;
    llvm::Module module{"para_cl", context};

    auto* main_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false);
    auto* main_function = llvm::Function::Create(main_type, llvm::GlobalValue::ExternalLinkage, "main", module);
    auto* entry = llvm::BasicBlock::Create(context, "entry", main_function);
    llvm::IRBuilder<> builder{entry};
    LLVMGenerator generator{module, builder};
    program.accept(generator);
    builder.CreateRet(builder.getInt32(0));

    std::string diagnostics;
    llvm::raw_string_ostream diagnostic_output{diagnostics};
    if (llvm::verifyModule(module, &diagnostic_output)) {
        throw std::runtime_error(diagnostics);
    }

    llvm::raw_os_ostream ir_output{output};
    module.print(ir_output, nullptr);
}

} // namespace ParaCL
