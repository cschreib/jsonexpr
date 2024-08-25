#ifndef JSONEXPR_FUNCTIONS_HPP
#define JSONEXPR_FUNCTIONS_HPP

#include "jsonexpr/base.hpp"

namespace jsonexpr {
void register_function(
    function_registry&                                funcs,
    std::string_view                                  name,
    std::size_t                                       arity,
    std::function<basic_function_result(const json&)> func);

void register_function(
    function_registry&                                                                   funcs,
    std::string_view                                                                     name,
    std::size_t                                                                          arity,
    std::function<function_result(
        std::span<const ast::node>, const variable_registry&, const function_registry&)> func);

function_registry default_functions();
} // namespace jsonexpr

#endif
