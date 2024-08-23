#ifndef JSONEXPR_HPP
#define JSONEXPR_HPP

#include "jsonexpr/eval.hpp"
#include "jsonexpr/parse.hpp"

namespace jsonexpr {
struct error {
    std::size_t position = 0;
    std::size_t length   = 0;
    std::string message;

    explicit error(const parse_error& e) noexcept :
        position(e.position), length(e.length), message(e.message) {}

    explicit error(const eval_error& e) noexcept :
        position(e.position), length(e.length), message(e.message) {}
};

tl::expected<json, error> evaluate(
    std::string_view         expression,
    const variable_registry& vreg = {},
    const function_registry& freg = default_functions());

std::string format_error(std::string_view expression, const error& e);

} // namespace jsonexpr

#endif
