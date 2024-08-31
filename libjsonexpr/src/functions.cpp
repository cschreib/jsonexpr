#include "jsonexpr/functions.hpp"

#include "jsonexpr/eval.hpp"

#if JSONEXPR_USE_STD_FROM_CHARS
#    include <charconv>
#else
#    include <iomanip>
#    include <locale>
#    include <sstream>
#endif

#include <cmath>

using namespace jsonexpr;

namespace {
#define COMPARISON_OPERATOR(NAME, OPERATOR)                                                        \
    template<typename T, typename U>                                                               \
    basic_function_result safe_##NAME(const T& lhs, const U& rhs) {                                \
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
    requires(std::is_same_v<T, std::nullptr_t> || std::is_same_v<U, std::nullptr_t>)
basic_function_result safe_eq(const T&, const U&) {
    return std::is_same_v<T, U>;
}

template<typename T, typename U>
    requires(std::is_same_v<T, std::nullptr_t> || std::is_same_v<U, std::nullptr_t>)
basic_function_result safe_ne(const T&, const U&) {
    return !std::is_same_v<T, U>;
}

template<typename T, typename U>
basic_function_result safe_min(const T& lhs, const U& rhs) {
    if constexpr (std::is_floating_point_v<T> != std::is_floating_point_v<U>) {
        const json::number_float_t lhs_float = static_cast<json::number_float_t>(lhs);
        const json::number_float_t rhs_float = static_cast<json::number_float_t>(rhs);
        return lhs_float <= rhs_float ? lhs_float : rhs_float;
    } else {
        return lhs <= rhs ? lhs : rhs;
    }
}

template<typename T, typename U>
basic_function_result safe_max(const T& lhs, const U& rhs) {
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
    basic_function_result safe_##NAME(const T& lhs, const U& rhs) {                                \
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

template<typename T>
basic_function_result safe_unary_plus(const T& lhs) {
    return lhs;
}

template<typename T>
basic_function_result safe_unary_minus(const T& lhs) {
    return -lhs;
}

template<typename T, typename U>
basic_function_result safe_div(const T& lhs, const U& rhs) {
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
basic_function_result safe_mod(const T& lhs, const U& rhs) {
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
    basic_function_result safe_##NAME(const T& lhs) {                                              \
        return static_cast<RETURN>(FUNC(static_cast<json::number_float_t>(lhs)));                  \
    }

UNARY_MATH_FUNCTION(floor, std::floor, json::number_integer_t)
UNARY_MATH_FUNCTION(ceil, std::ceil, json::number_integer_t)
UNARY_MATH_FUNCTION(round, std::round, json::number_integer_t)
UNARY_MATH_FUNCTION(sqrt, std::sqrt, json::number_float_t)
UNARY_MATH_FUNCTION(abs, std::abs, json::number_float_t)

template<typename T, typename U>
basic_function_result safe_pow(const T& lhs, const U& rhs) {
    return std::pow(static_cast<json::number_float_t>(lhs), static_cast<json::number_float_t>(rhs));
}

std::size_t normalize_index(json::number_integer_t i, std::size_t size) {
    return static_cast<std::size_t>(i < 0 ? i + static_cast<json::number_integer_t>(size) : i);
}

template<typename T, typename U>
    requires(std::is_same_v<T, json::string_t> || std::is_same_v<T, json::array_t>)
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
    requires(std::is_same_v<T, json::string_t> || std::is_same_v<T, json::array_t>)
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

template<bool Expected, typename T, std::same_as<json::array_t> U>
basic_function_result safe_contains(const T& lhs, const U& rhs) {
    return (std::find(rhs.begin(), rhs.end(), lhs) != rhs.end()) == Expected;
}

template<bool Expected, std::same_as<std::string> T, std::same_as<json> U>
basic_function_result safe_contains(const T& lhs, const U& rhs) {
    return rhs.contains(lhs) == Expected;
}

template<bool Expected, std::same_as<std::string> T, std::same_as<std::string> U>
basic_function_result safe_contains(const T& lhs, const U& rhs) {
    return (rhs.find(lhs) != rhs.npos) == Expected;
}

template<typename T>
basic_function_result safe_len(const T& lhs) {
    return static_cast<json::number_integer_t>(lhs.size());
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
    json::number_integer_t value = 0;

#if JSONEXPR_USE_STD_FROM_CHARS
    const auto* begin = lhs.data();
    const auto* end   = lhs.data() + lhs.size();

    // Ignore leading '+' sign, not supported by std::from_chars.
    if (begin != end && *begin == '+') {
        ++begin;
    }

    auto [last, error_code] = std::from_chars(begin, end, value);
    if (error_code != std::errc{} || last != end) {
        return unexpected(std::string("could not convert string '" + lhs + "' to int"));
    }
#else
    std::istringstream stream(lhs);
    stream.imbue(std::locale::classic());

    if (!(stream >> std::noskipws >> value) || !stream.eof()) {
        return unexpected(std::string("could not convert string '" + lhs + "' to int"));
    }
#endif

    return value;
}

template<typename T>
    requires std::is_arithmetic_v<T>
basic_function_result safe_cast_float(const T& lhs) {
    return static_cast<json::number_float_t>(lhs);
}

template<std::same_as<json::string_t> T>
basic_function_result safe_cast_float(const T& lhs) {
    json::number_float_t value = 0;

#if JSONEXPR_USE_STD_FROM_CHARS
    const auto* begin = lhs.data();
    const auto* end   = lhs.data() + lhs.size();

    // Ignore leading '+' sign, not supported by std::from_chars.
    if (begin != end && *begin == '+') {
        ++begin;
    }

    auto [last, error_code] = std::from_chars(begin, end, value);
    if (error_code != std::errc{} || last != end) {
        return unexpected(std::string("could not convert string '" + lhs + "' to float"));
    }
#else
    std::istringstream stream(lhs);
    stream.imbue(std::locale::classic());

    if (!(stream >> std::noskipws >> value) || !stream.eof()) {
        return unexpected(std::string("could not convert string '" + lhs + "' to float"));
    }
#endif

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
    if (lhs == "true") {
        return true;
    } else if (lhs == "false") {
        return false;
    } else {
        return unexpected(std::string("could not convert string '" + lhs + "' to bool"));
    }
}

function_result safe_cast_string(
    std::span<const ast::node> args, const variable_registry& vars, const function_registry& funs) {
    if (args.size() != 1u) {
        return unexpected(error{
            .message =
                "function takes 1 arguments, but " + std::to_string(args.size()) + " provided"});
    }

    const auto eval_result = evaluate(args[0], vars, funs);
    if (!eval_result.has_value()) {
        return unexpected(eval_result.error());
    }

    if (eval_result.value().is_string()) {
        return eval_result.value();
    } else {
        return eval_result.value().dump();
    }
}

expected<bool, error> evaluate_as_bool(
    const ast::node& node, const variable_registry& vars, const function_registry& funs) {
    const auto eval_result = evaluate(node, vars, funs);
    if (!eval_result.has_value()) {
        return unexpected(eval_result.error());
    }

    if (eval_result.value().is_boolean()) {
        return eval_result.value().get<json::boolean_t>();
    } else {
        return unexpected(node_error(
            node, std::string("expected boolean, got ") +
                      std::string(get_dynamic_type_name(eval_result.value()))));
    }
}

function_result safe_not(
    std::span<const ast::node> args, const variable_registry& vars, const function_registry& funs) {
    if (args.size() != 1u) {
        return unexpected(error{
            .message =
                "function takes 1 argument, but " + std::to_string(args.size()) + " provided"});
    }

    const auto lhs = evaluate_as_bool(args[0], vars, funs);
    if (!lhs.has_value()) {
        return unexpected(lhs.error());
    }

    return !lhs.value();
}

function_result safe_and(
    std::span<const ast::node> args, const variable_registry& vars, const function_registry& funs) {
    if (args.size() != 2u) {
        return unexpected(error{
            .message =
                "function takes 2 arguments, but " + std::to_string(args.size()) + " provided"});
    }

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
    if (args.size() != 2u) {
        return unexpected(error{
            .message =
                "function takes 2 arguments, but " + std::to_string(args.size()) + " provided"});
    }

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

void jsonexpr::impl::add_type(std::string& key, std::string_view type) {
    if (!key.empty()) {
        key += ",";
    }
    key += std::string(type);
}

void jsonexpr::register_ast_function(
    function_registry&                                                                   funcs,
    std::string_view                                                                     name,
    std::function<function_result(
        std::span<const ast::node>, const variable_registry&, const function_registry&)> func) {
    funcs[std::string{name}] = {std::move(func)};
}

function_registry jsonexpr::default_functions() {
    function_registry freg;
    register_ast_function(freg, "str", &safe_cast_string);

    // Boolean operators are more complex since they short-circuit (avoid evaluation).
    register_ast_function(freg, "not", &safe_not);
    register_ast_function(freg, "and", &safe_and);
    register_ast_function(freg, "or", &safe_or);

    register_function(freg, "+", &safe_unary_plus<json::number_integer_t>);
    register_function(freg, "+", &safe_unary_plus<json::number_float_t>);
    register_function(freg, "+", &safe_add<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "+", &safe_add<json::number_integer_t, json::number_float_t>);
    register_function(freg, "+", &safe_add<json::number_float_t, json::number_integer_t>);
    register_function(freg, "+", &safe_add<json::number_float_t, json::number_float_t>);
    register_function(freg, "+", &safe_add<json::string_t, json::string_t>);

    register_function(freg, "-", &safe_unary_minus<json::number_integer_t>);
    register_function(freg, "-", &safe_unary_minus<json::number_float_t>);
    register_function(freg, "-", &safe_sub<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "-", &safe_sub<json::number_integer_t, json::number_float_t>);
    register_function(freg, "-", &safe_sub<json::number_float_t, json::number_integer_t>);
    register_function(freg, "-", &safe_sub<json::number_float_t, json::number_float_t>);

    register_function(freg, "*", &safe_mul<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "*", &safe_mul<json::number_integer_t, json::number_float_t>);
    register_function(freg, "*", &safe_mul<json::number_float_t, json::number_integer_t>);
    register_function(freg, "*", &safe_mul<json::number_float_t, json::number_float_t>);

    register_function(freg, "/", &safe_div<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "/", &safe_div<json::number_integer_t, json::number_float_t>);
    register_function(freg, "/", &safe_div<json::number_float_t, json::number_integer_t>);
    register_function(freg, "/", &safe_div<json::number_float_t, json::number_float_t>);

    register_function(freg, "%", &safe_mod<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "%", &safe_mod<json::number_integer_t, json::number_float_t>);
    register_function(freg, "%", &safe_mod<json::number_float_t, json::number_integer_t>);
    register_function(freg, "%", &safe_mod<json::number_float_t, json::number_float_t>);

    register_function(freg, "**", &safe_pow<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "**", &safe_pow<json::number_integer_t, json::number_float_t>);
    register_function(freg, "**", &safe_pow<json::number_float_t, json::number_integer_t>);
    register_function(freg, "**", &safe_pow<json::number_float_t, json::number_float_t>);

    register_function(freg, "abs", &safe_abs<json::number_integer_t>);
    register_function(freg, "abs", &safe_abs<json::number_float_t>);

    register_function(freg, "sqrt", &safe_sqrt<json::number_integer_t>);
    register_function(freg, "sqrt", &safe_sqrt<json::number_float_t>);

    register_function(freg, "round", &safe_round<json::number_integer_t>);
    register_function(freg, "round", &safe_round<json::number_float_t>);

    register_function(freg, "floor", &safe_floor<json::number_integer_t>);
    register_function(freg, "floor", &safe_floor<json::number_float_t>);

    register_function(freg, "ceil", &safe_ceil<json::number_integer_t>);
    register_function(freg, "ceil", &safe_ceil<json::number_float_t>);

    register_function(freg, "len", &safe_len<json::string_t>);
    register_function(freg, "len", &safe_len<json::array_t>);
    register_function(freg, "len", &safe_len<json>);

    register_function(freg, "min", &safe_min<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "min", &safe_min<json::number_integer_t, json::number_float_t>);
    register_function(freg, "min", &safe_min<json::number_float_t, json::number_integer_t>);
    register_function(freg, "min", &safe_min<json::number_float_t, json::number_float_t>);
    register_function(freg, "min", &safe_min<json::string_t, json::string_t>);

    register_function(freg, "max", &safe_max<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "max", &safe_max<json::number_integer_t, json::number_float_t>);
    register_function(freg, "max", &safe_max<json::number_float_t, json::number_integer_t>);
    register_function(freg, "max", &safe_max<json::number_float_t, json::number_float_t>);
    register_function(freg, "max", &safe_max<json::string_t, json::string_t>);

    register_function(freg, "int", &safe_cast_int<json::number_integer_t>);
    register_function(freg, "int", &safe_cast_int<json::number_float_t>);
    register_function(freg, "int", &safe_cast_int<json::boolean_t>);
    register_function(freg, "int", &safe_cast_int<json::string_t>);

    register_function(freg, "float", &safe_cast_float<json::number_integer_t>);
    register_function(freg, "float", &safe_cast_float<json::number_float_t>);
    register_function(freg, "float", &safe_cast_float<json::boolean_t>);
    register_function(freg, "float", &safe_cast_float<json::string_t>);

    register_function(freg, "bool", &safe_cast_bool<json::number_integer_t>);
    register_function(freg, "bool", &safe_cast_bool<json::number_float_t>);
    register_function(freg, "bool", &safe_cast_bool<json::boolean_t>);
    register_function(freg, "bool", &safe_cast_bool<json::string_t>);

    register_function(freg, "==", &safe_eq<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "==", &safe_eq<json::number_integer_t, json::number_float_t>);
    register_function(freg, "==", &safe_eq<json::number_integer_t, std::nullptr_t>);
    register_function(freg, "==", &safe_eq<json::number_float_t, json::number_integer_t>);
    register_function(freg, "==", &safe_eq<json::number_float_t, json::number_float_t>);
    register_function(freg, "==", &safe_eq<json::number_float_t, std::nullptr_t>);
    register_function(freg, "==", &safe_eq<json::boolean_t, json::boolean_t>);
    register_function(freg, "==", &safe_eq<json::boolean_t, std::nullptr_t>);
    register_function(freg, "==", &safe_eq<json::string_t, json::string_t>);
    register_function(freg, "==", &safe_eq<json::string_t, std::nullptr_t>);
    register_function(freg, "==", &safe_eq<json::array_t, json::array_t>);
    register_function(freg, "==", &safe_eq<json::array_t, std::nullptr_t>);
    register_function(freg, "==", &safe_eq<std::nullptr_t, json::number_integer_t>);
    register_function(freg, "==", &safe_eq<std::nullptr_t, json::number_float_t>);
    register_function(freg, "==", &safe_eq<std::nullptr_t, json::boolean_t>);
    register_function(freg, "==", &safe_eq<std::nullptr_t, json::string_t>);
    register_function(freg, "==", &safe_eq<std::nullptr_t, json::array_t>);
    register_function(freg, "==", &safe_eq<std::nullptr_t, std::nullptr_t>);
    register_function(freg, "==", &safe_eq<std::nullptr_t, json>);
    register_function(freg, "==", &safe_eq<json, std::nullptr_t>);
    register_function(freg, "==", &safe_eq<json, json>);

    register_function(freg, "!=", &safe_ne<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "!=", &safe_ne<json::number_integer_t, json::number_float_t>);
    register_function(freg, "!=", &safe_ne<json::number_integer_t, std::nullptr_t>);
    register_function(freg, "!=", &safe_ne<json::number_float_t, json::number_integer_t>);
    register_function(freg, "!=", &safe_ne<json::number_float_t, json::number_float_t>);
    register_function(freg, "!=", &safe_ne<json::number_float_t, std::nullptr_t>);
    register_function(freg, "!=", &safe_ne<json::boolean_t, json::boolean_t>);
    register_function(freg, "!=", &safe_ne<json::boolean_t, std::nullptr_t>);
    register_function(freg, "!=", &safe_ne<json::string_t, json::string_t>);
    register_function(freg, "!=", &safe_ne<json::string_t, std::nullptr_t>);
    register_function(freg, "!=", &safe_ne<json::array_t, json::array_t>);
    register_function(freg, "!=", &safe_ne<json::array_t, std::nullptr_t>);
    register_function(freg, "!=", &safe_ne<std::nullptr_t, json::number_integer_t>);
    register_function(freg, "!=", &safe_ne<std::nullptr_t, json::number_float_t>);
    register_function(freg, "!=", &safe_ne<std::nullptr_t, json::boolean_t>);
    register_function(freg, "!=", &safe_ne<std::nullptr_t, json::string_t>);
    register_function(freg, "!=", &safe_ne<std::nullptr_t, json::array_t>);
    register_function(freg, "!=", &safe_ne<std::nullptr_t, std::nullptr_t>);
    register_function(freg, "!=", &safe_ne<std::nullptr_t, json>);
    register_function(freg, "!=", &safe_ne<json, std::nullptr_t>);
    register_function(freg, "!=", &safe_ne<json, json>);

    register_function(freg, ">", &safe_gt<json::number_integer_t, json::number_integer_t>);
    register_function(freg, ">", &safe_gt<json::number_integer_t, json::number_float_t>);
    register_function(freg, ">", &safe_gt<json::number_float_t, json::number_integer_t>);
    register_function(freg, ">", &safe_gt<json::number_float_t, json::number_float_t>);
    register_function(freg, ">", &safe_gt<json::string_t, json::string_t>);

    register_function(freg, ">=", &safe_ge<json::number_integer_t, json::number_integer_t>);
    register_function(freg, ">=", &safe_ge<json::number_integer_t, json::number_float_t>);
    register_function(freg, ">=", &safe_ge<json::number_float_t, json::number_integer_t>);
    register_function(freg, ">=", &safe_ge<json::number_float_t, json::number_float_t>);
    register_function(freg, ">=", &safe_ge<json::string_t, json::string_t>);

    register_function(freg, "<", &safe_lt<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "<", &safe_lt<json::number_integer_t, json::number_float_t>);
    register_function(freg, "<", &safe_lt<json::number_float_t, json::number_integer_t>);
    register_function(freg, "<", &safe_lt<json::number_float_t, json::number_float_t>);
    register_function(freg, "<", &safe_lt<json::string_t, json::string_t>);

    register_function(freg, "<=", &safe_le<json::number_integer_t, json::number_integer_t>);
    register_function(freg, "<=", &safe_le<json::number_integer_t, json::number_float_t>);
    register_function(freg, "<=", &safe_le<json::number_float_t, json::number_integer_t>);
    register_function(freg, "<=", &safe_le<json::number_float_t, json::number_float_t>);
    register_function(freg, "<=", &safe_le<json::string_t, json::string_t>);

    register_function(freg, "[]", &safe_access<json::string_t, json::number_integer_t>);
    register_function(freg, "[]", &safe_access<json::array_t, json::number_integer_t>);
    register_function(freg, "[]", &safe_access<json, json::string_t>);

    register_function(freg, "[:]", &safe_range_access<json::string_t, json::number_integer_t>);
    register_function(freg, "[:]", &safe_range_access<json::array_t, json::number_integer_t>);

    register_function(freg, "in", &safe_contains<true, json::number_integer_t, json::array_t>);
    register_function(freg, "in", &safe_contains<true, json::number_float_t, json::array_t>);
    register_function(freg, "in", &safe_contains<true, json::boolean_t, json::array_t>);
    register_function(freg, "in", &safe_contains<true, json::string_t, json::string_t>);
    register_function(freg, "in", &safe_contains<true, json::string_t, json::array_t>);
    register_function(freg, "in", &safe_contains<true, json::string_t, json>);
    register_function(freg, "in", &safe_contains<true, json::array_t, json::array_t>);
    register_function(freg, "in", &safe_contains<true, std::nullptr_t, json::array_t>);
    register_function(freg, "in", &safe_contains<true, json, json::array_t>);

    register_function(freg, "not in", &safe_contains<false, json::number_integer_t, json::array_t>);
    register_function(freg, "not in", &safe_contains<false, json::number_float_t, json::array_t>);
    register_function(freg, "not in", &safe_contains<false, json::boolean_t, json::array_t>);
    register_function(freg, "not in", &safe_contains<false, json::string_t, json::string_t>);
    register_function(freg, "not in", &safe_contains<false, json::string_t, json::array_t>);
    register_function(freg, "not in", &safe_contains<false, json::string_t, json>);
    register_function(freg, "not in", &safe_contains<false, json::array_t, json::array_t>);
    register_function(freg, "not in", &safe_contains<false, std::nullptr_t, json::array_t>);
    register_function(freg, "not in", &safe_contains<false, json, json::array_t>);

    return freg;
}
