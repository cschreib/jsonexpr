#include "common.hpp"

TEST_CASE("boolean", "[maths]") {
    SECTION("bad") {
        CHECK(!evaluate("&").has_value());
        CHECK(!evaluate("&&").has_value());
        CHECK(!evaluate("and").has_value());
        CHECK(!evaluate("|").has_value());
        CHECK(!evaluate("||").has_value());
        CHECK(!evaluate("or").has_value());
        CHECK(!evaluate("!").has_value());
        CHECK(!evaluate("not").has_value());
        CHECK(!evaluate("!=").has_value());
        CHECK(!evaluate("==").has_value());
        CHECK(!evaluate("=").has_value());
        CHECK(!evaluate(">").has_value());
        CHECK(!evaluate("<").has_value());
        CHECK(!evaluate("<=").has_value());
        CHECK(!evaluate(">=").has_value());
        CHECK(!evaluate("=<").has_value());
        CHECK(!evaluate("=>").has_value());
        CHECK(!evaluate("and and").has_value());
        CHECK(!evaluate("true and").has_value());
        CHECK(!evaluate("and true").has_value());
        CHECK(!evaluate("or or").has_value());
        CHECK(!evaluate("true or").has_value());
        CHECK(!evaluate("or true").has_value());
        CHECK(!evaluate("> >").has_value());
        CHECK(!evaluate("true >").has_value());
        CHECK(!evaluate("> true").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("not true") == "false"_json);
        CHECK(evaluate("not false") == "true"_json);

        CHECK(evaluate("true and true") == "true"_json);
        CHECK(evaluate("true and false") == "false"_json);
        CHECK(evaluate("false and true") == "false"_json);
        CHECK(evaluate("false and false") == "false"_json);

        CHECK(evaluate("true or true") == "true"_json);
        CHECK(evaluate("true or false") == "true"_json);
        CHECK(evaluate("false or true") == "true"_json);
        CHECK(evaluate("false or false") == "false"_json);

        CHECK(evaluate("1 < 2") == "true"_json);
        CHECK(evaluate("1 <= 2") == "true"_json);
        CHECK(evaluate("1 > 2") == "false"_json);
        CHECK(evaluate("1 >= 2") == "false"_json);
        CHECK(evaluate("1 == 2") == "false"_json);
        CHECK(evaluate("1 != 2") == "true"_json);

        CHECK(evaluate("1 < 1") == "false"_json);
        CHECK(evaluate("1 <= 1") == "true"_json);
        CHECK(evaluate("1 > 1") == "false"_json);
        CHECK(evaluate("1 >= 1") == "true"_json);
        CHECK(evaluate("1 == 1") == "true"_json);
        CHECK(evaluate("1 != 1") == "false"_json);
    }

    SECTION("precedence") {
        CHECK(evaluate("true or true and true") == "true"_json);
        CHECK(evaluate("false or true and true") == "true"_json);
        CHECK(evaluate("true or false and true") == "true"_json);
        CHECK(evaluate("true or true and false") == "true"_json);
        CHECK(evaluate("false or false and true") == "false"_json);
        CHECK(evaluate("false or true and false") == "false"_json);
        CHECK(evaluate("true or false and false") == "true"_json);
        CHECK(evaluate("false or false and false") == "false"_json);

        CHECK(evaluate("true and true or true") == "true"_json);
        CHECK(evaluate("true and true or false") == "true"_json);
        CHECK(evaluate("false and true or true") == "true"_json);
        CHECK(evaluate("true and false or true") == "true"_json);
        CHECK(evaluate("false and true or false") == "false"_json);
        CHECK(evaluate("true and false or false") == "false"_json);
        CHECK(evaluate("false and false or true") == "true"_json);
        CHECK(evaluate("false and false or false") == "false"_json);

        CHECK(evaluate("1 < 2 or 1 > 2") == "true"_json);
        CHECK(evaluate("1+1 < 2") == "false"_json);
        CHECK(evaluate("1 < -2") == "false"_json);
    }

    SECTION("short-circuit") {
        CHECK(!evaluate("error()").has_value());

        CHECK(evaluate("false and error()") == "false"_json);
        CHECK(!evaluate("error() and false").has_value());

        CHECK(evaluate("true or error()") == "true"_json);
        CHECK(!evaluate("error() or true").has_value());

        CHECK(evaluate("true and false and error()") == "false"_json);
        CHECK(evaluate("false and true and error()") == "false"_json);
        CHECK(evaluate("false and error() and true") == "false"_json);
        CHECK(!evaluate("error() and false and true").has_value());
        CHECK(!evaluate("error() and true and false").has_value());
        CHECK(!evaluate("true and error() and false").has_value());

        CHECK(evaluate("true or false or error()") == "true"_json);
        CHECK(evaluate("false or true or error()") == "true"_json);
        CHECK(evaluate("true or error() or false") == "true"_json);
        CHECK(!evaluate("error() or false or true").has_value());
        CHECK(!evaluate("error() or true or false").has_value());
        CHECK(!evaluate("false or error() or true").has_value());
    }
}
