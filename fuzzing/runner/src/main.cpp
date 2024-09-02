#include "jsonexpr/jsonexpr.hpp"

#include <fstream>
#include <iostream>

__AFL_FUZZ_INIT();

int main() {
    jsonexpr::variable_registry vars;
    jsonexpr::function_registry funcs = jsonexpr::default_functions();

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#endif

    // must be after __AFL_INIT and before __AFL_LOOP!
    unsigned char* buf = __AFL_FUZZ_TESTCASE_BUF;

    while (__AFL_LOOP(10000)) {
        // Don't use the macro directly in a call!
        const int   len        = __AFL_FUZZ_TESTCASE_LEN;
        const char* expr_chars = reinterpret_cast<char*>(buf);

        const auto expression = std::string_view{expr_chars, expr_chars + len};
        const auto result     = jsonexpr::evaluate(expression, vars, funcs);
        if (!result.has_value()) {
            jsonexpr::format_error(expression, result.error());
        }
    }

    return 0;
}
