#include "runtime_context.hpp"

#include <format>
#include <memory>
#include <stdexcept>

namespace ParaCL {

int RuntimeContext::get_variable(const std::string& name) const {
    const int* variable_value = find_variable_value(name);
    if (!variable_value) {
        throw std::runtime_error(std::format("Unknown variable: {}", name));
    }
    return *variable_value;
}

void RuntimeContext::set_variable(const std::string& name, const int value) {
    int* variable_value = find_variable_value(name);
    if (!variable_value) {
        create_variable(name, value);
        return;
    }
    *variable_value = value;
}

void RuntimeContext::leave_scope() {
    if (scopes_.size() <= 1) {
        throw std::logic_error("Cannot leave the global scope");
    }
    scopes_.pop_back();
}

const int* RuntimeContext::find_variable_value(const std::string& name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        const auto variable_it = it->find(name);
        if (variable_it != it->end()) {
            return std::addressof(variable_it->second);
        }
    }
    return nullptr;
}

void RuntimeContext::create_variable(const std::string& name, const int value) {
    auto& current_scope = scopes_.back();
    current_scope.emplace(name, value);
}

} // namespace ParaCL
