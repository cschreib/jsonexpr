#include "jsonexpr/ast.hpp"

#include <cmath>

using namespace jsonexpr;

namespace {
std::string dump_content(const ast::variable& v, std::size_t indent) {
    return std::string(2 * indent, ' ') + "variable(" + std::string(v.name) + ")";
}

std::string dump_content(const ast::literal& l, std::size_t indent) {
    return std::string(2 * indent, ' ') + "literal(" + std::string(l.value.dump()) + ")";
}

std::string dump_content(const ast::function& v, std::size_t indent) {
    std::string heading = std::string(2 * indent, ' ');
    std::string str     = heading + "function(" + std::string(v.name) + ", args={\n";
    for (const auto& a : v.args) {
        str += dump(a, indent + 1) + "\n";
    }
    str += heading + "})";
    return str;
}
} // namespace

std::string jsonexpr::ast::dump(const ast::node& n, std::size_t indent) {
    return std::visit([&](const auto& c) { return dump_content(c, indent); }, n.content);
}

error jsonexpr::ast::node_error(const ast::node& n, std::string message) {
    return error{
        .position = n.location.position,
        .length   = n.location.content.length(),
        .message  = std::move(message)};
}
