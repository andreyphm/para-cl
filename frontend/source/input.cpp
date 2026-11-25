#include "input.hpp"

#include <format>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ParaCL {

std::string read_source_file(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error(std::format("Cannot open source file: {}", path));
    }

    const auto file_size = static_cast<std::streamoff>(file.tellg());
    if (file_size < 0) {
        throw std::runtime_error(std::format("Cannot get file end position: {}", path));
    }
    if (file_size > std::numeric_limits<std::streamsize>::max() ||
        std::cmp_greater(file_size, std::string{}.max_size())) {
        throw std::runtime_error(std::format("Source file is too large: {}", path));
    }

    file.seekg(0);
    if (!file) {
        throw std::runtime_error(std::format("Cannot seek to the beginning of source file: {}", path));
    }

    std::string source(static_cast<std::string::size_type>(file_size), '\0');
    file.read(source.data(), static_cast<std::streamsize>(file_size));
    if (!file) {
        throw std::runtime_error(std::format("Cannot read source file: {}", path));
    }

    return source;
}

} // namespace ParaCL
