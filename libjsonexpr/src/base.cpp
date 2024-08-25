#include "jsonexpr/base.hpp"

#include "jsonexpr/ast.hpp"

#include <cmath>
#include <sstream>

using namespace jsonexpr;

std::string jsonexpr::format_error(std::string_view expression, const error& e) {
    std::ostringstream str;
    str << expression << std::endl;
    str << std::string(e.position, ' ') << '^' << std::string(e.length > 0 ? e.length - 1 : 0, '~')
        << std::endl;
    str << "error: " << e.message << std::endl;
    return str.str();
}

std::string_view jsonexpr::get_type_name(const json& j) noexcept {
    switch (j.type()) {
    case json::value_t::object: return "object";
    case json::value_t::array: return "array";
    case json::value_t::string: return "string";
    case json::value_t::boolean: return "bool";
    case json::value_t::number_unsigned: return "unsigned int";
    case json::value_t::number_integer: return "int";
    case json::value_t::number_float: return "float";
    case json::value_t::null: return "null";
    default: return "unknown";
    }
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
    case json::value_t::array: return j.get<json::array_t>();
    case json::value_t::string: return j.get<json::string_t>();
    case json::value_t::boolean: return j.get<json::boolean_t>();
    case json::value_t::number_unsigned: [[fallthrough]];
    case json::value_t::number_integer: return j.get<json::number_integer_t>();
    case json::value_t::number_float: return j.get<json::number_float_t>();
    default: return j;
    }
}
} // namespace

#define UNARY_FUNCTION(NAME, EXPR)                                                                 \
    [](const json& args) -> function_result {                                                      \
        return std::visit(                                                                         \
            [&](const auto& lhs) -> function_result {                                              \
                if constexpr (requires { EXPR; }) {                                                \
                    return EXPR;                                                                   \
                } else {                                                                           \
                    return unexpected(                                                             \
                        std::string("incompatible type for '" NAME "', got ") +                    \
                        std::string(get_type_name(args[0])));                                      \
                }                                                                                  \
            },                                                                                     \
            to_variant(args[0]));                                                                  \
    }

#define BINARY_FUNCTION(NAME, EXPR)                                                                \
    [](const json& args) -> function_result {                                                      \
        return std::visit(                                                                         \
            [&](const auto& lhs, const auto& rhs) -> function_result {                             \
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
    }

#define UNARY_OPERATOR(OPERATOR) UNARY_FUNCTION(#OPERATOR, OPERATOR lhs)

#define BINARY_OPERATOR(OPERATOR) BINARY_FUNCTION(#OPERATOR, lhs OPERATOR rhs)

template<typename T, typename U>
    requires requires(T lhs, U rhs) { lhs / rhs; }
function_result safe_div(T lhs, U rhs) {
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
function_result safe_access(const T& lhs, U rhs) {
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
    freg["[]"][2]    = BINARY_FUNCTION("[]", safe_access(lhs, rhs));
    freg["min"][2]   = BINARY_FUNCTION("min", lhs <= rhs ? lhs : rhs);
    freg["max"][2]   = BINARY_FUNCTION("max", lhs >= rhs ? lhs : rhs);
    freg["abs"][1]   = UNARY_FUNCTION("abs", std::abs(lhs));
    freg["sqrt"][2]  = UNARY_FUNCTION("sqrt", std::sqrt(lhs));
    freg["round"][2] = UNARY_FUNCTION("round", std::round(lhs));
    freg["floor"][2] = UNARY_FUNCTION("floor", std::floor(lhs));
    freg["ceil"][2]  = UNARY_FUNCTION("ceil", std::ceil(lhs));
    freg["size"][1]  = UNARY_FUNCTION("size", lhs.size());

    return freg;
}
