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
        requires(!std::is_same_v<T, null_t> && !std::is_same_v<U, null_t>)                         \
    function_result safe_##NAME(const T& lhs, const U& rhs) {                                      \
        if constexpr (std::is_floating_point_v<T> != std::is_floating_point_v<U>) {                \
            return static_cast<number_float_t>(lhs) OPERATOR static_cast<number_float_t>(rhs);     \
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
    requires(std::is_same_v<T, null_t> || std::is_same_v<U, null_t>)
function_result safe_eq(const T&, const U&) {
    return std::is_same_v<T, U>;
}

template<typename T, typename U>
    requires(std::is_same_v<T, null_t> || std::is_same_v<U, null_t>)
function_result safe_ne(const T&, const U&) {
    return !std::is_same_v<T, U>;
}

template<typename T, typename U>
function_result safe_min(const T& lhs, const U& rhs) {
    if constexpr (std::is_floating_point_v<T> != std::is_floating_point_v<U>) {
        const number_float_t lhs_float = static_cast<number_float_t>(lhs);
        const number_float_t rhs_float = static_cast<number_float_t>(rhs);
        return lhs_float <= rhs_float ? lhs_float : rhs_float;
    } else {
        return lhs <= rhs ? lhs : rhs;
    }
}

template<typename T, typename U>
function_result safe_max(const T& lhs, const U& rhs) {
    if constexpr (std::is_floating_point_v<T> != std::is_floating_point_v<U>) {
        const number_float_t lhs_float = static_cast<number_float_t>(lhs);
        const number_float_t rhs_float = static_cast<number_float_t>(rhs);
        return lhs_float >= rhs_float ? lhs_float : rhs_float;
    } else {
        return lhs >= rhs ? lhs : rhs;
    }
}

#define MATHS_OPERATOR(NAME, OPERATOR)                                                             \
    template<typename T, typename U>                                                               \
    function_result safe_##NAME(const T& lhs, const U& rhs) {                                      \
        if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {                \
            return static_cast<number_float_t>(lhs) OPERATOR static_cast<number_float_t>(rhs);     \
        } else {                                                                                   \
            return lhs OPERATOR rhs;                                                               \
        }                                                                                          \
    }

MATHS_OPERATOR(mul, *)
MATHS_OPERATOR(add, +)
MATHS_OPERATOR(sub, -)

template<typename T>
function_result safe_unary_plus(const T& lhs) {
    return lhs;
}

template<typename T>
function_result safe_unary_minus(const T& lhs) {
    return -lhs;
}

template<typename T, typename U>
function_result safe_div(const T& lhs, const U& rhs) {
    if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
        return static_cast<number_float_t>(lhs) / static_cast<number_float_t>(rhs);
    } else {
        if (rhs == 0) {
            return unexpected(std::string("integer division by zero"));
        }

        return lhs / rhs;
    }
}

template<typename T, typename U>
function_result safe_mod(const T& lhs, const U& rhs) {
    if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
        return std::fmod(static_cast<number_float_t>(lhs), static_cast<number_float_t>(rhs));
    } else {
        if (rhs == 0) {
            return unexpected(std::string("integer modulo by zero"));
        }

        return lhs % rhs;
    }
}

template<typename T>
function_result safe_floor(T lhs) {
    return static_cast<number_integer_t>(std::floor(lhs));
}

template<typename T>
function_result safe_ceil(T lhs) {
    return static_cast<number_integer_t>(std::ceil(lhs));
}

template<typename T>
function_result safe_round(T lhs) {
    return static_cast<number_integer_t>(std::round(lhs));
}

template<typename T>
function_result safe_abs(T lhs) {
    return std::abs(lhs);
}

template<typename T>
function_result safe_sqrt(T lhs) {
    return std::sqrt(static_cast<number_float_t>(lhs));
}

template<typename T>
function_result identity(T lhs) {
    return lhs;
}

template<typename T, typename U>
function_result safe_pow(const T& lhs, const U& rhs) {
    return std::pow(static_cast<number_float_t>(lhs), static_cast<number_float_t>(rhs));
}

std::size_t normalize_index(number_integer_t i, std::size_t size) {
    return static_cast<std::size_t>(i < 0 ? i + static_cast<number_integer_t>(size) : i);
}

