#ifndef JSONEXPR_AST_HPP
#define JSONEXPR_AST_HPP

#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace jsonexpr {
using json = nlohmann::json;

struct source_location {
    std::size_t      position;
    std::string_view content;
};

namespace ast {
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
} // namespace ast
} // namespace jsonexpr

#endif
