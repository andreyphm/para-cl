#include <exception>
#include <iostream>

#include "input.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: para_cl <source_file>\n";
        return 1;
    }
    try {
        ParaCL::read_source_file(argv[1]);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
