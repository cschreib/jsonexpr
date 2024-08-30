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

    const auto& func = iter->second;

    try {
        const auto args = std::span<const ast::node>(f.args.begin(), f.args.end());

        auto result = [&]() -> function_result {
            if (std::holds_alternative<function::ast_function_t>(func.overloads)) {
                return std::get<function::ast_function_t>(func.overloads)(args, vreg, freg);
            } else {
                const auto& map = std::get<function::overload_t>(func.overloads);

                std::vector<json> json_args;
                std::string       key;
                for (const auto& arg : args) {
                    auto eval_arg = evaluate(arg, vreg, freg);
                    if (!eval_arg.has_value()) {
                        return unexpected(eval_arg.error());
                    }
                    json_args.push_back(std::move(eval_arg.value()));

                    if (!key.empty()) {
                        key += ",";
                    }
                    key += get_dynamic_type_name(json_args.back());
                }

                auto iter = map.find(key);
                if (iter == map.end()) {
                    std::string message = "no overload of '" + std::string{f.name} +
                                          "' accepts the provided types (" + key + ")\n" +
                                          "note: available overloads:";
                    for (const auto& [key, value] : map) {
                        message += "\n        " + std::string{f.name} + "(" + key + ")";
                    }

                    return unexpected(node_error(n, message));
                }

                auto result =
                    iter->second(std::span<const json>(json_args.cbegin(), json_args.cend()));
                if (result.has_value()) {
                    return result.value();
                } else {
                    return unexpected(error{.message = result.error()});
                }
            }
        }();

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
                                std::string(get_dynamic_type_name(key.value())) + "'"));
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
