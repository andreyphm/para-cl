#include "runtime_context.hpp"

#include <gtest/gtest.h>

#include <format>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace ParaCL {
namespace {

TEST(RuntimeContextTest, CreatesAndUpdatesVariablesInGlobalScope) {
    std::istringstream input;
    RuntimeContext context{input};
    const RuntimeContext& view = context;

    context.set_variable("x", 42);
    EXPECT_EQ(view.get_variable("x"), 42);
    context.set_variable("x", -7);
    context.set_variable("y", 0);
    EXPECT_EQ(view.get_variable("x"), -7);
    EXPECT_EQ(view.get_variable("y"), 0);
}

TEST(RuntimeContextTest, OwnsVariableNames) {
    std::istringstream input;
    RuntimeContext context{input};
    std::string name = "original";
    context.set_variable(name, 42);
    name.assign(100, 'x');

    EXPECT_EQ(context.get_variable("original"), 42);
    EXPECT_THROW(context.get_variable(name), std::runtime_error);
}

TEST(RuntimeContextTest, ReportsMissingNameWithoutCreatingVariable) {
    std::istringstream input;
    RuntimeContext context{input};

    try {
        (void)context.get_variable("missing");
        FAIL() << "Expected an unknown-variable error";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string{error.what()}.find("missing"), std::string::npos);
    }
    EXPECT_THROW(context.get_variable("missing"), std::runtime_error);
    context.set_variable("missing", 5);
    EXPECT_EQ(context.get_variable("missing"), 5);
}

TEST(RuntimeContextTest, UpdatesExistingOuterVariableWithoutShadowing) {
    std::istringstream input;
    RuntimeContext context{input};
    context.set_variable("x", 1);
    context.enter_scope();

    EXPECT_EQ(context.get_variable("x"), 1);
    context.set_variable("x", 2);
    EXPECT_EQ(context.get_variable("x"), 2);
    context.leave_scope();
    EXPECT_EQ(context.get_variable("x"), 2);
}

TEST(RuntimeContextTest, RemovesOnlyVariablesCreatedInLeavingScope) {
    std::istringstream input;
    RuntimeContext context{input};
    context.set_variable("global", 1);
    context.enter_scope();
    context.set_variable("outer", 2);
    context.enter_scope();
    context.set_variable("inner", 3);
    context.set_variable("outer", 4);

    context.leave_scope();
    EXPECT_THROW(context.get_variable("inner"), std::runtime_error);
    EXPECT_EQ(context.get_variable("outer"), 4);
    EXPECT_EQ(context.get_variable("global"), 1);
    context.leave_scope();
    EXPECT_THROW(context.get_variable("outer"), std::runtime_error);
    EXPECT_EQ(context.get_variable("global"), 1);
}

TEST(RuntimeContextTest, CanCreateSameNameAfterItsScopeIsRemoved) {
    std::istringstream input;
    RuntimeContext context{input};
    context.enter_scope();
    context.set_variable("local", 1);
    context.leave_scope();
    context.enter_scope();

    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
    context.set_variable("local", 2);
    EXPECT_EQ(context.get_variable("local"), 2);
    context.leave_scope();
    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
}

TEST(RuntimeContextTest, PreservesGlobalScopeAfterRejectedExit) {
    std::istringstream input;
    RuntimeContext context{input};
    context.set_variable("x", 1);

    EXPECT_THROW(context.leave_scope(), std::logic_error);
    EXPECT_EQ(context.get_variable("x"), 1);
    context.set_variable("y", 2);
    EXPECT_EQ(context.get_variable("y"), 2);
    context.enter_scope();
    context.leave_scope();
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(RuntimeContextTest, HandlesManyNestedScopes) {
    std::istringstream input;
    RuntimeContext context{input};
    context.set_variable("global", 0);
    constexpr int scope_count = 128;

    for (int i = 0; i < scope_count; ++i) {
        context.enter_scope();
        context.set_variable(std::format("local_{}", i), i);
        context.set_variable("global", i);
    }
    for (int i = scope_count; i > 0; --i) {
        const std::string name = std::format("local_{}", i - 1);
        EXPECT_EQ(context.get_variable(name), i - 1);
        EXPECT_EQ(context.get_variable("global"), scope_count - 1);
        context.leave_scope();
        EXPECT_THROW(context.get_variable(name), std::runtime_error);
    }
    EXPECT_EQ(context.get_variable("global"), scope_count - 1);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(ScopeOwnerTest, RemovesLocalVariablesOnNormalExit) {
    std::istringstream input;
    RuntimeContext context{input};
    context.set_variable("global", 1);
    {
        const ScopeOwner scope{context};
        context.set_variable("local", 2);
        context.set_variable("global", 3);
        EXPECT_EQ(context.get_variable("local"), 2);
    }

    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
    EXPECT_EQ(context.get_variable("global"), 3);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(ScopeOwnerTest, NestedOwnerPreservesOuterScope) {
    std::istringstream input;
    RuntimeContext context{input};
    {
        const ScopeOwner outer{context};
        context.set_variable("outer", 1);
        {
            const ScopeOwner inner{context};
            context.set_variable("inner", 2);
            context.set_variable("outer", 3);
        }
        EXPECT_THROW(context.get_variable("inner"), std::runtime_error);
        EXPECT_EQ(context.get_variable("outer"), 3);
    }
    EXPECT_THROW(context.get_variable("outer"), std::runtime_error);
}

TEST(ScopeOwnerTest, RemovesNestedScopesDuringExceptionUnwinding) {
    std::istringstream input;
    RuntimeContext context{input};
    context.set_variable("global", 1);

    EXPECT_THROW({
        const ScopeOwner outer{context};
        context.set_variable("outer", 2);
        const ScopeOwner inner{context};
        context.set_variable("inner", 3);
        throw std::runtime_error("Execution failed");
    }, std::runtime_error);

    EXPECT_THROW(context.get_variable("outer"), std::runtime_error);
    EXPECT_THROW(context.get_variable("inner"), std::runtime_error);
    EXPECT_EQ(context.get_variable("global"), 1);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

TEST(ScopeOwnerTest, RemovesScopeOnEarlyReturn) {
    std::istringstream input;
    RuntimeContext context{input};
    const auto execute_block = [&context]() {
        const ScopeOwner scope{context};
        context.set_variable("local", 42);
        return context.get_variable("local");
    };

    EXPECT_EQ(execute_block(), 42);
    EXPECT_THROW(context.get_variable("local"), std::runtime_error);
    EXPECT_THROW(context.leave_scope(), std::logic_error);
}

} // namespace
} // namespace ParaCL
