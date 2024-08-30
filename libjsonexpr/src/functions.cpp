#include "jsonexpr/functions.hpp"

#include "jsonexpr/eval.hpp"

#include <charconv>
#include <cmath>

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

namespace {
// We define our operators based on what is allowed in C++. However, C++ is not always sane,
// for example when allowing mixed operations with booleans (true < 1.0).
// Hence we define our own traits to identify acceptable types in operations.
template<typename T>
constexpr bool is_arithmetic_not_bool =
    std::is_arithmetic_v<T> && !std::is_same_v<T, json::boolean_t>;

template<typename T, typename U>
constexpr bool is_safe_to_compare =
    ((std::is_same_v<T, U> && !std::is_same_v<T, json::boolean_t> &&
      !std::is_same_v<T, json::array_t> && !std::is_same_v<T, json> &&
      !std::is_same_v<T, std::nullptr_t>) ||
     (is_arithmetic_not_bool<T> && is_arithmetic_not_bool<U>)) &&
    requires(T lhs, U rhs) { lhs <= rhs; };

template<typename T, typename U>
constexpr bool is_safe_for_maths = is_arithmetic_not_bool<T> && is_arithmetic_not_bool<U>;

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

#define TRINARY_FUNCTION(NAME, EXPR)                                                               \
    register_function(freg, NAME, 3, [](const json& args) -> basic_function_result {               \
        return std::visit(                                                                         \
            [](const auto& first, const auto& second,                                              \
               const auto& third) -> basic_function_result {                                       \
                if constexpr (requires { EXPR; }) {                                                \
                    return EXPR;                                                                   \
                } else {                                                                           \
                    return unexpected(                                                             \
                        std::string("incompatible types for '" NAME "', got ") +                   \
                        std::string(get_type_name(first)) + ", " +                                 \
                        std::string(get_type_name(second)) + ", and " +                            \
                        std::string(get_type_name(third)));                                        \
                }                                                                                  \
            },                                                                                     \
            to_variant(args[0]), to_variant(args[1]), to_variant(args[2]));                        \
    })

#define COMPARISON_OPERATOR(NAME, OPERATOR)                                                        \
    template<typename T, typename U>                                                               \
        requires(is_safe_to_compare<T, U>) bool                                                    \
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

template<std::same_as<json::boolean_t> T>
bool safe_eq(T lhs, T rhs) {
    return lhs == rhs;
}

template<std::same_as<json::boolean_t> T>
bool safe_ne(T lhs, T rhs) {
    return lhs != rhs;
}

template<std::same_as<json::array_t> T>
bool safe_eq(const T& lhs, const T& rhs) {
    return lhs == rhs;
}

template<std::same_as<json::array_t> T>
bool safe_ne(const T& lhs, const T& rhs) {
    return lhs != rhs;
}

template<std::same_as<json> T>
bool safe_eq(const T& lhs, const T& rhs) {
    return lhs == rhs;
}

template<std::same_as<json> T>
bool safe_ne(const T& lhs, const T& rhs) {
    return lhs != rhs;
}

template<typename T, typename U>
    requires(std::is_same_v<T, std::nullptr_t> || std::is_same_v<U, std::nullptr_t>) bool
safe_eq(const T&, const U&) {
    return std::is_same_v<T, U>;
}

template<typename T, typename U>
    requires(std::is_same_v<T, std::nullptr_t> || std::is_same_v<U, std::nullptr_t>) bool
safe_ne(const T&, const U&) {
    return !std::is_same_v<T, U>;
}

template<typename T, typename U>
    requires(is_safe_to_compare<T, U>)
auto safe_min(const T& lhs, const U& rhs) {
    if constexpr (std::is_floating_point_v<T> != std::is_floating_point_v<U>) {
        const json::number_float_t lhs_float = static_cast<json::number_float_t>(lhs);
        const json::number_float_t rhs_float = static_cast<json::number_float_t>(rhs);
        return lhs_float <= rhs_float ? lhs_float : rhs_float;
    } else {
        return lhs <= rhs ? lhs : rhs;
    }
}

template<typename T, typename U>
    requires(is_safe_to_compare<T, U>)
auto safe_max(const T& lhs, const U& rhs) {
    if constexpr (std::is_floating_point_v<T> != std::is_floating_point_v<U>) {
        const json::number_float_t lhs_float = static_cast<json::number_float_t>(lhs);
        const json::number_float_t rhs_float = static_cast<json::number_float_t>(rhs);
        return lhs_float >= rhs_float ? lhs_float : rhs_float;
    } else {
        return lhs >= rhs ? lhs : rhs;
    }
}

#define MATHS_OPERATOR(NAME, OPERATOR)                                                             \
    template<typename T, typename U>                                                               \
        requires(is_safe_for_maths<T, U>)                                                          \
    auto safe_##NAME(const T& lhs, const U& rhs) {                                                 \
        if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {                \
            return static_cast<json::number_float_t>(lhs)                                          \
                OPERATOR static_cast<json::number_float_t>(rhs);                                   \
        } else {                                                                                   \
            return lhs OPERATOR rhs;                                                               \
        }                                                                                          \
    }

