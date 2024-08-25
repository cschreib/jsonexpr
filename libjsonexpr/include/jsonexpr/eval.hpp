#ifndef JSONEXPR_EVAL_HPP
#define JSONEXPR_EVAL_HPP

#include "jsonexpr/ast.hpp"
#include "jsonexpr/base.hpp"
#include "jsonexpr/config.hpp"
#include "jsonexpr/expected.hpp"
#include "jsonexpr/functions.hpp"

namespace jsonexpr {
expected<json, error> evaluate(
    const ast::node&         n,
    const variable_registry& vreg = {},
    const function_registry& freg = default_functions());
} // namespace jsonexpr

#endif
