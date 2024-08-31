#include <fstream>
#include <jsonexpr/jsonexpr.hpp>
#include <jsonexpr/parse.hpp>
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace jsonexpr;
using namespace snitch::matchers;

namespace snitch {
template<std::same_as<json> T>
bool append(snitch::small_string_span ss, const T& j) {
    return append(ss, j.dump());
}

inline bool append(snitch::small_string_span ss, const error& e) {
    return append(ss, e.message);
}

template<typename T, typename E>
bool append(snitch::small_string_span ss, const expected<T, E>& r) {
    if (r.has_value()) {
        return append(ss, r.value());
    } else {
        return append(ss, r.error());
    }
}
} // namespace snitch

inline void clear_errors() {
    std::ofstream("tests/data/errors.txt");
}

inline void dump_error(std::string_view expr, const error& e) {
    std::ofstream("tests/data/errors.txt", std::ios::app) << format_error(expr, e) << "\n";
}

template<typename... Args>
void CHECK_ERROR(
    std::string_view         expr,
    const variable_registry& vars  = {},
    const function_registry& funcs = default_functions()) {
    const auto check_result = evaluate(expr, vars, funcs);
    CAPTURE(expr);
    CHECK(!check_result.has_value());
    if (!check_result.has_value()) {
        dump_error(expr, check_result.error());
    }
}
