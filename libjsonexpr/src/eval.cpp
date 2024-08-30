#include "jsonexpr/eval.hpp"

using namespace jsonexpr;

namespace {
expected<json, error> eval(
    const ast::node&         n,
    const ast::identifier&   v,
    const variable_registry& vreg,
    const function_registry&) {

    auto iter = vreg.find(std::string{v.name});
    if (iter == vreg.end()) {
        return unexpected(node_error(n, "unknown variable '" + std::string(v.name) + "'"));
    }

    return iter->second;
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

    auto iter = freg.find(std::string{f.name});
    if (iter == freg.end()) {
        return unexpected(node_error(n, "unknown function '" + std::string{f.name} + "'"));
    }

    auto overload = iter->second.find(f.args.size());
    if (overload == iter->second.end()) {
        return unexpected(node_error(
            n, "function does not take '" + std::to_string(f.args.size()) + "' arguments"));
    }

    const auto& func = overload->second.callable;

    try {
        auto result = func(std::span<const ast::node>(f.args.begin(), f.args.end()), vreg, freg);
        if (result.has_value()) {
            return result.value();
        } else {
            const auto& e = result.error();
            if (e.location.length == 0) {
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

expected<json, error> eval(
    const ast::node&,
    const ast::array&        a,
    const variable_registry& vreg,
    const function_registry& freg) {

    json output(json::value_t::array);
    for (const auto& elem : a.data) {
        auto result = eval(elem, vreg, freg);
        if (!result.has_value()) {
            return unexpected(result.error());
        }

        output.push_back(std::move(result.value()));
    }

    return output;
}

expected<json, error> eval(
    const ast::node&,
    const ast::object&       a,
    const variable_registry& vreg,
    const function_registry& freg) {

    json output(json::value_t::object);
    for (const auto& elem : a.data) {
        auto key = eval(elem.first, vreg, freg);
        if (!key.has_value()) {
            return unexpected(key.error());
        }
        if (!key.value().is_string()) {
            return unexpected(node_error(
                elem.first, "object key must be a string, got '" +
                                std::string(get_type_name(key.value())) + "'"));
        }

        auto value = eval(elem.second, vreg, freg);
        if (!value.has_value()) {
            return unexpected(value.error());
        }

        output[key.value().get<std::string>()] = std::move(value.value());
    }

    return output;
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
