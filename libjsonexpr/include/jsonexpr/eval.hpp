#ifndef JSONEXPR_EVAL_HPP
#define JSONEXPR_EVAL_HPP

#include "jsonexpr/ast.hpp"
#include "jsonexpr/expected.hpp"

#include <functional>
#include <string_view>

namespace jsonexpr {
struct eval_error {
    std::size_t position = 0;
    std::size_t length   = 0;
    std::string message;

    explicit eval_error(std::size_t p, std::size_t l, std::string msg) noexcept :
        position(p), length(l), message(std::move(msg)) {}

    explicit eval_error(const ast::node& n, std::string msg) noexcept :
        position(n.location.position),
        length(n.location.content.length()),
        message(std::move(msg)) {}

    explicit eval_error(std::string msg) noexcept : message(std::move(msg)) {}
};

using variable_registry = std::unordered_map<std::string_view, json>;
using function          = std::function<tl::expected<json, eval_error>(const json&)>;
using function_registry =
    std::unordered_map<std::string_view, std::unordered_map<std::size_t, function>>;

function_registry default_functions();

tl::expected<json, eval_error> evaluate_ast(
    const ast::node&         n,
    const variable_registry& vreg = {},
    const function_registry& freg = default_functions());

} // namespace jsonexpr

#endif
