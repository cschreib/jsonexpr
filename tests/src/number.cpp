#include "common.hpp"

TEST_CASE("number literal", "[maths]") {
    SECTION("bad") {
        CHECK_ERROR("01");
        CHECK_ERROR("1 0");
        CHECK_ERROR("9223372036854775808");
        CHECK_ERROR("1 e-2");
        CHECK_ERROR("1e -2");
        CHECK_ERROR("1e- 2");
        CHECK_ERROR("1d10");
        CHECK_ERROR("1.1.2");
    }

    SECTION("good") {
        CHECK(evaluate("1") == "1"_json);
        CHECK(evaluate("10") == "10"_json);
        CHECK(evaluate("0.1") == "0.1"_json);
        CHECK(evaluate("123456789") == "123456789"_json);
        CHECK(evaluate("9223372036854775807") == "9223372036854775807"_json);
        CHECK(evaluate("1e2") == "1e2"_json);
        CHECK(evaluate("1E2") == "1E2"_json);
        CHECK(evaluate("1e-2") == "1e-2"_json);
        CHECK(evaluate("10e-2") == "10e-2"_json);
        CHECK(evaluate("1.5e-2") == "1.5e-2"_json);
        CHECK(evaluate("0.8e-2") == "0.8e-2"_json);
        CHECK(evaluate("0.8e-02") == "0.8e-02"_json);
        CHECK(evaluate("0.8e15") == "0.8e15"_json);
    }
}

TEST_CASE("adsub", "[maths]") {
    SECTION("bad") {
        CHECK_ERROR("1-");
        CHECK_ERROR("1+");
        CHECK_ERROR("+");
        CHECK_ERROR("++");
        CHECK_ERROR("-");
        CHECK_ERROR("--");
    }

    SECTION("unary") {
        CHECK(evaluate("+1") == "1"_json);
        CHECK(evaluate("-1") == "-1"_json);
        CHECK(evaluate("+ 1") == "1"_json);
        CHECK(evaluate("- 1") == "-1"_json);
    }

    SECTION("binary") {
        CHECK(evaluate("1+1") == "2"_json);
        CHECK(evaluate("1-1") == "0"_json);
        CHECK(evaluate("0-1") == "-1"_json);
        CHECK(evaluate("1+0") == "1"_json);
        CHECK(evaluate("1-0") == "1"_json);
        CHECK(evaluate("1 + 1") == "2"_json);
        CHECK(evaluate(" 1+1") == "2"_json);
        CHECK(evaluate("1 +1") == "2"_json);
        CHECK(evaluate("1+ 1") == "2"_json);
        CHECK(evaluate("1+1 ") == "2"_json);
        CHECK(evaluate("1+1 ") == "2"_json);
    }

    SECTION("binary and unary") {
        CHECK(evaluate("1 + -1") == "0"_json);
        CHECK(evaluate("1+-1") == "0"_json);
        CHECK(evaluate("-1+1") == "0"_json);
        CHECK(evaluate("-1+-1") == "-2"_json);
        CHECK(evaluate("1++1") == "2"_json);
        CHECK(evaluate("1+ +1") == "2"_json);
        CHECK(evaluate("1 ++1") == "2"_json);
        CHECK(evaluate("1++ 1") == "2"_json);
    }

    SECTION("int overflow") {
        CHECK(evaluate("9223372036854775807+0") == "9223372036854775807"_json);
        CHECK_ERROR("9223372036854775807+1");
        CHECK_ERROR("9223372036854775807+2");
        CHECK_ERROR("-9223372036854775807+(-1)");
        CHECK_ERROR("-9223372036854775807+(-2)");
        CHECK_ERROR("1+9223372036854775807");
        CHECK_ERROR("2+9223372036854775807");
        CHECK_ERROR("-1+(-9223372036854775807)");
        CHECK_ERROR("-2+(-9223372036854775807)");
        CHECK_ERROR("9223372036854775807+9223372036854775807");

        CHECK(evaluate("-9223372036854775807-0") == "-9223372036854775807"_json);
        CHECK_ERROR("-9223372036854775807-1");
        CHECK_ERROR("-9223372036854775807-2");
        CHECK_ERROR("9223372036854775807-(-1)");
        CHECK_ERROR("9223372036854775807-(-2)");
        CHECK_ERROR("1-(-9223372036854775807)");
        CHECK_ERROR("2-(-9223372036854775807)");
        CHECK_ERROR("-1-9223372036854775807");
        CHECK_ERROR("-2-9223372036854775807");
        CHECK_ERROR("9223372036854775807-(-2)");
        CHECK_ERROR("-9223372036854775807-9223372036854775807");
    }
}