template<typename T, typename U>
    requires(std::is_same_v<T, string_t> || std::is_same_v<T, array_t>)
function_result safe_access(const T& lhs, const U& rhs) {
    const std::size_t unsigned_rhs = normalize_index(rhs, lhs.size());
    if (unsigned_rhs >= lhs.size()) {
        return unexpected(
            std::string("out-of-bounds access at position ") + std::to_string(rhs) + " in " +
            std::string(get_type_name(lhs)) + " of size " + std::to_string(lhs.size()));
    }

    if constexpr (std::is_same_v<T, string_t>) {
        // If we just returned lhs[rhs], we would get the numerical value of the 'char'.
        // Return a new string instead with just that character, which will be more practical.
        return string_t(1, lhs[unsigned_rhs]);
    } else {
        return lhs[unsigned_rhs];
    }
}

template<std::same_as<object_t> T, std::same_as<string_t> U>
function_result safe_access(const T& lhs, const U& rhs) {
    const auto iter = lhs.find(rhs);
    if (iter == lhs.end()) {
        return unexpected(std::string("unknown field '") + rhs + "'");
    }

    return iter->second;
}

template<typename T, typename U>
    requires(std::is_same_v<T, string_t> || std::is_same_v<T, array_t>)
function_result safe_range_access(const T& object, const U& begin, const U& end) {
    const std::size_t unsigned_begin = normalize_index(begin, object.size());
    const std::size_t unsigned_end   = end != std::numeric_limits<number_integer_t>::max()
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

    if constexpr (std::is_same_v<T, string_t>) {
        return object.substr(unsigned_begin, unsigned_end - unsigned_begin);
    } else {
        return T(object.begin() + unsigned_begin, object.begin() + unsigned_end);
    }
}

template<bool Expected, typename T, std::same_as<array_t> U>
function_result safe_contains(const T& lhs, const U& rhs) {
    return (std::find(rhs.begin(), rhs.end(), lhs) != rhs.end()) == Expected;
}

template<bool Expected, std::same_as<std::string> T, std::same_as<object_t> U>
function_result safe_contains(const T& lhs, const U& rhs) {
    return (rhs.find(lhs) != rhs.end()) == Expected;
}

template<bool Expected, std::same_as<std::string> T, std::same_as<std::string> U>
function_result safe_contains(const T& lhs, const U& rhs) {
    return (rhs.find(lhs) != rhs.npos) == Expected;
}

template<typename T>
function_result safe_len(const T& lhs) {
    return static_cast<number_integer_t>(lhs.size());
}

template<typename T>
    requires std::is_arithmetic_v<T>
function_result safe_cast_int(const T& lhs) {
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(lhs) ||
            lhs < static_cast<number_float_t>(std::numeric_limits<number_integer_t>::min()) ||
            lhs > static_cast<number_float_t>(std::numeric_limits<number_integer_t>::max())) {
            return unexpected(
                std::string("could not convert float '" + std::to_string(lhs) + "' to int"));
        }
    }

    return static_cast<number_integer_t>(lhs);
}

