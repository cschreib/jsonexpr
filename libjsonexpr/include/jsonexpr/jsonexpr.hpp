#ifndef JSONEXPR_HPP
#define JSONEXPR_HPP

#include "jsonexpr/base.hpp"

namespace jsonexpr {
expected<json, error> evaluate(
    std::string_view         expression,
    const variable_registry& vreg = {},
    const function_registry& freg = default_functions());

} // namespace jsonexpr

#endif