MATHS_OPERATOR(mul, *)
MATHS_OPERATOR(add, +)
MATHS_OPERATOR(sub, -)

template<std::same_as<json::string_t> T>
T safe_add(const T& lhs, const T& rhs) {
    return lhs + rhs;
}

template<typename T>
    requires(is_arithmetic_not_bool<T>)
T safe_unary_plus(T lhs) {
    return lhs;
}

template<typename T>
    requires(is_arithmetic_not_bool<T>)
T safe_unary_minus(T lhs) {
    return -lhs;
}

template<typename T, typename U>
    requires(is_safe_for_maths<T, U>)
basic_function_result safe_div(T lhs, U rhs) {
    if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
        return static_cast<json::number_float_t>(lhs) / static_cast<json::number_float_t>(rhs);
    } else {
        if (rhs == 0) {
            return unexpected(std::string("integer division by zero"));
        }

        return lhs / rhs;
    }
}

template<typename T, typename U>
    requires(is_safe_for_maths<T, U>)
basic_function_result safe_mod(T lhs, U rhs) {
    if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
        return std::fmod(
            static_cast<json::number_float_t>(lhs), static_cast<json::number_float_t>(rhs));
    } else {
        if (rhs == 0) {
            return unexpected(std::string("integer modulo by zero"));
        }

        return lhs % rhs;
    }
}

#define UNARY_MATH_FUNCTION(NAME, FUNC, RETURN)                                                    \
    template<typename T>                                                                           \
        requires(is_arithmetic_not_bool<T>)                                                        \
    RETURN safe_##NAME(T lhs) {                                                                    \
        return static_cast<RETURN>(FUNC(static_cast<json::number_float_t>(lhs)));                  \
    }

UNARY_MATH_FUNCTION(floor, std::floor, json::number_integer_t)
UNARY_MATH_FUNCTION(ceil, std::ceil, json::number_integer_t)
UNARY_MATH_FUNCTION(round, std::round, json::number_integer_t)
UNARY_MATH_FUNCTION(sqrt, std::sqrt, json::number_float_t)
UNARY_MATH_FUNCTION(abs, std::abs, json::number_float_t)

template<typename T, typename U>
    requires(is_safe_for_maths<T, U>)
auto safe_pow(T lhs, U rhs) {
    return std::pow(static_cast<json::number_float_t>(lhs), static_cast<json::number_float_t>(rhs));
}

std::size_t normalize_index(json::number_integer_t i, std::size_t size) {
    return static_cast<std::size_t>(i < 0 ? i + static_cast<json::number_integer_t>(size) : i);
}

template<typename T, typename U>
    requires(
        std::is_integral_v<U> && !std::is_same_v<U, json::boolean_t> &&
        requires(const T& lhs, U rhs) { lhs[rhs]; })
basic_function_result safe_access(const T& lhs, const U& rhs) {
    const std::size_t unsigned_rhs = normalize_index(rhs, lhs.size());
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
}

template<std::same_as<json> T, std::same_as<json::string_t> U>
basic_function_result safe_access(const T& lhs, const U& rhs) {
    if (!lhs.contains(rhs)) {
        return unexpected(std::string("unknown field '") + rhs + "'");
    }

    return lhs[rhs];
}

