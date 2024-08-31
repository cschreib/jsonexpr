#ifndef JSONEXPR_FUNCTIONS_HPP
#define JSONEXPR_FUNCTIONS_HPP

#include "jsonexpr/base.hpp"

#include <iostream>

namespace jsonexpr {
namespace impl {
template<typename T, std::size_t I>
decltype(auto) get_value(std::span<const json>& args) {
    if constexpr (std::is_same_v<T, std::nullptr_t>) {
        return nullptr;
    } else if constexpr (
        std::is_same_v<T, string_t> || std::is_same_v<T, array_t> || std::is_same_v<T, object_t>) {
        return args[I].get_ref<const T&>();
    } else if constexpr (std::is_same_v<T, json>) {
        return args[I];
    } else {
        return args[I].get<T>();
    }
}

template<typename... Args>
struct type_list {};

template<typename Function, typename... Args, std::size_t... Indices>
basic_function_result call(
    Function              func,
    std::span<const json> args,
    type_list<Args...>,
    std::index_sequence<Indices...>) {
    return func(get_value<std::decay_t<Args>, Indices>(args)...);
}

JSONEXPR_EXPORT void add_type(std::string& key, std::string_view type);

template<typename T>
concept function_ptr = std::is_function_v<std::remove_pointer_t<T>>;

template<typename T>
concept stateless_lambda = (!function_ptr<T>) && requires(const T& func) {
                                                     { +func } -> function_ptr;
                                                 };
} // namespace impl

template<typename R, typename... Args>
    requires std::is_convertible_v<R, basic_function_result>
void register_function(function_registry& funcs, std::string_view name, R (*func)(Args...)) {
    std::string key;
    (impl::add_type(key, get_type_name<std::decay_t<Args>>()), ...);

    funcs[std::string{name}].add_overload(
        key, [=](std::span<const json> args) -> basic_function_result {
            return impl::call(
                func, args, impl::type_list<Args...>{},
                std::make_index_sequence<sizeof...(Args)>{});
        });
}

template<impl::stateless_lambda Func>
void register_function(function_registry& funcs, std::string_view name, const Func& func) {
    register_function(funcs, name, +func);
}

JSONEXPR_EXPORT void register_ast_function(
    function_registry&                                                                   funcs,
    std::string_view                                                                     name,
    std::function<function_result(
        std::span<const ast::node>, const variable_registry&, const function_registry&)> func);

JSONEXPR_EXPORT function_registry default_functions();
} // namespace jsonexpr

#endif
