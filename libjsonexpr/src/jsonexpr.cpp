#include "jsonexpr/jsonexpr.hpp"

#include "jsonexpr/eval.hpp"
#include "jsonexpr/parse.hpp"

using namespace jsonexpr;

expected<json, error> jsonexpr::evaluate(
    std::string_view expression, const variable_registry& vreg, const function_registry& freg) {

    const auto ast = parse(expression);
    if (!ast.has_value()) {
        auto e = error(ast.error());
        if (e.location.length == 0) {
            e.location.position = expression.size();
        }

        return unexpected(e);
    }

    const auto result = evaluate(ast.value(), vreg, freg);
    if (!result.has_value()) {
        return unexpected(error(result.error()));
    }

    return result.value();
}
