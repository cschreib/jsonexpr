#include "jsonexpr/ast.hpp"

#include <cmath>

using namespace jsonexpr;

namespace {
std::string dump_content(const ast::identifier& v, std::size_t indent) {
    return std::string(2 * indent, ' ') + "identifier(" + std::string(v.name) + ")";
}

std::string dump_content(const ast::literal& l, std::size_t indent) {
    return std::string(2 * indent, ' ') + "literal(" + std::string(l.value.dump()) + ")";
}

std::string dump_content(const ast::array& a, std::size_t indent) {
    std::string str = std::string(2 * indent, ' ') + "array({";
    for (const auto& e : a.data) {
        str += "\n" + dump(e, indent + 1);
    }
    str += "})";
    return str;
}

std::string dump_content(const ast::object& a, std::size_t indent) {
    std::string str = std::string(2 * indent, ' ') + "object({";
    for (const auto& e : a.data) {
        str += "\n" + dump(e.first, indent + 1) + " :";
        str += "\n" + dump(e.second, indent + 1);
    }
    str += "})";
    return str;
}

std::string dump_content(const ast::function& v, std::size_t indent) {
    std::string str = std::string(2 * indent, ' ') + "function(" + std::string(v.name) + ", args={";
    for (const auto& a : v.args) {
        str += "\n" + dump(a, indent + 1);
    }
    str += "})";
    return str;
}
} // namespace

std::string jsonexpr::ast::dump(const ast::node& n, std::size_t indent) {
    return std::visit([&](const auto& c) { return dump_content(c, indent); }, n.content);
}

error jsonexpr::ast::node_error(const ast::node& n, std::string message) {
    return error{.location = n.location, .message = std::move(message)};
}
