#pragma once

#include <format>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
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

    int get_variable(const std::string& name) const {
        const int* variable_value = find_variable_value(name);
        if (!variable_value) {
            throw std::runtime_error(std::format("Unknown variable: {}", name));
        }
        return *variable_value;
    }

    void set_variable(const std::string& name, const int value) {
        int* variable_value = find_variable_value(name);
        if (!variable_value) {
            create_variable(name, value);
            return;
        }
        *variable_value = value;
    }

    void enter_scope() {
        scopes_.emplace_back();
    }

    void leave_scope() {
        if (scopes_.size() <= 1) {
            throw std::logic_error("Cannot leave the global scope");
        }
        scopes_.pop_back();
    }

private:
    std::vector<std::unordered_map<std::string, int>> scopes_{1};

    const int* find_variable_value(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            const auto variable_it = it->find(name);
            if (variable_it != it->end()) {
                return std::addressof(variable_it->second);
            }
        }
        return nullptr;
    }

    int* find_variable_value(const std::string& name) {
        return const_cast<int*>(std::as_const(*this).find_variable_value(name));
    }

    void create_variable(const std::string& name, const int value) {
        auto& current_scope = scopes_.back();
        current_scope.emplace(name, value);
    }
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
