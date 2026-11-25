#include "codegen.hpp"

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

void emit_llvm_ir(std::ostream& output) {
    llvm::LLVMContext context;
    llvm::Module module{"para_cl", context};

    auto* main_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(context), false);
    auto* main_function = llvm::Function::Create(main_type, llvm::GlobalValue::ExternalLinkage, "main", module);
    auto* entry = llvm::BasicBlock::Create(context, "entry", main_function);
    llvm::IRBuilder<> builder{entry};
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
