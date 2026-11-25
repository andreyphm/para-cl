#pragma once

#include <iosfwd>

namespace ParaCL {

class Program;

void emit_llvm_ir(const Program& program, std::ostream& output);

} // namespace ParaCL
