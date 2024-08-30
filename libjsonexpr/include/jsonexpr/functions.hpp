#ifndef JSONEXPR_FUNCTIONS_HPP
#define JSONEXPR_FUNCTIONS_HPP

#include "jsonexpr/base.hpp"

namespace jsonexpr {
template<typename... Args>
struct type_list {};

template<typename Function, typename... Args>
basic_function_result call(Function func, std::span<const json>, type_list<>, const Args&... args) {
    return func(args...);
}

template<typename Function, typename... Args, typename T, typename... NextArgs>
basic_function_result call(
    Function              func,
    std::span<const json> next_args,
    type_list<T, NextArgs...>,
    const Args&... args) {

    if constexpr (std::is_same_v<T, json>) {
        return call(func, next_args.subspan(1), type_list<NextArgs...>{}, args..., next_args[0]);
    } else if constexpr (std::is_same_v<T, std::nullptr_t>) {
        return call(func, next_args.subspan(1), type_list<NextArgs...>{}, args..., nullptr);
    } else if constexpr (std::is_same_v<T, json::string_t> || std::is_same_v<T, json::array_t>) {
        return call(
            func, next_args.subspan(1), type_list<NextArgs...>{}, args...,
            next_args[0].get_ref<const T&>());
    } else {
        return call(
            func, next_args.subspan(1), type_list<NextArgs...>{}, args..., next_args[0].get<T>());
    }
}

template<typename... Args>
void register_function(
    function_registry& funcs, std::string_view name, basic_function_ptr<Args...> func) {
    std::string key;
    auto        add_type = [&](std::string_view type) {
        if (!key.empty()) {
            key += ",";
        }
        key += std::string(type);
    };

    (add_type(get_type_name<Args>()), ...);

    funcs[std::string{name}].add_overload(
        key, [=](std::span<const json> args) -> basic_function_result {
            return call(func, args, type_list<Args...>{});
        });
}

JSONEXPR_EXPORT void register_ast_function(
    function_registry&                                                                   funcs,
    std::string_view                                                                     name,
    std::function<function_result(
        std::span<const ast::node>, const variable_registry&, const function_registry&)> func);

JSONEXPR_EXPORT function_registry default_functions();
} // namespace jsonexpr

#endif
