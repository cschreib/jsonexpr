#include "jsonexpr/base.hpp"

#include "jsonexpr/ast.hpp"

#include <cmath>
#include <sstream>

using namespace jsonexpr;

std::string jsonexpr::format_error(std::string_view expression, const error& e) {
    std::ostringstream str;
    str << expression << std::endl;
    str << std::string(e.location.position, ' ') << '^'
        << std::string(e.location.length > 0 ? e.location.length - 1 : 0, '~') << std::endl;
    str << "error: " << e.message;
    return str.str();
}

std::string_view jsonexpr::get_dynamic_type_name(const json& j) noexcept {
    switch (j.type()) {
    case json::value_t::object: return get_type_name<object_t>();
    case json::value_t::array: return get_type_name<array_t>();
    case json::value_t::string: return get_type_name<string_t>();
    case json::value_t::boolean: return get_type_name<boolean_t>();
    case json::value_t::number_unsigned: [[fallthrough]];
    case json::value_t::number_integer: return get_type_name<number_integer_t>();
    case json::value_t::number_float: return get_type_name<number_float_t>();
    case json::value_t::null: return get_type_name<null_t>();
    default: return "unknown";
    }
}

void impl::function::add_overload(std::string key, basic_function_t func) {
    if (!std::holds_alternative<overload_t>(overloads)) {
        overloads.emplace<overload_t>();
    }

    std::get<overload_t>(overloads)[key] = std::move(func);
}
