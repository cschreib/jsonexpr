#include "jsonexpr/eval.hpp"

using namespace jsonexpr;

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
        return unexpected(node_error(n, "unknown function '" + std::string(f.name) + "'"));
    }

    auto overload = iter->second.find(f.args.size());
    if (overload == iter->second.end()) {
        return unexpected(node_error(
            n, "function does not take '" + std::to_string(f.args.size()) + "' arguments"));
    }

    const auto& func = overload->second.callable;

    try {
        auto result = func(std::span<const ast::node>(f.args), vreg, freg);
        if (result.has_value()) {
            return result.value();
        } else {
            const auto& e = result.error();
            if (e.length == 0) {
                return unexpected(node_error(n, e.message));
            } else {
                return unexpected(e);
            }
        }
    } catch (const std::exception& error) {
        return unexpected(
            node_error(n, "exception when evaluating function: " + std::string(error.what())));
    } catch (...) {
        return unexpected(node_error(n, "unknown exception when evaluating function"));
    }
}

expected<json, error>
eval(const ast::node& n, const variable_registry& vreg, const function_registry& freg) {
    return std::visit([&](const auto& c) { return eval(n, c, vreg, freg); }, n.content);
}
} // namespace

expected<json, error> jsonexpr::evaluate(
    const ast::node& n, const variable_registry& vreg, const function_registry& freg) {
    return eval(n, vreg, freg);
}