template<std::same_as<string_t> T>
function_result safe_cast_int(const T& lhs) {
    number_integer_t value = 0;

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
function_result safe_cast_float(const T& lhs) {
    return static_cast<number_float_t>(lhs);
}

template<std::same_as<string_t> T>
function_result safe_cast_float(const T& lhs) {
    number_float_t value = 0;

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
function_result safe_cast_bool(const T& lhs) {
    if constexpr (std::is_floating_point_v<T>) {
        if (!std::isfinite(lhs)) {
            return unexpected(
                std::string("could not convert float '" + std::to_string(lhs) + "' to bool"));
        }
    }

    return static_cast<boolean_t>(lhs);
}

template<std::same_as<string_t> T>
function_result safe_cast_bool(const T& lhs) {
    if (lhs == "true") {
        return true;
    } else if (lhs == "false") {
        return false;
    } else {
        return unexpected(std::string("could not convert string '" + lhs + "' to bool"));
    }
}

function_result safe_cast_string(const json& value) {
    if (value.is_string()) {
        return value;
    } else {
        return value.dump();
    }
}

expected<bool, error> evaluate_as_bool(
    const ast::node& node, const variable_registry& vars, const function_registry& funs) {
    const auto eval_result = evaluate(node, vars, funs);
    if (!eval_result.has_value()) {
        return unexpected(eval_result.error());
    }

    if (eval_result.value().is_boolean()) {
        return eval_result.value().get<boolean_t>();
    } else {
        return unexpected(node_error(
            node, std::string("expected boolean, got ") +
                      std::string(get_dynamic_type_name(eval_result.value()))));
    }
}

ast_function_result safe_not(
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

ast_function_result safe_and(
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

ast_function_result safe_or(
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
    std::function<ast_function_result(
        std::span<const ast::node>, const variable_registry&, const function_registry&)> func) {
    funcs[std::string{name}] = {std::move(func)};
}

function_registry jsonexpr::default_functions() {
    function_registry freg;

    // Boolean operators are more complex since they short-circuit (avoid evaluation).
    register_ast_function(freg, "not", &safe_not);
    register_ast_function(freg, "and", &safe_and);
    register_ast_function(freg, "or", &safe_or);

    register_function(freg, "+", &safe_unary_plus<number_integer_t>);
    register_function(freg, "+", &safe_unary_plus<number_float_t>);
    register_function(freg, "+", &safe_add<number_integer_t, number_integer_t>);
    register_function(freg, "+", &safe_add<number_integer_t, number_float_t>);
    register_function(freg, "+", &safe_add<number_float_t, number_integer_t>);
    register_function(freg, "+", &safe_add<number_float_t, number_float_t>);
    register_function(freg, "+", &safe_add<string_t, string_t>);

    register_function(freg, "-", &safe_unary_minus<number_integer_t>);
    register_function(freg, "-", &safe_unary_minus<number_float_t>);
    register_function(freg, "-", &safe_sub<number_integer_t, number_integer_t>);
    register_function(freg, "-", &safe_sub<number_integer_t, number_float_t>);
    register_function(freg, "-", &safe_sub<number_float_t, number_integer_t>);
    register_function(freg, "-", &safe_sub<number_float_t, number_float_t>);

    register_function(freg, "*", &safe_mul<number_integer_t, number_integer_t>);
    register_function(freg, "*", &safe_mul<number_integer_t, number_float_t>);
    register_function(freg, "*", &safe_mul<number_float_t, number_integer_t>);
    register_function(freg, "*", &safe_mul<number_float_t, number_float_t>);

    register_function(freg, "/", &safe_div<number_integer_t, number_integer_t>);
    register_function(freg, "/", &safe_div<number_integer_t, number_float_t>);
    register_function(freg, "/", &safe_div<number_float_t, number_integer_t>);
    register_function(freg, "/", &safe_div<number_float_t, number_float_t>);

    register_function(freg, "%", &safe_mod<number_integer_t, number_integer_t>);
    register_function(freg, "%", &safe_mod<number_integer_t, number_float_t>);
    register_function(freg, "%", &safe_mod<number_float_t, number_integer_t>);
    register_function(freg, "%", &safe_mod<number_float_t, number_float_t>);

    register_function(freg, "**", &safe_pow<number_integer_t, number_integer_t>);
    register_function(freg, "**", &safe_pow<number_integer_t, number_float_t>);
    register_function(freg, "**", &safe_pow<number_float_t, number_integer_t>);
    register_function(freg, "**", &safe_pow<number_float_t, number_float_t>);

    register_function(freg, "==", &safe_eq<number_integer_t, number_integer_t>);
    register_function(freg, "==", &safe_eq<number_integer_t, number_float_t>);
    register_function(freg, "==", &safe_eq<number_integer_t, null_t>);
    register_function(freg, "==", &safe_eq<number_float_t, number_integer_t>);
    register_function(freg, "==", &safe_eq<number_float_t, number_float_t>);
    register_function(freg, "==", &safe_eq<number_float_t, null_t>);
    register_function(freg, "==", &safe_eq<boolean_t, boolean_t>);
    register_function(freg, "==", &safe_eq<boolean_t, null_t>);
    register_function(freg, "==", &safe_eq<string_t, string_t>);
    register_function(freg, "==", &safe_eq<string_t, null_t>);
    register_function(freg, "==", &safe_eq<array_t, array_t>);
    register_function(freg, "==", &safe_eq<array_t, null_t>);
    register_function(freg, "==", &safe_eq<null_t, number_integer_t>);
    register_function(freg, "==", &safe_eq<null_t, number_float_t>);
    register_function(freg, "==", &safe_eq<null_t, boolean_t>);
    register_function(freg, "==", &safe_eq<null_t, string_t>);
    register_function(freg, "==", &safe_eq<null_t, array_t>);
    register_function(freg, "==", &safe_eq<null_t, null_t>);
    register_function(freg, "==", &safe_eq<null_t, object_t>);
    register_function(freg, "==", &safe_eq<object_t, null_t>);
    register_function(freg, "==", &safe_eq<object_t, object_t>);

    register_function(freg, "!=", &safe_ne<number_integer_t, number_integer_t>);
    register_function(freg, "!=", &safe_ne<number_integer_t, number_float_t>);
    register_function(freg, "!=", &safe_ne<number_integer_t, null_t>);
    register_function(freg, "!=", &safe_ne<number_float_t, number_integer_t>);
    register_function(freg, "!=", &safe_ne<number_float_t, number_float_t>);
    register_function(freg, "!=", &safe_ne<number_float_t, null_t>);
    register_function(freg, "!=", &safe_ne<boolean_t, boolean_t>);
    register_function(freg, "!=", &safe_ne<boolean_t, null_t>);
    register_function(freg, "!=", &safe_ne<string_t, string_t>);
    register_function(freg, "!=", &safe_ne<string_t, null_t>);
    register_function(freg, "!=", &safe_ne<array_t, array_t>);
    register_function(freg, "!=", &safe_ne<array_t, null_t>);
    register_function(freg, "!=", &safe_ne<null_t, number_integer_t>);
    register_function(freg, "!=", &safe_ne<null_t, number_float_t>);
    register_function(freg, "!=", &safe_ne<null_t, boolean_t>);
    register_function(freg, "!=", &safe_ne<null_t, string_t>);
    register_function(freg, "!=", &safe_ne<null_t, array_t>);
    register_function(freg, "!=", &safe_ne<null_t, null_t>);
    register_function(freg, "!=", &safe_ne<null_t, object_t>);
    register_function(freg, "!=", &safe_ne<object_t, null_t>);
    register_function(freg, "!=", &safe_ne<object_t, object_t>);

    register_function(freg, ">", &safe_gt<number_integer_t, number_integer_t>);
    register_function(freg, ">", &safe_gt<number_integer_t, number_float_t>);
    register_function(freg, ">", &safe_gt<number_float_t, number_integer_t>);
    register_function(freg, ">", &safe_gt<number_float_t, number_float_t>);
    register_function(freg, ">", &safe_gt<string_t, string_t>);

    register_function(freg, ">=", &safe_ge<number_integer_t, number_integer_t>);
    register_function(freg, ">=", &safe_ge<number_integer_t, number_float_t>);
    register_function(freg, ">=", &safe_ge<number_float_t, number_integer_t>);
    register_function(freg, ">=", &safe_ge<number_float_t, number_float_t>);
    register_function(freg, ">=", &safe_ge<string_t, string_t>);

    register_function(freg, "<", &safe_lt<number_integer_t, number_integer_t>);
    register_function(freg, "<", &safe_lt<number_integer_t, number_float_t>);
    register_function(freg, "<", &safe_lt<number_float_t, number_integer_t>);
    register_function(freg, "<", &safe_lt<number_float_t, number_float_t>);
    register_function(freg, "<", &safe_lt<string_t, string_t>);

    register_function(freg, "<=", &safe_le<number_integer_t, number_integer_t>);
    register_function(freg, "<=", &safe_le<number_integer_t, number_float_t>);
    register_function(freg, "<=", &safe_le<number_float_t, number_integer_t>);
    register_function(freg, "<=", &safe_le<number_float_t, number_float_t>);
    register_function(freg, "<=", &safe_le<string_t, string_t>);

    register_function(freg, "[]", &safe_access<string_t, number_integer_t>);
    register_function(freg, "[]", &safe_access<array_t, number_integer_t>);
    register_function(freg, "[]", &safe_access<object_t, string_t>);

    register_function(freg, "[:]", &safe_range_access<string_t, number_integer_t>);
    register_function(freg, "[:]", &safe_range_access<array_t, number_integer_t>);

    register_function(freg, "in", &safe_contains<true, number_integer_t, array_t>);
    register_function(freg, "in", &safe_contains<true, number_float_t, array_t>);
    register_function(freg, "in", &safe_contains<true, boolean_t, array_t>);
    register_function(freg, "in", &safe_contains<true, string_t, string_t>);
    register_function(freg, "in", &safe_contains<true, string_t, array_t>);
    register_function(freg, "in", &safe_contains<true, string_t, object_t>);
    register_function(freg, "in", &safe_contains<true, array_t, array_t>);
    register_function(freg, "in", &safe_contains<true, null_t, array_t>);
    register_function(freg, "in", &safe_contains<true, object_t, array_t>);

    register_function(freg, "not in", &safe_contains<false, number_integer_t, array_t>);
    register_function(freg, "not in", &safe_contains<false, number_float_t, array_t>);
    register_function(freg, "not in", &safe_contains<false, boolean_t, array_t>);
    register_function(freg, "not in", &safe_contains<false, string_t, string_t>);
    register_function(freg, "not in", &safe_contains<false, string_t, array_t>);
    register_function(freg, "not in", &safe_contains<false, string_t, object_t>);
    register_function(freg, "not in", &safe_contains<false, array_t, array_t>);
    register_function(freg, "not in", &safe_contains<false, null_t, array_t>);
    register_function(freg, "not in", &safe_contains<false, object_t, array_t>);

    register_function(freg, "int", &safe_cast_int<number_integer_t>);
    register_function(freg, "int", &safe_cast_int<number_float_t>);
    register_function(freg, "int", &safe_cast_int<boolean_t>);
    register_function(freg, "int", &safe_cast_int<string_t>);

    register_function(freg, "float", &safe_cast_float<number_integer_t>);
    register_function(freg, "float", &safe_cast_float<number_float_t>);
    register_function(freg, "float", &safe_cast_float<boolean_t>);
    register_function(freg, "float", &safe_cast_float<string_t>);

    register_function(freg, "bool", &safe_cast_bool<number_integer_t>);
    register_function(freg, "bool", &safe_cast_bool<number_float_t>);
    register_function(freg, "bool", &safe_cast_bool<boolean_t>);
    register_function(freg, "bool", &safe_cast_bool<string_t>);

    register_function(freg, "str", &safe_cast_string);

    register_function(freg, "abs", &safe_abs<number_integer_t>);
    register_function(freg, "abs", &safe_abs<number_float_t>);

    register_function(freg, "sqrt", &safe_sqrt<number_integer_t>);
    register_function(freg, "sqrt", &safe_sqrt<number_float_t>);

    register_function(freg, "round", &identity<number_integer_t>);
    register_function(freg, "round", &safe_round<number_float_t>);

    register_function(freg, "floor", &identity<number_integer_t>);
    register_function(freg, "floor", &safe_floor<number_float_t>);

    register_function(freg, "ceil", &identity<number_integer_t>);
    register_function(freg, "ceil", &safe_ceil<number_float_t>);

    register_function(freg, "len", &safe_len<string_t>);
    register_function(freg, "len", &safe_len<array_t>);
    register_function(freg, "len", &safe_len<object_t>);

    register_function(freg, "min", &safe_min<number_integer_t, number_integer_t>);
    register_function(freg, "min", &safe_min<number_integer_t, number_float_t>);
    register_function(freg, "min", &safe_min<number_float_t, number_integer_t>);
    register_function(freg, "min", &safe_min<number_float_t, number_float_t>);
    register_function(freg, "min", &safe_min<string_t, string_t>);

    register_function(freg, "max", &safe_max<number_integer_t, number_integer_t>);
    register_function(freg, "max", &safe_max<number_integer_t, number_float_t>);
    register_function(freg, "max", &safe_max<number_float_t, number_integer_t>);
    register_function(freg, "max", &safe_max<number_float_t, number_float_t>);
    register_function(freg, "max", &safe_max<string_t, string_t>);

    return freg;
}
