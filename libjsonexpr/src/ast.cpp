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

namespace {
error make_error(const ast::node& n, std::string message) {
    return error{
        .position = n.location.position,
        .length   = n.location.content.length(),
        .message  = std::move(message)};
}

error make_error(std::string message) {
    return error{.message = std::move(message)};
}
} // namespace

namespace {
expected<json, error> eval(
    const ast::node&         n,
    const ast::variable&     v,
    const variable_registry& vreg,
    const function_registry&) {

    auto dot_pos = v.name.find_first_of(".");
    auto name    = v.name.substr(0, dot_pos);
    auto iter    = vreg.find(name.substr(0, dot_pos));
    if (iter == vreg.end()) {
        return unexpected(error{
            n.location.position, name.size(), "unknown variable '" + std::string(name) + "'"});
    }

    const json* object = &iter->second;
    while (dot_pos != v.name.npos) {
        const auto prev_pos = dot_pos + 1;

        dot_pos = v.name.find_first_of(".", prev_pos);
        name    = v.name.substr(prev_pos, dot_pos - prev_pos);

        if (!object->contains(name)) {
            return unexpected(error{
                n.location.position + prev_pos, name.size(),
                "unknown field '" + std::string(name) + "'"});
        }

        object = &(*object)[name];
    }

    return *object;
}

expected<json, error>
eval(const ast::node&, const ast::literal& v, const variable_registry&, const function_registry&) {
    return v.value;
}

expected<json, error>
eval(const ast::node& n, const variable_registry& vreg, const function_registry& freg);

expected<json, error> eval(
    const ast::node&         n,
    const ast::function&     f,
    const variable_registry& vreg,
    const function_registry& freg) {

    auto iter = freg.find(f.name);
    if (iter == freg.end()) {
        return unexpected(make_error(n, "unknown function '" + std::string(f.name) + "'"));
    }

    auto overload = iter->second.find(f.args.size());
    if (overload == iter->second.end()) {
        return unexpected(make_error(
            n, "function does not take '" + std::to_string(f.args.size()) + "' arguments"));
    }

    json args(json::value_t::array);
    for (const auto& arg : f.args) {
        auto eval_arg = eval(arg, vreg, freg);
        if (!eval_arg.has_value()) {
            return unexpected(eval_arg.error());
        }
        args.push_back(std::move(eval_arg.value()));
    }

    try {
        auto result = overload->second(args);
        if (result.has_value()) {
            return result.value();
        } else {
            return unexpected(make_error(n, result.error()));
        }
    } catch (const std::exception& error) {
        return unexpected(
            make_error(n, "exception when evaluating function: " + std::string(error.what())));
    } catch (...) {
        return unexpected(make_error(n, "unknown exception when evaluating function"));
    }
}

expected<json, error>
eval(const ast::node& n, const variable_registry& vreg, const function_registry& freg) {
    return std::visit([&](const auto& c) { return eval(n, c, vreg, freg); }, n.content);
}
} // namespace

expected<json, error> jsonexpr::ast::evaluate(
    const ast::node& n, const variable_registry& vreg, const function_registry& freg) {
    return eval(n, vreg, freg);
}