template<typename T, typename U>
    requires(
        (std::is_same_v<T, json::string_t> || std::is_same_v<T, json::array_t>) &&
        std::is_integral_v<U> && !std::is_same_v<U, json::boolean_t> &&
        requires(const T& lhs, U rhs) { lhs[rhs]; })
basic_function_result safe_range_access(const T& object, const U& begin, const U& end) {
    const std::size_t unsigned_begin = normalize_index(begin, object.size());
    const std::size_t unsigned_end   = end != std::numeric_limits<json::number_integer_t>::max()
                                           ? normalize_index(end, object.size())
                                           : object.size();

    if (unsigned_end <= unsigned_begin) {
        return T{};
    }

    if (unsigned_begin >= object.size()) {
        return unexpected(
            std::string("out-of-bounds access for start of range ") + std::to_string(begin) +
            " in " + std::string(get_type_name(object)) + " of size " +
            std::to_string(object.size()));
    }

    if (unsigned_end > object.size()) {
        return unexpected(
            std::string("out-of-bounds access for end of range ") + std::to_string(end) + " in " +
            std::string(get_type_name(object)) + " of size " + std::to_string(object.size()));
    }

    if constexpr (std::is_same_v<T, json::string_t>) {
        return object.substr(unsigned_begin, unsigned_end - unsigned_begin);
    } else {
        return T(object.begin() + unsigned_begin, object.begin() + unsigned_end);
    }
}

template<typename T, std::same_as<json::array_t> U>
basic_function_result safe_contains(bool expected, const T& lhs, const U& rhs) {
    return (std::find(rhs.begin(), rhs.end(), lhs) != rhs.end()) == expected;
}

template<std::same_as<std::string> T, std::same_as<json> U>
basic_function_result safe_contains(bool expected, const T& lhs, const U& rhs) {
    if (!rhs.is_object()) {
        return unexpected(
            std::string("incompatible type for 'in', got " + std::string(get_type_name(rhs))));
    }

    return rhs.contains(lhs) == expected;
}

template<std::same_as<std::string> T, std::same_as<std::string> U>
basic_function_result safe_contains(bool expected, const T& lhs, const U& rhs) {
    return (rhs.find(lhs) != rhs.npos) == expected;
}

template<typename T>
    requires std::is_arithmetic_v<T>
basic_function_result safe_cast_int(const T& lhs) {
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(lhs) ||
            lhs < static_cast<json::number_float_t>(
                      std::numeric_limits<json::number_integer_t>::min()) ||
            lhs > static_cast<json::number_float_t>(
                      std::numeric_limits<json::number_integer_t>::max())) {
            return unexpected(
                std::string("could not convert float '" + std::to_string(lhs) + "' to int"));
        }
    }

    return static_cast<json::number_integer_t>(lhs);
}

template<std::same_as<json::string_t> T>
basic_function_result safe_cast_int(const T& lhs) {
    const auto* begin = lhs.data();
    const auto* end   = lhs.data() + lhs.size();
    if (begin != end && *begin == '+') {
        // Ignore leading '+' sign, not supported by std::from_chars.
        ++begin;
    }

    json::number_integer_t value = 0;
    auto [last, error_code]      = std::from_chars(begin, end, value);
    if (error_code != std::errc{} || last != end) {
        return unexpected(std::string("could not convert string '" + lhs + "' to int"));
    }

    return value;
}

template<typename T>
    requires std::is_arithmetic_v<T>
basic_function_result safe_cast_float(const T& lhs) {
    return static_cast<json::number_float_t>(lhs);
}

template<std::same_as<json::string_t> T>
basic_function_result safe_cast_float(const T& lhs) {
    const auto* begin = lhs.data();
    const auto* end   = lhs.data() + lhs.size();
    if (begin != end && *begin == '+') {
        // Ignore leading '+' sign, not supported by std::from_chars.
        ++begin;
    }

    json::number_float_t value = 0.0;
    auto [last, error_code]    = std::from_chars(begin, end, value);
    if (error_code != std::errc{} || last != end) {
        return unexpected(std::string("could not convert string '" + lhs + "' to float"));
    }

    return value;
}

template<typename T>
    requires std::is_arithmetic_v<T>
basic_function_result safe_cast_bool(const T& lhs) {
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(lhs)) {
            return unexpected(
                std::string("could not convert float '" + std::to_string(lhs) + "' to bool"));
        }
    }

    return static_cast<json::boolean_t>(lhs);
}

