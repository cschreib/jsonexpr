#include "jsonexpr/eval.hpp"

using namespace jsonexpr::ast;

namespace {
std::string dump_content(const variable& v, std::size_t indent) {
    return std::string(2 * indent, ' ') + "variable(" + std::string(v.name) + ")";
}

std::string dump_content(const literal& l, std::size_t indent) {
    return std::string(2 * indent, ' ') + "literal(" + std::string(l.value.dump()) + ")";
}

std::string dump_content(const function& v, std::size_t indent) {
    std::string heading = std::string(2 * indent, ' ');
    std::string str     = heading + "function(" + std::string(v.name) + ", args={\n";
    for (const auto& a : v.args) {
        str += dump(a, indent + 1) + "\n";
    }
    str += heading + "})";
    return str;
}
} // namespace

std::string jsonexpr::ast::dump(const node& n, std::size_t indent) {
    return std::visit([&](const auto& c) { return dump_content(c, indent); }, n.content);
}
