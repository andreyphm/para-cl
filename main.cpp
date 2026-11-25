#include <exception>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <utility>

#include "input.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "runtime_context.hpp"

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
    std::cerr << "Usage: " << program_path.make_preferred().string() << " <source_file>\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }
    try {
        auto tokens = ParaCL::tokenize(ParaCL::read_source_file(argv[1]));
        ParaCL::Parser parser{std::move(tokens)};
        const auto program = parser.parse_program();
        ParaCL::RuntimeContext context{std::cin, std::cout};
        program->execute(context);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
