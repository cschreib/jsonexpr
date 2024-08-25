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
    std::size_t      position;
    std::string_view content;
};

struct error {
    std::size_t position = 0;
    std::size_t length   = 0;
    std::string message;
};

std::string format_error(std::string_view expression, const error& e);

using json_variant = std::variant<
    json::number_float_t,
    json::number_integer_t,
    json::string_t,
    json::array_t,
    json::boolean_t,
    json>;

json_variant to_variant(const json& j);

std::string_view get_type_name(const json& j) noexcept;

using variable_registry = std::unordered_map<std::string_view, json>;

using function_result = expected<json, std::string>;
using function        = std::function<function_result(const json&)>;

using function_registry =
    std::unordered_map<std::string_view, std::unordered_map<std::size_t, function>>;
} // namespace jsonexpr

#endif
