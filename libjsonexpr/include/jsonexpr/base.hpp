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

using json_variant = std::variant<
    json::number_float_t,
    json::number_integer_t,
    json::string_t,
    json::array_t,
    json::boolean_t,
    std::nullptr_t,
    json>;

JSONEXPR_EXPORT json_variant to_variant(const json& j);

JSONEXPR_EXPORT std::string_view get_type_name(const json&) noexcept;
JSONEXPR_EXPORT std::string_view get_type_name(json::number_float_t) noexcept;
JSONEXPR_EXPORT std::string_view get_type_name(json::number_integer_t) noexcept;
JSONEXPR_EXPORT std::string_view get_type_name(const json::string_t&) noexcept;
JSONEXPR_EXPORT std::string_view get_type_name(const json::array_t&) noexcept;
JSONEXPR_EXPORT std::string_view get_type_name(json::boolean_t) noexcept;
JSONEXPR_EXPORT std::string_view get_type_name(std::nullptr_t) noexcept;

using function_result       = expected<json, error>;
using basic_function_result = expected<json, std::string>;

struct function;
using function_registry =
    std::unordered_map<std::string_view, std::unordered_map<std::size_t, function>>;

using variable_registry = std::unordered_map<std::string, json>;

struct function {
    std::function<function_result(
        std::span<const ast::node>, const variable_registry&, const function_registry&)>
        callable;
};
} // namespace jsonexpr

#endif
