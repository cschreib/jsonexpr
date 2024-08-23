#include "jsonexpr/eval.hpp"

#include <cmath>

using namespace jsonexpr;

namespace {
tl::expected<json, eval_error> eval(
    const ast::node&         n,
    const ast::variable&     v,
    const variable_registry& vreg,
    const function_registry&) {

    auto dot_pos = v.name.find_first_of(".");
    auto name    = v.name.substr(0, dot_pos);
    auto iter    = vreg.find(name.substr(0, dot_pos));
    if (iter == vreg.end()) {
        return tl::unexpected(eval_error(
            n.location.position, name.size(), "unknown variable '" + std::string(name) + "'"));
    }

    const json* object = &iter->second;
    while (dot_pos != v.name.npos) {
        const auto prev_pos = dot_pos + 1;

        dot_pos = v.name.find_first_of(".", prev_pos);
        name    = v.name.substr(prev_pos, dot_pos - prev_pos);

        if (!object->contains(name)) {
            return tl::unexpected(eval_error(
                n.location.position + prev_pos, name.size(),
                "unknown field '" + std::string(name) + "'"));
        }

        object = &(*object)[name];
    }

    return *object;
}

tl::expected<json, eval_error>
eval(const ast::node&, const ast::literal& v, const variable_registry&, const function_registry&) {
    return v.value;
}

tl::expected<json, eval_error>
eval(const ast::node& n, const variable_registry& vreg, const function_registry& freg);

tl::expected<json, eval_error> eval(
    const ast::node&         n,
    const ast::function&     f,
    const variable_registry& vreg,
    const function_registry& freg) {

    auto iter = freg.find(f.name);
    if (iter == freg.end()) {
        return tl::unexpected(eval_error(n, "unknown function '" + std::string(f.name) + "'"));
    }

    auto overload = iter->second.find(f.args.size());
    if (overload == iter->second.end()) {
        return tl::unexpected(eval_error(
            n, "function does not take '" + std::to_string(f.args.size()) + "' arguments"));
    }

    json args(json::value_t::array);
    for (const auto& arg : f.args) {
        auto eval_arg = eval(arg, vreg, freg);
        if (!eval_arg.has_value()) {
            return tl::unexpected(eval_arg.error());
        }
        args.push_back(std::move(eval_arg.value()));
    }

    try {
        auto result = overload->second(args);
        if (result.has_value()) {
            return result.value();
        } else {
            return tl::unexpected(eval_error(n, result.error().message));
        }
    } catch (const std::exception& error) {
        return tl::unexpected(
            eval_error(n, "exception when evaluating function: " + std::string(error.what())));
    } catch (...) {
        return tl::unexpected(eval_error(n, "unknown exception when evaluating function"));
    }
}

tl::expected<json, eval_error>
eval(const ast::node& n, const variable_registry& vreg, const function_registry& freg) {
    return std::visit([&](const auto& c) { return eval(n, c, vreg, freg); }, n.content);
}
} // namespace

tl::expected<json, eval_error> jsonexpr::evaluate_ast(
    const ast::node& n, const variable_registry& vreg, const function_registry& freg) {
    return eval(n, vreg, freg);
}

namespace {
using json_variant = std::variant<
    json::number_float_t,
    json::number_integer_t,
    json::string_t,
    json::array_t,
    json::boolean_t,
    json>;

json_variant to_variant(const json& j) {
    switch (j.type()) {
    case json::value_t::object: return j;
    case json::value_t::array: return j.get<json::array_t>();
    case json::value_t::string: return j.get<json::string_t>();
    case json::value_t::boolean: return j.get<json::boolean_t>();
    case json::value_t::number_unsigned: [[fallthrough]];
    case json::value_t::number_integer: return j.get<json::number_integer_t>();
    case json::value_t::number_float: return j.get<json::number_float_t>();
    default: throw std::runtime_error("cannot represent json type");
    }
}
} // namespace

