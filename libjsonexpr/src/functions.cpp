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
            [](const auto& lhs) -> basic_function_result {                                         \
                if constexpr (requires { EXPR; }) {                                                \
                    return EXPR;                                                                   \
                } else {                                                                           \
                    return unexpected(                                                             \
                        std::string("incompatible type for '" NAME "', got ") +                    \
                        std::string(get_type_name(lhs)));                                          \
                }                                                                                  \
            },                                                                                     \
            to_variant(args[0]));                                                                  \
    })

#define BINARY_FUNCTION(NAME, EXPR)                                                                \
    register_function(freg, NAME, 2, [](const json& args) -> basic_function_result {               \
        return std::visit(                                                                         \
            [](const auto& lhs, const auto& rhs) -> basic_function_result {                        \
                if constexpr (requires { EXPR; }) {                                                \
                    return EXPR;                                                                   \
                } else {                                                                           \
                    return unexpected(                                                             \
                        std::string("incompatible types for '" NAME "', got ") +                   \
                        std::string(get_type_name(lhs)) + " and " +                                \
                        std::string(get_type_name(rhs)));                                          \
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
            return unexpected(std::string("integer division by zero"));
        }
    }

    return lhs / rhs;
}

template<typename T>
    requires requires(T lhs) { std::abs(lhs); }
basic_function_result safe_abs(T lhs) {
    if constexpr (std::is_signed_v<T>) {
        return std::abs(lhs);
    } else {
        return lhs;
    }
}

template<typename T>
constexpr bool is_arithmetic_not_bool =
    std::is_arithmetic_v<T> && !std::is_same_v<T, json::boolean_t>;

template<typename T, typename U>
constexpr bool is_safe_to_compare =
    std::is_same_v<T, U> || (is_arithmetic_not_bool<T> && is_arithmetic_not_bool<U>);

#define COMPARISON_OPERATOR(NAME, OPERATOR)                                                        \
    template<typename T, typename U>                                                               \
        requires(is_safe_to_compare<T, U> && requires(T lhs, U rhs) { lhs OPERATOR rhs; }) bool    \
    safe_##NAME(const T& lhs, const U& rhs) {                                                      \
        if constexpr (std::is_floating_point_v<T> != std::is_floating_point_v<U>) {                \
            return static_cast<json::number_float_t>(lhs)                                          \
                OPERATOR static_cast<json::number_float_t>(rhs);                                   \
        } else {                                                                                   \
            return lhs OPERATOR rhs;                                                               \
        }                                                                                          \
    }

COMPARISON_OPERATOR(eq, ==)
COMPARISON_OPERATOR(ne, !=)
COMPARISON_OPERATOR(lt, <)
COMPARISON_OPERATOR(le, <=)
COMPARISON_OPERATOR(gt, >)
COMPARISON_OPERATOR(ge, >=)

template<typename T, typename U>
    requires(
        (std::is_integral_v<U> || std::is_same_v<U, json::string_t>) &&
        requires(const T& lhs, U rhs) { lhs[rhs]; })
basic_function_result safe_access(const T& lhs, U rhs) {
    if constexpr (std::is_arithmetic_v<U>) {
        const std::size_t unsigned_rhs = [&] {
            if constexpr (std::is_signed_v<U>) {
                return static_cast<std::size_t>(rhs < 0 ? rhs + static_cast<U>(lhs.size()) : rhs);
            } else {
                return rhs;
            }
        }();

        if (unsigned_rhs >= lhs.size()) {
            return unexpected(
                std::string("out-of-bounds access at position ") + std::to_string(rhs) + " in " +
                std::string(get_type_name(lhs)) + " of size " + std::to_string(lhs.size()));
        }

        if constexpr (std::is_same_v<T, json::string_t>) {
            // If we just returned lhs[rhs], we would get the numerical value of the 'char'.
            // Return a new string instead with just that character, which will be more practical.
            return json::string_t(1, lhs[unsigned_rhs]);
        } else {
            return lhs[unsigned_rhs];
        }
    } else {
        if (!lhs.contains(rhs)) {
            return unexpected(std::string("unknown field '") + rhs + "'");
        }

        return lhs[rhs];
    }
}

expected<bool, error> evaluate_as_bool(
    const ast::node& node, const variable_registry& vars, const function_registry& funs) {
    const auto value_json = evaluate(node, vars, funs);
    if (!value_json.has_value()) {
        return unexpected(value_json.error());
    }

    return std::visit(
        [&](const auto& value) -> expected<bool, error> {
            if constexpr (requires { static_cast<bool>(value); }) {
                return static_cast<bool>(value);
            } else {
                return unexpected(node_error(
                    node, std::string("expected type convertible to bool, got ") +
                              std::string(get_type_name(value_json.value()))));
            }
        },
        to_variant(value_json.value()));
}
} // namespace

function_registry jsonexpr::default_functions() {
    function_registry freg;
    BINARY_FUNCTION("==", safe_eq(lhs, rhs));
    BINARY_FUNCTION("!=", safe_ne(lhs, rhs));
    UNARY_OPERATOR(!);
    BINARY_FUNCTION(">", safe_gt(lhs, rhs));
    BINARY_FUNCTION(">=", safe_ge(lhs, rhs));
    BINARY_FUNCTION("<", safe_lt(lhs, rhs));
    BINARY_FUNCTION("<=", safe_le(lhs, rhs));
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
    UNARY_FUNCTION("abs", safe_abs(lhs));
    UNARY_FUNCTION("sqrt", std::sqrt(lhs));
    UNARY_FUNCTION("round", std::round(lhs));
    UNARY_FUNCTION("floor", std::floor(lhs));
    UNARY_FUNCTION("ceil", std::ceil(lhs));
    UNARY_FUNCTION("size", lhs.size());

    // Boolean operators are more complex since they short-circuit (avoid evaluation).
    register_function(
        freg, "&&", 2,
        [](std::span<const ast::node> args, const variable_registry& vars,
           const function_registry& funs) -> function_result {
            // Evaluate left-hand-side first.
            const auto lhs = evaluate_as_bool(args[0], vars, funs);
            if (!lhs.has_value()) {
                return unexpected(lhs.error());
            }

            // Value is falsy, short-circuit.
            if (!lhs.value()) {
                return false;
            }

            // Value is truthy, evaluate right-hand-side.
            const auto rhs = evaluate_as_bool(args[1], vars, funs);
            if (!rhs.has_value()) {
                return unexpected(rhs.error());
            }

            return rhs.value();
        });

    register_function(
        freg, "||", 2,
        [](std::span<const ast::node> args, const variable_registry& vars,
           const function_registry& funs) -> function_result {
            // Evaluate left-hand-side first.
            const auto lhs = evaluate_as_bool(args[0], vars, funs);
            if (!lhs.has_value()) {
                return unexpected(lhs.error());
            }

            // Value is truthy, short-circuit.
            if (lhs.value()) {
                return true;
            }

            // Value is falsy, evaluate right-hand-side.
            const auto rhs = evaluate_as_bool(args[1], vars, funs);
            if (!rhs.has_value()) {
                return unexpected(rhs.error());
            }

            return rhs.value();
        });

    return freg;
}
