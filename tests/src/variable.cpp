#include "common.hpp"

TEST_CASE("variable", "[general]") {
    variable_registry vars;
    vars["a"] = "1"_json;
    vars["b"] = "2"_json;
    vars["A"] = "3"_json;
    vars["B"] = "4"_json;

    SECTION("bad") {
        CHECK_ERROR("d", vars);
        CHECK_ERROR("ab", vars);
        CHECK_ERROR("a b", vars);
        CHECK_ERROR("D", vars);
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
