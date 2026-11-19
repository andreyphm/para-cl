#include <exception>
#include <iostream>
#include <utility>

#include "input.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "runtime_context.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: para_cl <source_file>\n";
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
