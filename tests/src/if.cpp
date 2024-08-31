#include "common.hpp"

TEST_CASE("if else", "[general]") {
    SECTION("bad") {
        CHECK_ERROR("if");
        CHECK_ERROR("else");
        CHECK_ERROR("if true");
        CHECK_ERROR("if true else");
        CHECK_ERROR("if true else 0");
        CHECK_ERROR("1 if");
        CHECK_ERROR("1 if true");
        CHECK_ERROR("1 if true else");
        CHECK_ERROR("1 if else");
        CHECK_ERROR("1 else");
        CHECK_ERROR("1 else if");

        CHECK_ERROR("1 if *1 else 2");
        CHECK_ERROR("1 if true else *2");
    }

    SECTION("good") {
        CHECK(evaluate("1 if true else 0") == "1"_json);
        CHECK(evaluate("1 if false else 0") == "0"_json);
        CHECK(evaluate("1 if 1 > 0 else 0") == "1"_json);
        CHECK(evaluate("1 if 1 < 0 else 0") == "0"_json);
        CHECK(evaluate("'a'+'b' if true else 'c'+'d'") == R"("ab")"_json);
        CHECK(evaluate("'a'+'b' if false else 'c'+'d'") == R"("cd")"_json);

        variable_registry vars;
        vars["a"] = "a";
        CHECK(evaluate("1 if a == 'a' else 2 if a == 'b' else 3", vars) == "1"_json);
        vars["a"] = "b";
        CHECK(evaluate("1 if a == 'a' else 2 if a == 'b' else 3", vars) == "2"_json);
        vars["a"] = "c";
        CHECK(evaluate("1 if a == 'a' else 2 if a == 'b' else 3", vars) == "3"_json);
    }

    SECTION("short-circuit") {
        CHECK_ERROR("error()");

        CHECK(evaluate("error() if false else 1") == "1"_json);
        CHECK(evaluate("1 if true else error()") == "1"_json);
        CHECK(evaluate("1 if true else error() if error() else error()") == "1"_json);
    }
}
