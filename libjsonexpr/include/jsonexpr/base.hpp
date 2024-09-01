#ifndef JSONEXPR_BASE_HPP
#define JSONEXPR_BASE_HPP

#include "jsonexpr/config.hpp"
#include "jsonexpr/expected.hpp"

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace jsonexpr {
using json = nlohmann::json;

using number_integer_t = json::number_integer_t;
using number_float_t   = json::number_float_t;
using boolean_t        = json::boolean_t;
using string_t         = json::string_t;
using array_t          = json::array_t;
using object_t         = json::object_t;
using null_t           = std::nullptr_t;

namespace ast {
struct node;
}

struct source_location {
    std::size_t position = 0;
    std::size_t length   = 0;
};

struct error {
    source_location location = {};
    std::string     message  = {};
};

JSONEXPR_EXPORT std::string format_error(std::string_view expression, const error& e);

JSONEXPR_EXPORT std::string_view get_dynamic_type_name(const json& j) noexcept;

template<typename T>
std::string_view get_type_name() noexcept {
    if constexpr (std::is_same_v<T, number_float_t>) {
        return "float";
    } else if constexpr (std::is_same_v<T, number_integer_t>) {
        return "int";
    } else if constexpr (std::is_same_v<T, string_t>) {
        return "string";
    } else if constexpr (std::is_same_v<T, array_t>) {
        return "array";
    } else if constexpr (std::is_same_v<T, boolean_t>) {
        return "bool";
    } else if constexpr (std::is_same_v<T, null_t>) {
        return "null";
    } else if constexpr (std::is_same_v<T, object_t>) {
        return "object";
    } else if constexpr (std::is_same_v<T, json>) {
        return "json";
    } else {
        static_assert(!std::is_same_v<T, T>, "unsupported type");
    }
}

template<typename T>
std::string_view get_type_name(const T&) noexcept {
    return get_type_name<T>();
}

using ast_function_result = expected<json, error>;
using function_result     = expected<json, std::string>;

namespace impl {
struct function;
}

using function_registry = std::unordered_map<std::string, impl::function>;

using variable_registry = std::unordered_map<std::string, json>;

namespace impl {
struct function {
    using ast_function_t   = std::function<ast_function_result(
        std::span<const ast::node>, const variable_registry&, const function_registry&)>;
    using basic_function_t = std::function<function_result(std::span<const json>)>;

    using overload_t = std::unordered_map<std::string, basic_function_t>;

    std::variant<ast_function_t, overload_t> overloads;

    JSONEXPR_EXPORT void add_overload(std::string key, basic_function_t func);
};
} // namespace impl
} // namespace jsonexpr

#endif
