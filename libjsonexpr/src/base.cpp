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
    case json::value_t::number_unsigned: [[fallthrough]];
    case json::value_t::number_integer: return "int";
    case json::value_t::number_float: return "float";
    case json::value_t::null: return "null";
    default: return "unknown";
    }
}

std::string_view jsonexpr::get_type_name(const json::number_float_t&) noexcept {
    return "float";
}
std::string_view jsonexpr::get_type_name(const json::number_integer_t&) noexcept {
    return "int";
}
std::string_view jsonexpr::get_type_name(const json::string_t&) noexcept {
    return "string";
}
std::string_view jsonexpr::get_type_name(const json::array_t&) noexcept {
    return "array";
}
std::string_view jsonexpr::get_type_name(const json::boolean_t&) noexcept {
    return "bool";
}

json_variant jsonexpr::to_variant(const json& j) {
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
