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
    funcs[std::string{name}][arity] = {
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
    funcs[std::string{name}][arity] = {std::move(func)};
}

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
    if (!rhs.is_object()) {
        return unexpected(
            std::string("incompatible type for 'in', got " + std::string(get_type_name(rhs))));
    }

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

template<typename... Args>
using basic_function = basic_function_result (*)(const Args&...);

template<typename T>
struct type_arity : std::integral_constant<std::size_t, T::arity> {};

template<typename... Args>
struct type_arity<basic_function<Args...>> : std::integral_constant<std::size_t, 0u> {};

template<template<typename...> typename NextOverloadSet, typename... Args>
struct overload_set {
    NextOverloadSet<Args..., json::number_integer_t> arg_int    = {};
    NextOverloadSet<Args..., json::number_float_t>   arg_flt    = {};
    NextOverloadSet<Args..., json::boolean_t>        arg_bool   = {};
    NextOverloadSet<Args..., json::string_t>         arg_string = {};
    NextOverloadSet<Args..., json::array_t>          arg_array  = {};
    NextOverloadSet<Args..., std::nullptr_t>         arg_null   = {};
    NextOverloadSet<Args..., json>                   arg_object = {};

    static constexpr std::size_t arity = 1u + type_arity<NextOverloadSet<Args..., json>>::value;

    auto operator<=>(const overload_set&) const noexcept = default;
};

template<typename... Args>
using unary_overload_set_impl = overload_set<basic_function, Args...>;
using unary_overload_set      = unary_overload_set_impl<>;

template<typename... Args>
using binary_overload_set_impl = overload_set<unary_overload_set_impl, Args...>;
using binary_overload_set      = binary_overload_set_impl<>;

template<typename... Args>
using trinary_overload_set_impl = overload_set<binary_overload_set_impl, Args...>;
using trinary_overload_set      = trinary_overload_set_impl<>;

template<typename OverloadSet>
bool has_function(const OverloadSet& set) {
    return set != OverloadSet{};
}

template<typename OverloadSet, typename... Args>
    requires(type_arity<OverloadSet>::value == 0u)
function_result call(
    const OverloadSet& func,
    std::span<const ast::node>,
    const variable_registry&,
    const function_registry&,
    const Args&... args) {

    auto result = (*func)(args...);
    if (result.has_value()) {
        return std::move(result.value());
    } else {
        return unexpected(error{.message = result.error()});
    }
}

template<typename OverloadSet, typename... Args>
    requires(type_arity<OverloadSet>::value > 0u)
function_result call(
    const OverloadSet&         set,
    std::span<const ast::node> next_args,
    const variable_registry&   vars,
    const function_registry&   funs,
    const Args&... prev_args) {

    const auto eval_result = evaluate(next_args[0], vars, funs);
    if (!eval_result.has_value()) {
        return unexpected(eval_result.error());
    }

    const auto& arg = eval_result.value();

    switch (arg.type()) {
    case json::value_t::array:
        if (has_function(set.arg_array)) {
            return call(
                set.arg_array, next_args.subspan(1), vars, funs, prev_args...,
                arg.get_ref<const json::array_t&>());
        }
        break;
    case json::value_t::string: {
        if (has_function(set.arg_string)) {
            return call(
                set.arg_string, next_args.subspan(1), vars, funs, prev_args...,
                arg.get_ref<const json::string_t&>());
        }
        break;
    }
    case json::value_t::boolean: {
        if (has_function(set.arg_bool)) {
            return call(
                set.arg_bool, next_args.subspan(1), vars, funs, prev_args...,
                arg.get<json::boolean_t>());
        }
        break;
    }
    case json::value_t::number_unsigned: [[fallthrough]];
    case json::value_t::number_integer: {
        if (has_function(set.arg_int)) {
            return call(
                set.arg_int, next_args.subspan(1), vars, funs, prev_args...,
                arg.get<json::number_integer_t>());
        }
        break;
    }
    case json::value_t::number_float: {
        if (has_function(set.arg_flt)) {
            return call(
                set.arg_flt, next_args.subspan(1), vars, funs, prev_args...,
                arg.get<json::number_float_t>());
        }
        break;
    }
    case json::value_t::null: {
        if (has_function(set.arg_null)) {
            return call(set.arg_null, next_args.subspan(1), vars, funs, prev_args..., nullptr);
        }
        break;
    }
    default: {
        if (has_function(set.arg_object)) {
            return call(set.arg_object, next_args.subspan(1), vars, funs, prev_args..., arg);
        }
        break;
    }
    }

    return unexpected(node_error(
        next_args[0], std::string("incompatible type, got ") + std::string(get_type_name(arg))));
}

