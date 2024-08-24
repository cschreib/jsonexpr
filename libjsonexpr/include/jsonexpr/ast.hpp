#ifndef JSONEXPR_AST_HPP
#define JSONEXPR_AST_HPP

#include "jsonexpr/base.hpp"

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace jsonexpr::ast {
struct node;

struct variable {
    std::string_view name;
};

struct literal {
    json value;
};

struct function {
    std::string_view  name;
    std::vector<node> args;
};

struct node {
    source_location                           location;
    std::variant<variable, literal, function> content;
};

std::string dump(const node& n, std::size_t indent = 0);

tl::expected<json, error> evaluate(
    const ast::node&         n,
    const variable_registry& vreg = {},
    const function_registry& freg = default_functions());
} // namespace jsonexpr::ast

#endif
