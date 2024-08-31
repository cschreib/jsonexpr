#include "common.hpp"

TEST_CASE("boolean", "[maths]") {
    SECTION("bad") {
        CHECK_ERROR("&");
        CHECK_ERROR("&&");
        CHECK_ERROR("and");
        CHECK_ERROR("|");
        CHECK_ERROR("||");
        CHECK_ERROR("or");
        CHECK_ERROR("!");
        CHECK_ERROR("not");
        CHECK_ERROR("!=");
        CHECK_ERROR("==");
        CHECK_ERROR("=");
        CHECK_ERROR(">");
        CHECK_ERROR("<");
        CHECK_ERROR("<=");
        CHECK_ERROR(">=");
        CHECK_ERROR("=<");
        CHECK_ERROR("=>");
        CHECK_ERROR("and and");
        CHECK_ERROR("true and");
        CHECK_ERROR("and true");
        CHECK_ERROR("or or");
        CHECK_ERROR("true or");
        CHECK_ERROR("or true");
        CHECK_ERROR("> >");
        CHECK_ERROR("true >");
        CHECK_ERROR("> true");
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
        CHECK_ERROR("error()");

        CHECK(evaluate("false and error()") == "false"_json);
        CHECK_ERROR("error() and false");

        CHECK(evaluate("true or error()") == "true"_json);
        CHECK_ERROR("error() or true");

        CHECK(evaluate("true and false and error()") == "false"_json);
        CHECK(evaluate("false and true and error()") == "false"_json);
        CHECK(evaluate("false and error() and true") == "false"_json);
        CHECK_ERROR("error() and false and true");
        CHECK_ERROR("error() and true and false");
        CHECK_ERROR("true and error() and false");

        CHECK(evaluate("true or false or error()") == "true"_json);
        CHECK(evaluate("false or true or error()") == "true"_json);
        CHECK(evaluate("true or error() or false") == "true"_json);
        CHECK_ERROR("error() or false or true");
        CHECK_ERROR("error() or true or false");
        CHECK_ERROR("false or error() or true");
    }
}