#define UNARY_FUNCTION(NAME, EXPR)                                                                 \
    [](const json& args) {                                                                         \
        return std::visit(                                                                         \
            [](const auto& lhs) -> tl::expected<json, eval_error> {                                \
                if constexpr (requires { EXPR; }) {                                                \
                    return EXPR;                                                                   \
                } else {                                                                           \
                    return tl::unexpected(eval_error("incompatible type for '" NAME "'"));         \
                }                                                                                  \
            },                                                                                     \
            to_variant(args[0]));                                                                  \
    }

#define BINARY_FUNCTION(NAME, EXPR)                                                                \
    [](const json& args) {                                                                         \
        return std::visit(                                                                         \
            [](const auto& lhs, const auto& rhs) -> tl::expected<json, eval_error> {               \
                if constexpr (requires { EXPR; }) {                                                \
                    return EXPR;                                                                   \
                } else {                                                                           \
                    return tl::unexpected(eval_error("incompatible type for '" NAME "'"));         \
                }                                                                                  \
            },                                                                                     \
            to_variant(args[0]), to_variant(args[1]));                                             \
    }

#define UNARY_OPERATOR(OPERATOR) UNARY_FUNCTION(#OPERATOR, OPERATOR lhs)

#define BINARY_OPERATOR(OPERATOR) BINARY_FUNCTION(#OPERATOR, lhs OPERATOR rhs)

template<typename T, typename U>
    requires requires(T lhs, U rhs) { lhs / rhs; }
tl::expected<json, eval_error> safe_div(T lhs, U rhs) {
    using return_type = decltype(lhs / rhs);
    if constexpr (std::is_integral_v<return_type>) {
        if (rhs == 0) {
            return tl::unexpected(eval_error("division by zero"));
        }
    }

    return lhs / rhs;
}

function_registry jsonexpr::default_functions() {
    function_registry freg;
    freg["=="][2]    = BINARY_OPERATOR(==);
    freg["!="][2]    = BINARY_OPERATOR(!=);
    freg["!"][1]     = UNARY_OPERATOR(!);
    freg[">"][2]     = BINARY_OPERATOR(>);
    freg[">="][2]    = BINARY_OPERATOR(>=);
    freg["<"][2]     = BINARY_OPERATOR(<);
    freg["<="][2]    = BINARY_OPERATOR(<=);
    freg["&&"][2]    = BINARY_OPERATOR(&&);
    freg["||"][2]    = BINARY_OPERATOR(||);
    freg["/"][2]     = BINARY_FUNCTION("/", safe_div(lhs, rhs));
    freg["*"][2]     = BINARY_OPERATOR(*);
    freg["+"][2]     = BINARY_OPERATOR(+);
    freg["+"][1]     = UNARY_OPERATOR(+);
    freg["-"][2]     = BINARY_OPERATOR(-);
    freg["-"][1]     = UNARY_OPERATOR(-);
    freg["%"][2]     = BINARY_OPERATOR(%);
    freg["**"][2]    = BINARY_FUNCTION("**", std::pow(lhs, rhs));
    freg["^"][2]     = BINARY_FUNCTION("^", std::pow(lhs, rhs));
    freg["[]"][2]    = BINARY_FUNCTION("[]", lhs.at(rhs));
    freg["min"][2]   = BINARY_FUNCTION("min", lhs <= rhs ? lhs : rhs);
    freg["max"][2]   = BINARY_FUNCTION("max", lhs >= rhs ? lhs : rhs);
    freg["abs"][1]   = UNARY_FUNCTION("abs", std::abs(lhs));
    freg["sqrt"][2]  = UNARY_FUNCTION("abs", std::sqrt(lhs));
    freg["round"][2] = UNARY_FUNCTION("round", std::round(lhs));
    freg["floor"][2] = UNARY_FUNCTION("floor", std::floor(lhs));
    freg["ceil"][2]  = UNARY_FUNCTION("ceil", std::ceil(lhs));

    return freg;
}
