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
        auto* printf_type = llvm::FunctionType::get(builder_.getInt32Ty(), {builder_.getPtrTy()}, true);
        const auto print_function = module_.getOrInsertFunction("printf", printf_type);
        if (!output_format_) {
            output_format_ = builder_.CreateGlobalString("%d\n", "output_format");
        }
        builder_.CreateCall(print_function, {output_format_, expression_value_});
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

    void visit_expression_statement(const Expression&) override {
        throw std::runtime_error("LLVM IR generation for expression statements is not implemented");
    }

    void visit_if(const Expression&, const Statement&, const Statement*) override {
        throw std::runtime_error("LLVM IR generation for if statements is not implemented");
    }

    void visit_while(const Expression&, const Statement&) override {
        throw std::runtime_error("LLVM IR generation for while statements is not implemented");
    }

private:
    llvm::Module& module_;
    llvm::IRBuilder<>& builder_;
    llvm::Value* expression_value_ = nullptr;
    llvm::GlobalVariable* output_format_ = nullptr;
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
