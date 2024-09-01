#ifndef JSONEXPR_HPP
#define JSONEXPR_HPP

#include "jsonexpr/base.hpp"
#include "jsonexpr/eval.hpp"
#include "jsonexpr/expected.hpp"
#include "jsonexpr/functions.hpp"

namespace jsonexpr {
JSONEXPR_EXPORT expected<json, error> evaluate(
    std::string_view         expression,
    const variable_registry& vreg = {},
    const function_registry& freg = default_functions());

} // namespace jsonexpr

#endif
