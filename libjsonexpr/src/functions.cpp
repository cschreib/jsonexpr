#include "jsonexpr/functions.hpp"

#include "jsonexpr/eval.hpp"

#include <cmath>
#include <sstream>

using namespace jsonexpr;

void jsonexpr::register_function(
    function_registry&                                funcs,
    std::string_view                                  name,
    std::size_t                                       arity,
    std::function<basic_function_result(const json&)> func) {
    funcs[name][arity] = {
        [func = std::move(func)](
            std::span<const ast::node> args, const variable_registry& vars,
            const function_registry& funs) -> function_result {
            json json_args(json::value_t::array);
            for (const auto& arg : args) {
                auto eval_arg = evaluate(arg, vars, funs);
                if (!eval_arg.has_value()) {
                    return unexpected(eval_arg.error());
                }
                json_args.push_back(std::move(eval_arg.value()));
            }

            auto result = func(json_args);
            if (result.has_value()) {
                return result.value();
            } else {
                return unexpected(error{.message = result.error()});
            }
        }};
}

void jsonexpr::register_function(
    function_registry&                                                                   funcs,
    std::string_view                                                                     name,
    std::size_t                                                                          arity,
    std::function<function_result(
        std::span<const ast::node>, const variable_registry&, const function_registry&)> func) {
    funcs[name][arity] = {std::move(func)};
}

#define UNARY_FUNCTION(NAME, EXPR)                                                                 \
    register_function(freg, NAME, 1, [](const json& args) -> basic_function_result {               \
        return std::visit(                                                                         \
            [&](const auto& lhs) -> basic_function_result {                                        \
                if constexpr (requires { EXPR; }) {                                                \
                    return EXPR;                                                                   \
                } else {                                                                           \
                    return unexpected(                                                             \
                        std::string("incompatible type for '" NAME "', got ") +                    \
                        std::string(get_type_name(args[0])));                                      \
                }                                                                                  \
            },                                                                                     \
            to_variant(args[0]));                                                                  \
    })

#define BINARY_FUNCTION(NAME, EXPR)                                                                \
    register_function(freg, NAME, 2, [](const json& args) -> basic_function_result {               \
        return std::visit(                                                                         \
            [&](const auto& lhs, const auto& rhs) -> basic_function_result {                       \
                if constexpr (requires { EXPR; }) {                                                \
                    return EXPR;                                                                   \
                } else {                                                                           \
                    return unexpected(                                                             \
                        std::string("incompatible types for '" NAME "', got ") +                   \
                        std::string(get_type_name(args[0])) + " and " +                            \
                        std::string(get_type_name(args[1])));                                      \
                }                                                                                  \
            },                                                                                     \
            to_variant(args[0]), to_variant(args[1]));                                             \
    })

#define UNARY_OPERATOR(OPERATOR) UNARY_FUNCTION(#OPERATOR, OPERATOR lhs)

#define BINARY_OPERATOR(OPERATOR) BINARY_FUNCTION(#OPERATOR, lhs OPERATOR rhs)

namespace {
template<typename T, typename U>
    requires requires(T lhs, U rhs) { lhs / rhs; }
basic_function_result safe_div(T lhs, U rhs) {
    using return_type = decltype(lhs / rhs);
    if constexpr (std::is_integral_v<return_type>) {
        if (rhs == 0) {
            return unexpected(std::string("division by zero"));
        }
    }

    return lhs / rhs;
}

template<typename T, typename U>
    requires(!std::same_as<U, json> && requires(const T& lhs, U rhs) { lhs[rhs]; })
basic_function_result safe_access(const T& lhs, U rhs) {
    if constexpr (std::is_arithmetic_v<U>) {
        if (rhs >= lhs.size()) {
            return unexpected(
                std::string("out-of-bounds access at position ") + std::to_string(rhs) +
                " in array of size " + std::to_string(lhs.size()));
        }
    } else {
        if (!lhs.contains(rhs)) {
            return unexpected(std::string("unknown field '") + rhs + "'");
        }
    }

    return lhs[rhs];
}
} // namespace

function_registry jsonexpr::default_functions() {
    function_registry freg;
    BINARY_OPERATOR(==);
    BINARY_OPERATOR(!=);
    UNARY_OPERATOR(!);
    BINARY_OPERATOR(>);
    BINARY_OPERATOR(>=);
    BINARY_OPERATOR(<);
    BINARY_OPERATOR(<=);
    BINARY_OPERATOR(&&);
    BINARY_OPERATOR(||);
    BINARY_FUNCTION("/", safe_div(lhs, rhs));
    BINARY_OPERATOR(*);
    BINARY_OPERATOR(+);
    UNARY_OPERATOR(+);
    BINARY_OPERATOR(-);
    UNARY_OPERATOR(-);
    BINARY_OPERATOR(%);
    BINARY_FUNCTION("**", std::pow(lhs, rhs));
    BINARY_FUNCTION("^", std::pow(lhs, rhs));
    BINARY_FUNCTION("[]", safe_access(lhs, rhs));
    BINARY_FUNCTION("min", lhs <= rhs ? lhs : rhs);
    BINARY_FUNCTION("max", lhs >= rhs ? lhs : rhs);
    UNARY_FUNCTION("abs", std::abs(lhs));
    UNARY_FUNCTION("sqrt", std::sqrt(lhs));
    UNARY_FUNCTION("round", std::round(lhs));
    UNARY_FUNCTION("floor", std::floor(lhs));
    UNARY_FUNCTION("ceil", std::ceil(lhs));
    UNARY_FUNCTION("size", lhs.size());

    return freg;
}
