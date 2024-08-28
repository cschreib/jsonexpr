#include "common.hpp"

TEST_CASE("variable", "[general]") {
    variable_registry vars;
    vars["a"] = "1"_json;
    vars["b"] = "2"_json;
    vars["A"] = "3"_json;
    vars["B"] = "4"_json;

    function_registry funcs = default_functions();
    register_function(funcs, "identity", 1, [](const json& j) { return j[0]; });

    SECTION("bad") {
        CHECK(!evaluate("d", vars).has_value());
        CHECK(!evaluate("ab", vars).has_value());
        CHECK(!evaluate("a b", vars).has_value());
        CHECK(!evaluate("D", vars).has_value());
    }

    SECTION("good") {
        CHECK(evaluate("a", vars) == "1"_json);
        CHECK(evaluate("b", vars) == "2"_json);
        CHECK(evaluate("A", vars) == "3"_json);
        CHECK(evaluate("B", vars) == "4"_json);
        CHECK(evaluate("a+b", vars) == "3"_json);
        CHECK(evaluate("(a)", vars) == "1"_json);
        CHECK(evaluate("(a)+(b)", vars) == "3"_json);
    }
}
