#ifndef JSONEXPR_AST_HPP
#define JSONEXPR_AST_HPP

#include "jsonexpr/base.hpp"
#include "jsonexpr/config.hpp"

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace jsonexpr::ast {
struct node;

struct identifier {
    std::string_view name;
};

struct literal {
    json value;
};

struct array {
    std::vector<node> data;
};

struct object {
    std::vector<std::pair<node, node>> data;
};

struct function {
    std::string_view  name;
    std::vector<node> args;
};

struct node {
    source_location                                            location;
    std::variant<identifier, literal, function, array, object> content;
};

JSONEXPR_EXPORT std::string dump(const node& n, std::size_t indent = 0);

JSONEXPR_EXPORT error node_error(const ast::node& n, std::string message);
} // namespace jsonexpr::ast

#endif
