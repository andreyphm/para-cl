#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ParaCL {

class RuntimeContext {
public:
    std::istream& input;
    std::ostream& output;

    explicit RuntimeContext(std::istream& in_stream, std::ostream& out_stream) : input(in_stream), output(out_stream) {}

    int get_variable(const std::string& name) const;
    void set_variable(const std::string& name, int value);

    void enter_scope() { scopes_.emplace_back(); }
    void leave_scope();

private:
    std::vector<std::unordered_map<std::string, int>> scopes_{1};

    const int* find_variable_value(const std::string& name) const;
    int* find_variable_value(const std::string& name) { return const_cast<int*>(std::as_const(*this).find_variable_value(name)); }
    void create_variable(const std::string& name, const int value);
};

class ScopeOwner {
public:
    explicit ScopeOwner(RuntimeContext& context) : context_(context) { context_.enter_scope(); }
    ~ScopeOwner() { context_.leave_scope(); }

    ScopeOwner(const ScopeOwner&) = delete;
    ScopeOwner& operator=(const ScopeOwner&) = delete;

private:
    RuntimeContext& context_;
};

}
