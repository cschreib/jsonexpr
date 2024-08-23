#include "jsonexpr/jsonexpr.hpp"

#include <sstream>

using namespace jsonexpr;

tl::expected<json, error> jsonexpr::evaluate(
    std::string_view expression, const variable_registry& vreg, const function_registry& freg) {

    const auto ast = parse(expression);
    if (!ast.has_value()) {
        auto e = error(ast.error());
        if (e.length == 0) {
            e.position = expression.size();
        }

        return tl::unexpected(e);
    }

    const auto result = evaluate_ast(ast.value(), vreg, freg);
    if (!result.has_value()) {
        return tl::unexpected(error(result.error()));
    }

    return result.value();
}

std::string jsonexpr::format_error(std::string_view expression, const error& e) {
    std::ostringstream str;
    str << expression << std::endl;
    str << std::string(e.position, ' ') << '^' << std::string(e.length > 0 ? e.length - 1 : 0, '~')
        << std::endl;
    str << "error: " << e.message << std::endl;
    return str.str();
}