void register_function(
    function_registry& funcs, std::string_view name, unary_overload_set overload) {
    register_function(
        funcs, name, overload.arity,
        [overload = std::move(overload)](
            std::span<const ast::node> args, const variable_registry& vars,
            const function_registry& funs) -> function_result {
            return call(overload, args, vars, funs);
        });
}

void register_function(
    function_registry& funcs, std::string_view name, binary_overload_set overload) {
    register_function(
        funcs, name, overload.arity,
        [overload = std::move(overload)](
            std::span<const ast::node> args, const variable_registry& vars,
            const function_registry& funs) -> function_result {
            return call(overload, args, vars, funs);
        });
}

void register_function(
    function_registry& funcs, std::string_view name, trinary_overload_set overload) {
    register_function(
        funcs, name, overload.arity,
        [overload = std::move(overload)](
            std::span<const ast::node> args, const variable_registry& vars,
            const function_registry& funs) -> function_result {
            return call(overload, args, vars, funs);
        });
}

function_registry jsonexpr::default_functions() {
    function_registry freg;
    register_function(freg, "str", 1, &safe_cast_string);

    // Boolean operators are more complex since they short-circuit (avoid evaluation).
    register_function(freg, "not", 1, &safe_not);
    register_function(freg, "and", 2, &safe_and);
    register_function(freg, "or", 2, &safe_or);

    register_function(
        freg, "+",
        unary_overload_set{
            .arg_int = &safe_unary_plus<json::number_integer_t>,
            .arg_flt = &safe_unary_plus<json::number_float_t>});

    register_function(
        freg, "-",
        unary_overload_set{
            .arg_int = &safe_unary_minus<json::number_integer_t>,
            .arg_flt = &safe_unary_minus<json::number_float_t>});

    register_function(
        freg, "abs",
        unary_overload_set{
            .arg_int = &safe_abs<json::number_integer_t>,
            .arg_flt = &safe_abs<json::number_float_t>});

    register_function(
        freg, "sqrt",
        unary_overload_set{
            .arg_int = &safe_sqrt<json::number_integer_t>,
            .arg_flt = &safe_sqrt<json::number_float_t>});

    register_function(
        freg, "round",
        unary_overload_set{
            .arg_int = &safe_round<json::number_integer_t>,
            .arg_flt = &safe_round<json::number_float_t>});

    register_function(
        freg, "floor",
        unary_overload_set{
            .arg_int = &safe_floor<json::number_integer_t>,
            .arg_flt = &safe_floor<json::number_float_t>});

    register_function(
        freg, "ceil",
        unary_overload_set{
            .arg_int = &safe_ceil<json::number_integer_t>,
            .arg_flt = &safe_ceil<json::number_float_t>});

    register_function(
        freg, "len",
        unary_overload_set{
            .arg_string = &safe_len<json::string_t>,
            .arg_array  = &safe_len<json::array_t>,
            .arg_object = &safe_len<json>});

    register_function(
        freg, "min",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_min<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_min<json::number_integer_t, json::number_float_t>},
            .arg_flt =
                {.arg_int = &safe_min<json::number_float_t, json::number_integer_t>,
                 .arg_flt = &safe_min<json::number_float_t, json::number_float_t>},
            .arg_string = {.arg_string = &safe_min<json::string_t, json::string_t>}});

    register_function(
        freg, "max",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_max<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_max<json::number_integer_t, json::number_float_t>},
            .arg_flt =
                {.arg_int = &safe_max<json::number_float_t, json::number_integer_t>,
                 .arg_flt = &safe_max<json::number_float_t, json::number_float_t>},
            .arg_string = {.arg_string = &safe_max<json::string_t, json::string_t>}});

    register_function(
        freg, "int",
        unary_overload_set{
            .arg_int    = &safe_cast_int<json::number_integer_t>,
            .arg_flt    = &safe_cast_int<json::number_float_t>,
            .arg_bool   = &safe_cast_int<json::boolean_t>,
            .arg_string = &safe_cast_int<json::string_t>});

    register_function(
        freg, "float",
        unary_overload_set{
            .arg_int    = &safe_cast_float<json::number_integer_t>,
            .arg_flt    = &safe_cast_float<json::number_float_t>,
            .arg_bool   = &safe_cast_float<json::boolean_t>,
            .arg_string = &safe_cast_float<json::string_t>});

    register_function(
        freg, "bool",
        unary_overload_set{
            .arg_int    = &safe_cast_bool<json::number_integer_t>,
            .arg_flt    = &safe_cast_bool<json::number_float_t>,
            .arg_bool   = &safe_cast_bool<json::boolean_t>,
            .arg_string = &safe_cast_bool<json::string_t>});

    register_function(
        freg, "==",
        binary_overload_set{
            .arg_int =
                {.arg_int  = &safe_eq<json::number_integer_t, json::number_integer_t>,
                 .arg_flt  = &safe_eq<json::number_integer_t, json::number_float_t>,
                 .arg_null = &safe_eq<json::number_integer_t, std::nullptr_t>},
            .arg_flt =
                {.arg_int  = &safe_eq<json::number_float_t, json::number_integer_t>,
                 .arg_flt  = &safe_eq<json::number_float_t, json::number_float_t>,
                 .arg_null = &safe_eq<json::number_float_t, std::nullptr_t>},
            .arg_bool =
                {.arg_bool = &safe_eq<json::boolean_t, json::boolean_t>,
                 .arg_null = &safe_eq<json::boolean_t, std::nullptr_t>},
            .arg_string =
                {.arg_string = &safe_eq<json::string_t, json::string_t>,
                 .arg_null   = &safe_eq<json::string_t, std::nullptr_t>},
            .arg_array =
                {.arg_array = &safe_eq<json::array_t, json::array_t>,
                 .arg_null  = &safe_eq<json::array_t, std::nullptr_t>},
            .arg_null =
                {.arg_int    = &safe_eq<std::nullptr_t, json::number_integer_t>,
                 .arg_flt    = &safe_eq<std::nullptr_t, json::number_float_t>,
                 .arg_bool   = &safe_eq<std::nullptr_t, json::boolean_t>,
                 .arg_string = &safe_eq<std::nullptr_t, json::string_t>,
                 .arg_array  = &safe_eq<std::nullptr_t, json::array_t>,
                 .arg_null   = &safe_eq<std::nullptr_t, std::nullptr_t>,
                 .arg_object = &safe_eq<std::nullptr_t, json>},
            .arg_object = {
                .arg_null = &safe_eq<json, std::nullptr_t>, .arg_object = &safe_eq<json, json>}});

    register_function(
        freg, "!=",
        binary_overload_set{
            .arg_int =
                {.arg_int  = &safe_ne<json::number_integer_t, json::number_integer_t>,
                 .arg_flt  = &safe_ne<json::number_integer_t, json::number_float_t>,
                 .arg_null = &safe_ne<json::number_integer_t, std::nullptr_t>},
            .arg_flt =
                {.arg_int  = &safe_ne<json::number_float_t, json::number_integer_t>,
                 .arg_flt  = &safe_ne<json::number_float_t, json::number_float_t>,
                 .arg_null = &safe_ne<json::number_float_t, std::nullptr_t>},
            .arg_bool =
                {.arg_bool = &safe_ne<json::boolean_t, json::boolean_t>,
                 .arg_null = &safe_ne<json::boolean_t, std::nullptr_t>},
            .arg_string =
                {.arg_string = &safe_ne<json::string_t, json::string_t>,
                 .arg_null   = &safe_ne<json::string_t, std::nullptr_t>},
            .arg_array =
                {.arg_array = &safe_ne<json::array_t, json::array_t>,
                 .arg_null  = &safe_ne<json::array_t, std::nullptr_t>},
            .arg_null =
                {.arg_int    = &safe_ne<std::nullptr_t, json::number_integer_t>,
                 .arg_flt    = &safe_ne<std::nullptr_t, json::number_float_t>,
                 .arg_bool   = &safe_ne<std::nullptr_t, json::boolean_t>,
                 .arg_string = &safe_ne<std::nullptr_t, json::string_t>,
                 .arg_array  = &safe_ne<std::nullptr_t, json::array_t>,
                 .arg_null   = &safe_ne<std::nullptr_t, std::nullptr_t>,
                 .arg_object = &safe_ne<std::nullptr_t, json>},
            .arg_object = {
                .arg_null = &safe_ne<json, std::nullptr_t>, .arg_object = &safe_ne<json, json>}});

    register_function(
        freg, ">",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_gt<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_gt<json::number_integer_t, json::number_float_t>},
            .arg_flt =
                {.arg_int = &safe_gt<json::number_float_t, json::number_integer_t>,
                 .arg_flt = &safe_gt<json::number_float_t, json::number_float_t>},
            .arg_string = {.arg_string = &safe_gt<json::string_t, json::string_t>}});

    register_function(
        freg, ">=",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_ge<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_ge<json::number_integer_t, json::number_float_t>},
            .arg_flt =
                {.arg_int = &safe_ge<json::number_float_t, json::number_integer_t>,
                 .arg_flt = &safe_ge<json::number_float_t, json::number_float_t>},
            .arg_string = {.arg_string = &safe_ge<json::string_t, json::string_t>}});

    register_function(
        freg, "<",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_lt<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_lt<json::number_integer_t, json::number_float_t>},
            .arg_flt =
                {.arg_int = &safe_lt<json::number_float_t, json::number_integer_t>,
                 .arg_flt = &safe_lt<json::number_float_t, json::number_float_t>},
            .arg_string = {.arg_string = &safe_lt<json::string_t, json::string_t>}});

    register_function(
        freg, "<=",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_le<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_le<json::number_integer_t, json::number_float_t>},
            .arg_flt =
                {.arg_int = &safe_le<json::number_float_t, json::number_integer_t>,
                 .arg_flt = &safe_le<json::number_float_t, json::number_float_t>},
            .arg_string = {.arg_string = &safe_le<json::string_t, json::string_t>}});

    register_function(
        freg, "+",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_add<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_add<json::number_integer_t, json::number_float_t>},
            .arg_flt =
                {.arg_int = &safe_add<json::number_float_t, json::number_integer_t>,
                 .arg_flt = &safe_add<json::number_float_t, json::number_float_t>},
            .arg_string = {.arg_string = &safe_add<json::string_t, json::string_t>}});

    register_function(
        freg, "-",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_sub<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_sub<json::number_integer_t, json::number_float_t>},
            .arg_flt = {
                .arg_int = &safe_sub<json::number_float_t, json::number_integer_t>,
                .arg_flt = &safe_sub<json::number_float_t, json::number_float_t>}});

    register_function(
        freg, "*",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_mul<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_mul<json::number_integer_t, json::number_float_t>},
            .arg_flt = {
                .arg_int = &safe_mul<json::number_float_t, json::number_integer_t>,
                .arg_flt = &safe_mul<json::number_float_t, json::number_float_t>}});

    register_function(
        freg, "/",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_div<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_div<json::number_integer_t, json::number_float_t>},
            .arg_flt = {
                .arg_int = &safe_div<json::number_float_t, json::number_integer_t>,
                .arg_flt = &safe_div<json::number_float_t, json::number_float_t>}});

    register_function(
        freg, "%",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_mod<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_mod<json::number_integer_t, json::number_float_t>},
            .arg_flt = {
                .arg_int = &safe_mod<json::number_float_t, json::number_integer_t>,
                .arg_flt = &safe_mod<json::number_float_t, json::number_float_t>}});

    register_function(
        freg, "**",
        binary_overload_set{
            .arg_int =
                {.arg_int = &safe_pow<json::number_integer_t, json::number_integer_t>,
                 .arg_flt = &safe_pow<json::number_integer_t, json::number_float_t>},
            .arg_flt = {
                .arg_int = &safe_pow<json::number_float_t, json::number_integer_t>,
                .arg_flt = &safe_pow<json::number_float_t, json::number_float_t>}});

    register_function(
        freg, "[]",
        binary_overload_set{
            .arg_string = {.arg_int = &safe_access<json::string_t, json::number_integer_t>},
            .arg_array  = {.arg_int = &safe_access<json::array_t, json::number_integer_t>},
            .arg_object = {.arg_string = &safe_access<json, json::string_t>}});

    register_function(
        freg, "[:]",
        trinary_overload_set{
            .arg_string =
                {.arg_int =
                     {.arg_int = &safe_range_access<json::string_t, json::number_integer_t>}},
            .arg_array =
                {.arg_int = {.arg_int = &safe_range_access<json::array_t, json::number_integer_t>}},
        });

    register_function(
        freg, "in",
        binary_overload_set{
            .arg_int  = {.arg_array = &safe_contains<true, json::number_integer_t, json::array_t>},
            .arg_flt  = {.arg_array = &safe_contains<true, json::number_float_t, json::array_t>},
            .arg_bool = {.arg_array = &safe_contains<true, json::boolean_t, json::array_t>},
            .arg_string =
                {.arg_string = &safe_contains<true, json::string_t, json::string_t>,
                 .arg_array  = &safe_contains<true, json::string_t, json::array_t>,
                 .arg_object = &safe_contains<true, json::string_t, json>},
            .arg_array  = {.arg_array = &safe_contains<true, json::array_t, json::array_t>},
            .arg_null   = {.arg_array = &safe_contains<true, std::nullptr_t, json::array_t>},
            .arg_object = {.arg_array = &safe_contains<true, json, json::array_t>}});

    register_function(
        freg, "not in",
        binary_overload_set{
            .arg_int  = {.arg_array = &safe_contains<false, json::number_integer_t, json::array_t>},
            .arg_flt  = {.arg_array = &safe_contains<false, json::number_float_t, json::array_t>},
            .arg_bool = {.arg_array = &safe_contains<false, json::boolean_t, json::array_t>},
            .arg_string =
                {.arg_string = &safe_contains<false, json::string_t, json::string_t>,
                 .arg_array  = &safe_contains<false, json::string_t, json::array_t>,
                 .arg_object = &safe_contains<false, json::string_t, json>},
            .arg_array  = {.arg_array = &safe_contains<false, json::array_t, json::array_t>},
            .arg_null   = {.arg_array = &safe_contains<false, std::nullptr_t, json::array_t>},
            .arg_object = {.arg_array = &safe_contains<false, json, json::array_t>}});

    return freg;
}
