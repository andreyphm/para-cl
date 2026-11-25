#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

#include "input.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "runtime_context.hpp"

#ifdef ENABLE_LLVM_IR_GEN
#include "codegen.hpp"
#endif

namespace {

void print_usage(const char* program_name) {
    std::filesystem::path program_path{(program_name && program_name[0] != '\0') ? program_name : "para_cl"};
    if (program_path.is_absolute()) {
        std::error_code error;
        const auto launch_directory = std::filesystem::current_path(error);
        if (!error) {
            program_path = program_path.lexically_proximate(launch_directory);
        }
    }
    std::cerr << "Usage: " << program_path.make_preferred().string() << " [--emit-llvm] <source_file>\n";
}

} // namespace

int main(int argc, char* argv[]) {
    const bool emit_llvm = argc > 1 && std::string_view{argv[1]} == "--emit-llvm";
    if (argc != (emit_llvm ? 3 : 2)) {
        print_usage(argv[0]);
        return 1;
    }
    try {
        auto tokens = ParaCL::tokenize(ParaCL::read_source_file(argv[argc - 1]));
        ParaCL::Parser parser{std::move(tokens)};
        const auto program = parser.parse_program();
        if (emit_llvm) {
#ifdef ENABLE_LLVM_IR_GEN
            ParaCL::emit_llvm_ir(std::cout);
            if (!std::cout.flush()) {
                throw std::runtime_error("Failed to write LLVM IR");
            }
            return 0;
#else
            throw std::runtime_error("LLVM IR generation is disabled in this build");
#endif
        }
        ParaCL::RuntimeContext context{std::cin, std::cout};
        program->execute(context);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