template<std::same_as<json::string_t> T>
basic_function_result safe_cast_bool(const T& lhs) {
    auto result = json::parse(lhs, nullptr, false);
    if (result.type() != json::value_t::boolean) {
        return unexpected(std::string("could not convert string '" + lhs + "' to bool"));
    }

    return result;
}

basic_function_result safe_cast_string(const json& args) {
    const json& lhs = args[0];
    if (lhs.is_string()) {
        return lhs;
    } else {
        return lhs.dump();
    }
}

expected<bool, error> evaluate_as_bool(
    const ast::node& node, const variable_registry& vars, const function_registry& funs) {
    const auto value_json = evaluate(node, vars, funs);
    if (!value_json.has_value()) {
        return unexpected(value_json.error());
    }

    const auto value = to_variant(value_json.value());
    if (std::holds_alternative<json::boolean_t>(value)) {
        return std::get<json::boolean_t>(value);
    } else {
        return unexpected(node_error(
            node, std::string("expected boolean, got ") +
                      std::string(get_type_name(value_json.value()))));
    }
}

function_result safe_not(
    std::span<const ast::node> args, const variable_registry& vars, const function_registry& funs) {
    const auto lhs = evaluate_as_bool(args[0], vars, funs);
    if (!lhs.has_value()) {
        return unexpected(lhs.error());
    }

    return !lhs.value();
}

function_result safe_and(
    std::span<const ast::node> args, const variable_registry& vars, const function_registry& funs) {
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
}

function_result safe_or(
    std::span<const ast::node> args, const variable_registry& vars, const function_registry& funs) {
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
}
} // namespace

function_registry jsonexpr::default_functions() {
    function_registry freg;
    BINARY_FUNCTION("==", safe_eq(lhs, rhs));
    BINARY_FUNCTION("!=", safe_ne(lhs, rhs));
    BINARY_FUNCTION(">", safe_gt(lhs, rhs));
    BINARY_FUNCTION(">=", safe_ge(lhs, rhs));
    BINARY_FUNCTION("<", safe_lt(lhs, rhs));
    BINARY_FUNCTION("<=", safe_le(lhs, rhs));
    BINARY_FUNCTION("/", safe_div(lhs, rhs));
    BINARY_FUNCTION("*", safe_mul(lhs, rhs));
    BINARY_FUNCTION("+", safe_add(lhs, rhs));
    UNARY_FUNCTION("+", safe_unary_plus(lhs));
    BINARY_FUNCTION("-", safe_sub(lhs, rhs));
    UNARY_FUNCTION("-", safe_unary_minus(lhs));
    BINARY_FUNCTION("%", safe_mod(lhs, rhs));
    BINARY_FUNCTION("**", safe_pow(lhs, rhs));
    BINARY_FUNCTION("[]", safe_access(lhs, rhs));
    TRINARY_FUNCTION("[:]", safe_range_access(first, second, third));
    BINARY_FUNCTION("min", safe_min(lhs, rhs));
    BINARY_FUNCTION("max", safe_max(lhs, rhs));
    UNARY_FUNCTION("abs", safe_abs(lhs));
    UNARY_FUNCTION("sqrt", safe_sqrt(lhs));
    UNARY_FUNCTION("round", safe_round(lhs));
    UNARY_FUNCTION("floor", safe_floor(lhs));
    UNARY_FUNCTION("ceil", safe_ceil(lhs));
    UNARY_FUNCTION("len", lhs.size());
    BINARY_FUNCTION("in", safe_contains(true, lhs, rhs));
    BINARY_FUNCTION("not in", safe_contains(false, lhs, rhs));
    UNARY_FUNCTION("int", safe_cast_int(lhs));
    UNARY_FUNCTION("float", safe_cast_float(lhs));
    UNARY_FUNCTION("bool", safe_cast_bool(lhs));
    register_function(freg, "str", 1, &safe_cast_string);

    // Boolean operators are more complex since they short-circuit (avoid evaluation).
    register_function(freg, "not", 1, &safe_not);
    register_function(freg, "and", 2, &safe_and);
    register_function(freg, "or", 2, &safe_or);

    return freg;
}