TEST_CASE("muldiv", "[maths]") {
    SECTION("bad") {
        CHECK_ERROR("1*");
        CHECK_ERROR("1/");
        CHECK_ERROR("*1");
        CHECK_ERROR("/1");
        CHECK_ERROR("*");
        CHECK_ERROR("/");
    }

    SECTION("int") {
        CHECK(evaluate("2*4") == "8"_json);
        CHECK(evaluate("4/2") == "2"_json);
        CHECK(evaluate("2/4") == "0"_json);
        CHECK(evaluate("0/1") == "0"_json);
        CHECK_ERROR("1/0");
        CHECK_ERROR("0/0");
    }

    SECTION("int overflow") {
        CHECK(evaluate("9223372036854775807*1") == "9223372036854775807"_json);
        CHECK_ERROR("9223372036854775807*2");
        CHECK_ERROR("9223372036854775807*9223372036854775807");
        CHECK(evaluate("9223372036854775807*-1") == "-9223372036854775807"_json);
        CHECK_ERROR("9223372036854775807*-2");
        CHECK_ERROR("9223372036854775807*-9223372036854775807");

        CHECK_ERROR("-9223372036854775808/-1");
    }

    SECTION("float") {
        CHECK(evaluate("2.0*4.0") == "8.0"_json);
        CHECK(evaluate("4.0/2.0") == "2.0"_json);
        CHECK(evaluate("2.0/4.0") == "0.5"_json);
        CHECK(evaluate("1.0/0.0") == json(std::numeric_limits<double>::infinity()));

        auto r = evaluate("0.0/0.0");
        REQUIRE(r.has_value());
        REQUIRE(r.value().type() == json::value_t::number_float);
        CHECK(std::isnan(r.value().get<double>()));
    }

    SECTION("mixed type") {
        CHECK(evaluate("2.0*4") == "8.0"_json);
        CHECK(evaluate("4.0/2") == "2.0"_json);
        CHECK(evaluate("2.0/4") == "0.5"_json);
        CHECK(evaluate("2*4.0") == "8.0"_json);
        CHECK(evaluate("4/2.0") == "2.0"_json);
        CHECK(evaluate("2/4.0") == "0.5"_json);
    }
}

TEST_CASE("modulo", "[maths]") {
    SECTION("bad") {
        CHECK_ERROR("1%");
        CHECK_ERROR("%1");
        CHECK_ERROR("%");
        CHECK_ERROR("1%0");
        CHECK_ERROR("0%0");
        CHECK_ERROR("'a'%1");
        CHECK_ERROR("1%'a'");
    }

    SECTION("good") {
        CHECK(evaluate("4%2") == "0"_json);
        CHECK(evaluate("5%2") == "1"_json);
        CHECK(evaluate("6%2") == "0"_json);
        CHECK(evaluate("7%2") == "1"_json);

        CHECK(evaluate("-4%2") == "0"_json);
        CHECK(evaluate("-5%2") == "-1"_json);
        CHECK(evaluate("-6%2") == "0"_json);
        CHECK(evaluate("-7%2") == "-1"_json);

        CHECK(evaluate("4%-2") == "0"_json);
        CHECK(evaluate("5%-2") == "1"_json);
        CHECK(evaluate("6%-2") == "0"_json);
        CHECK(evaluate("7%-2") == "1"_json);

        CHECK(evaluate("4.0%2.0") == "0.0"_json);
        CHECK(evaluate("5.0%2.0") == "1.0"_json);
        CHECK(evaluate("6.0%2.0") == "0.0"_json);
        CHECK(evaluate("7.0%2.0") == "1.0"_json);
        CHECK(evaluate("7.25%2.0") == "1.25"_json);

        CHECK(evaluate("-4.0%2.0") == "0.0"_json);
        CHECK(evaluate("-5.0%2.0") == "-1.0"_json);
        CHECK(evaluate("-6.0%2.0") == "0.0"_json);
        CHECK(evaluate("-7.0%2.0") == "-1.0"_json);
        CHECK(evaluate("-7.25%2.0") == "-1.25"_json);

        CHECK(evaluate("4.0%-2.0") == "0.0"_json);
        CHECK(evaluate("5.0%-2.0") == "1.0"_json);
        CHECK(evaluate("6.0%-2.0") == "0.0"_json);
        CHECK(evaluate("7.0%-2.0") == "1.0"_json);
        CHECK(evaluate("7.25%-2.0") == "1.25"_json);
    }
}

TEST_CASE("precedence", "[maths]") {
    SECTION("two") {
        CHECK(evaluate("1+2*3") == "7"_json);
        CHECK(evaluate("1 +2*3") == "7"_json);
        CHECK(evaluate("1+ 2*3") == "7"_json);
        CHECK(evaluate("1+2 *3") == "7"_json);
        CHECK(evaluate("1+2* 3") == "7"_json);

        CHECK(evaluate("1+(2*3)") == "7"_json);
        CHECK(evaluate("(1+2)*3") == "9"_json);
    }

    SECTION("three") {
        CHECK(evaluate("1+2*3**2") == "19"_json);
        CHECK(evaluate("1**2+3*2") == "7"_json);
        CHECK(evaluate("1*2**3+2") == "10"_json);

        CHECK(evaluate("1+2*(3**2)") == "19"_json);
        CHECK(evaluate("1+(2*3)**2") == "37"_json);
        CHECK(evaluate("(1+2)*3**2") == "27"_json);

        CHECK(evaluate("2*3+4*5") == "26"_json);
        CHECK(evaluate("2*3*4+5") == "29"_json);
        CHECK(evaluate("2+3*4*5") == "62"_json);
    }

    SECTION("unary binary") {
        CHECK(evaluate("-1**2") == "1"_json);
        CHECK(evaluate("10.0**-2") == "0.01"_json);
        CHECK(evaluate("-1*-1") == "1"_json);
        CHECK(evaluate("+1*-1") == "-1"_json);
        CHECK(evaluate("-1*+1") == "-1"_json);
    }
}
