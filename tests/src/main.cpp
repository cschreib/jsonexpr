#include <jsonexpr/jsonexpr.hpp>
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace jsonexpr;

namespace snitch {
bool append(snitch::small_string_span ss, const json& j) {
    return append(ss, std::string_view{j.dump()});
}

bool append(snitch::small_string_span ss, const error& e) {
    return append(ss, std::string_view{e.message});
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

TEST_CASE("string literal", "[string]") {
    SECTION("bad") {
        CHECK(!evaluate(R"('")").has_value());
        CHECK(!evaluate(R"("'')").has_value());
        CHECK(!evaluate(R"(")").has_value());
        CHECK(!evaluate(R"(')").has_value());
        CHECK(!evaluate(R"("abc"def")").has_value());
        CHECK(!evaluate(R"('abc'def')").has_value());
        CHECK(!evaluate(R"('\')").has_value());
        CHECK(!evaluate(R"("\")").has_value());
        CHECK(!evaluate(R"("0\1")").has_value());
    }

    SECTION("good") {
        CHECK(evaluate(R"("abc")") == R"("abc")"_json);
        CHECK(evaluate(R"('abc')") == R"("abc")"_json);
        CHECK(evaluate(R"('')") == R"("")"_json);
        CHECK(evaluate(R"(' ')") == R"(" ")"_json);
        CHECK(evaluate(R"('\t')") == R"("\t")"_json);
        CHECK(evaluate(R"('\n')") == R"("\n")"_json);
        CHECK(evaluate(R"('\r')") == R"("\r")"_json);
        CHECK(evaluate(R"('\\')") == R"("\\")"_json);
        CHECK(evaluate(R"('"')") == R"("\"")"_json);
        CHECK(evaluate(R"("'")") == R"("'")"_json);
        CHECK(evaluate(R"("\"")") == R"("\"")"_json);
        CHECK(evaluate(R"('\'')") == R"("'")"_json);
        CHECK(evaluate(R"('0\\1')") == R"("0\\1")"_json);
        CHECK(evaluate(R"('0/1')") == R"("0/1")"_json);
        CHECK(evaluate(R"('0\/1')") == R"("0\/1")"_json);
    }
}

TEST_CASE("string operations", "[string]") {
    SECTION("bad") {
        CHECK(!evaluate("'a'+").has_value());
        CHECK(!evaluate("+'b'").has_value());
        CHECK(!evaluate("-'b'").has_value());
        CHECK(!evaluate("'a'*'b'").has_value());
        CHECK(!evaluate("'a'/'b'").has_value());
        CHECK(!evaluate("'a'%'b'").has_value());
        CHECK(!evaluate("'a'^'b'").has_value());
        CHECK(!evaluate("'a' && 'b'").has_value());
        CHECK(!evaluate("'a' || 'b'").has_value());
        CHECK(!evaluate("'a' < 1").has_value());
        CHECK(!evaluate("'a' <= 1").has_value());
        CHECK(!evaluate("'a' > 1").has_value());
        CHECK(!evaluate("'a' >= 1").has_value());
        CHECK(!evaluate("!'a'").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("'a'+'b'") == R"("ab")"_json);
        CHECK(evaluate("''+'b'") == R"("b")"_json);
        CHECK(evaluate("'a'+''") == R"("a")"_json);
        CHECK(evaluate("'a' == ''") == "false"_json);
        CHECK(evaluate("'a' == 'a'") == "true"_json);
        CHECK(evaluate("'a' != ''") == "true"_json);
        CHECK(evaluate("'a' != 'a'") == "false"_json);
        CHECK(evaluate("'a' < 'b'") == "true"_json);
        CHECK(evaluate("'b' <= 'b'") == "true"_json);
        CHECK(evaluate("'b' <= 'a'") == "false"_json);
        CHECK(evaluate("'a' > 'b'") == "false"_json);
        CHECK(evaluate("'b' >= 'b'") == "true"_json);
        CHECK(evaluate("'b' >= 'a'") == "true"_json);
    }
}

TEST_CASE("number literal", "[maths]") {
    SECTION("bad") {
        CHECK(!evaluate("01").has_value());
        CHECK(!evaluate("1 0").has_value());
        CHECK(!evaluate("1 e-2").has_value());
        CHECK(!evaluate("1e -2").has_value());
        CHECK(!evaluate("1e- 2").has_value());
        CHECK(!evaluate("1d10").has_value());
        CHECK(!evaluate("1.1.2").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("1") == "1"_json);
        CHECK(evaluate("10") == "10"_json);
        CHECK(evaluate("0.1") == "0.1"_json);
        CHECK(evaluate("123456789") == "123456789"_json);
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
        CHECK(!evaluate("1-").has_value());
        CHECK(!evaluate("1+").has_value());
        CHECK(!evaluate("+").has_value());
        CHECK(!evaluate("++").has_value());
        CHECK(!evaluate("-").has_value());
        CHECK(!evaluate("--").has_value());
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
}

TEST_CASE("muldiv", "[maths]") {
    SECTION("bad") {
        CHECK(!evaluate("1*").has_value());
        CHECK(!evaluate("1/").has_value());
        CHECK(!evaluate("*1").has_value());
        CHECK(!evaluate("/1").has_value());
        CHECK(!evaluate("*").has_value());
        CHECK(!evaluate("/").has_value());
    }

    SECTION("int") {
        CHECK(evaluate("2*4") == "8"_json);
        CHECK(evaluate("4/2") == "2"_json);
        CHECK(evaluate("2/4") == "0"_json);
        CHECK(evaluate("0/1") == "0"_json);
        CHECK(!evaluate("1/0").has_value());
        CHECK(!evaluate("0/0").has_value());
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
        CHECK(evaluate("1+2*3^2") == "19"_json);
        CHECK(evaluate("1^2+3*2") == "7"_json);
        CHECK(evaluate("1*2^3+2") == "10"_json);

        CHECK(evaluate("1+2*(3^2)") == "19"_json);
        CHECK(evaluate("1+(2*3)^2") == "37"_json);
        CHECK(evaluate("(1+2)*3^2") == "27"_json);

        CHECK(evaluate("2*3+4*5") == "26"_json);
        CHECK(evaluate("2*3*4+5") == "29"_json);
        CHECK(evaluate("2+3*4*5") == "62"_json);
    }

    SECTION("unary binary") {
        CHECK(evaluate("-1^2") == "1"_json);
        CHECK(evaluate("10.0^-2") == "0.01"_json);
        CHECK(evaluate("-1*-1") == "1"_json);
        CHECK(evaluate("+1*-1") == "-1"_json);
        CHECK(evaluate("-1*+1") == "-1"_json);
    }
}

TEST_CASE("variable", "[general]") {
    variable_registry vars;
    vars["a"] = "1"_json;
    vars["b"] = "2"_json;
    vars["c"] = R"({"d": {"e": "f"}, "g": "h"})"_json;

    SECTION("bad") {
        CHECK(!evaluate("d", vars).has_value());
        CHECK(!evaluate("ab", vars).has_value());
        CHECK(!evaluate("a b", vars).has_value());
        CHECK(!evaluate("c .d", vars).has_value());
        CHECK(!evaluate("c. d", vars).has_value());
    }

    SECTION("good") {
        CHECK(evaluate("a", vars) == "1"_json);
        CHECK(evaluate("b", vars) == "2"_json);
        CHECK(evaluate("a+b", vars) == "3"_json);
        CHECK(evaluate("(a)", vars) == "1"_json);
        CHECK(evaluate("(a)+(b)", vars) == "3"_json);
        CHECK(evaluate("c", vars) == R"({"d": {"e": "f"}, "g": "h"})"_json);
        CHECK(evaluate("c.d", vars) == R"({"e": "f"})"_json);
        CHECK(evaluate("c.d.e", vars) == R"("f")"_json);
        CHECK(evaluate("c.g", vars) == R"("h")"_json);
    }
}

TEST_CASE("array", "[general]") {
    variable_registry vars;
    vars["obj"]  = "[1,2,3,4,5]"_json;
    vars["deep"] = R"({"sub": [6,7,8]})"_json;

    SECTION("bad") {
        CHECK(!evaluate("foo[1]", vars).has_value());
        CHECK(!evaluate("obj[]", vars).has_value());
        CHECK(!evaluate("obj[", vars).has_value());
        CHECK(!evaluate("obj]", vars).has_value());
        CHECK(!evaluate("obj[1)", vars).has_value());
        CHECK(!evaluate("obj[-1]", vars).has_value());
        CHECK(!evaluate("obj[1,2]", vars).has_value());
    }

    SECTION("good") {
        CHECK(evaluate("obj[0]", vars) == "1"_json);
        CHECK(evaluate("obj [0]", vars) == "1"_json);
        CHECK(evaluate("obj[ 0]", vars) == "1"_json);
        CHECK(evaluate("obj[0 ]", vars) == "1"_json);
        CHECK(evaluate("obj[0] ", vars) == "1"_json);
        CHECK(evaluate("obj[1]", vars) == "2"_json);
        CHECK(evaluate("obj[2]", vars) == "3"_json);
        CHECK(evaluate("obj[3]", vars) == "4"_json);
        CHECK(evaluate("obj[4]", vars) == "5"_json);
        CHECK(evaluate("obj[1+1]", vars) == "3"_json);
        CHECK(evaluate("obj[abs(-1)]", vars) == "2"_json);
        CHECK(evaluate("obj[obj[0]]", vars) == "2"_json);
        CHECK(evaluate("deep.sub[1]", vars) == "7"_json);
    }
}

TEST_CASE("boolean", "[maths]") {
    variable_registry vars;
    vars["true"]  = "true"_json;
    vars["false"] = "false"_json;

    SECTION("bad") {
        CHECK(!evaluate("&", vars).has_value());
        CHECK(!evaluate("&&", vars).has_value());
        CHECK(!evaluate("|", vars).has_value());
        CHECK(!evaluate("||", vars).has_value());
        CHECK(!evaluate("!", vars).has_value());
        CHECK(!evaluate("!=", vars).has_value());
        CHECK(!evaluate("==", vars).has_value());
        CHECK(!evaluate("=", vars).has_value());
        CHECK(!evaluate(">", vars).has_value());
        CHECK(!evaluate("<", vars).has_value());
        CHECK(!evaluate("<=", vars).has_value());
        CHECK(!evaluate(">=", vars).has_value());
        CHECK(!evaluate("=<", vars).has_value());
        CHECK(!evaluate("=>", vars).has_value());
        CHECK(!evaluate("&& &&", vars).has_value());
        CHECK(!evaluate("true &&", vars).has_value());
        CHECK(!evaluate("&& true", vars).has_value());
        CHECK(!evaluate("|| ||", vars).has_value());
        CHECK(!evaluate("true ||", vars).has_value());
        CHECK(!evaluate("|| true", vars).has_value());
        CHECK(!evaluate("> >", vars).has_value());
        CHECK(!evaluate("true >", vars).has_value());
        CHECK(!evaluate("> true", vars).has_value());
    }

    SECTION("good") {
        CHECK(evaluate("!true", vars) == "false"_json);
        CHECK(evaluate("!false", vars) == "true"_json);

        CHECK(evaluate("true && true", vars) == "true"_json);
        CHECK(evaluate("true && false", vars) == "false"_json);
        CHECK(evaluate("false && true", vars) == "false"_json);
        CHECK(evaluate("false && false", vars) == "false"_json);

        CHECK(evaluate("true || true", vars) == "true"_json);
        CHECK(evaluate("true || false", vars) == "true"_json);
        CHECK(evaluate("false || true", vars) == "true"_json);
        CHECK(evaluate("false || false", vars) == "false"_json);

        CHECK(evaluate("1 < 2", vars) == "true"_json);
        CHECK(evaluate("1 <= 2", vars) == "true"_json);
        CHECK(evaluate("1 > 2", vars) == "false"_json);
        CHECK(evaluate("1 >= 2", vars) == "false"_json);
        CHECK(evaluate("1 == 2", vars) == "false"_json);
        CHECK(evaluate("1 != 2", vars) == "true"_json);

        CHECK(evaluate("1 < 1", vars) == "false"_json);
        CHECK(evaluate("1 <= 1", vars) == "true"_json);
        CHECK(evaluate("1 > 1", vars) == "false"_json);
        CHECK(evaluate("1 >= 1", vars) == "true"_json);
        CHECK(evaluate("1 == 1", vars) == "true"_json);
        CHECK(evaluate("1 != 1", vars) == "false"_json);
    }

    SECTION("precedence") {
        CHECK(evaluate("true || true && true", vars) == "true"_json);
        CHECK(evaluate("false || true && true", vars) == "true"_json);
        CHECK(evaluate("true || false && true", vars) == "true"_json);
        CHECK(evaluate("true || true && false", vars) == "true"_json);
        CHECK(evaluate("false || false && true", vars) == "false"_json);
        CHECK(evaluate("false || true && false", vars) == "false"_json);
        CHECK(evaluate("true || false && false", vars) == "true"_json);
        CHECK(evaluate("false || false && false", vars) == "false"_json);

        CHECK(evaluate("true && true || true", vars) == "true"_json);
        CHECK(evaluate("true && true || false", vars) == "true"_json);
        CHECK(evaluate("false && true || true", vars) == "true"_json);
        CHECK(evaluate("true && false || true", vars) == "true"_json);
        CHECK(evaluate("false && true || false", vars) == "false"_json);
        CHECK(evaluate("true && false || false", vars) == "false"_json);
        CHECK(evaluate("false && false || true", vars) == "true"_json);
        CHECK(evaluate("false && false || false", vars) == "false"_json);

        CHECK(evaluate("1 < 2 || 1 > 2", vars) == "true"_json);
        CHECK(evaluate("1+1 < 2", vars) == "false"_json);
        CHECK(evaluate("1 < -2", vars) == "false"_json);
    }
}

TEST_CASE("function", "[general]") {
    SECTION("bad") {
        CHECK(!evaluate("foo()").has_value());
        CHECK(!evaluate("abs(").has_value());
        CHECK(!evaluate("abs)").has_value());
        CHECK(!evaluate("abs(1]").has_value());
        CHECK(!evaluate("abs(-1").has_value());
        CHECK(!evaluate("abs-1)").has_value());
        CHECK(!evaluate("abs(-1]").has_value());
        CHECK(!evaluate("abs(-1,2)").has_value());
        CHECK(!evaluate("min(1)").has_value());
    }

    SECTION("unary") {
        CHECK(evaluate("abs(1)") == "1"_json);
        CHECK(evaluate("abs(-1)") == "1"_json);
        CHECK(evaluate("abs (-1)") == "1"_json);
        CHECK(evaluate("abs( -1)") == "1"_json);
        CHECK(evaluate("abs(-1 )") == "1"_json);
        CHECK(evaluate("abs(-1) ") == "1"_json);
        CHECK(evaluate("abs(-abs(-1))") == "1"_json);
    }

    SECTION("binary") {
        CHECK(evaluate("min(1,2)") == "1"_json);
        CHECK(evaluate("min (1,2)") == "1"_json);
        CHECK(evaluate("min( 1,2)") == "1"_json);
        CHECK(evaluate("min(1 ,2)") == "1"_json);
        CHECK(evaluate("min(1, 2)") == "1"_json);
        CHECK(evaluate("min(1,2 )") == "1"_json);
        CHECK(evaluate("min(1,2) ") == "1"_json);
        CHECK(evaluate("min(2,1)") == "1"_json);
        CHECK(evaluate("max(min(5, 3), 2)") == "3"_json);
        CHECK(evaluate("min(2+1,1+3)") == "3"_json);
    }
}

TEST_CASE("stress test", "[general]") {
    SECTION("bad") {
        CHECK(!evaluate("").has_value());
        CHECK(!evaluate("()").has_value());
        CHECK(!evaluate("(").has_value());
        CHECK(!evaluate(")").has_value());
        CHECK(!evaluate(",").has_value());
        CHECK(!evaluate("            ").has_value());
        CHECK(!evaluate("\n\n\n\n\n\n").has_value());
        CHECK(!evaluate("((((((((((((").has_value());
        CHECK(!evaluate("))))))))))))").has_value());
        CHECK(!evaluate("++++++++++++").has_value());
        CHECK(!evaluate("------------").has_value());
        CHECK(!evaluate("************").has_value());
        CHECK(!evaluate("////////////").has_value());
        CHECK(!evaluate(">>>>>>>>>>>>").has_value());
        CHECK(!evaluate("<<<<<<<<<<<<").has_value());
        CHECK(!evaluate("============").has_value());
        CHECK(!evaluate("||||||||||||").has_value());
        CHECK(!evaluate("^^^^^^^^^^^^").has_value());
        CHECK(!evaluate("~~~~~~~~~~~~").has_value());
        CHECK(!evaluate("%%%%%%%%%%%%").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("((((((((((1+2)*3)-4)+2)/2)+6)*2)-7)+1)+0)") == "12"_json);
    }
}

TEST_CASE("wishlist for later", "[future]") {
    variable_registry vars;
    vars["array"]         = "[1,2,3,4,5]"_json;
    vars["object"]        = R"({"a": "b"})"_json;
    vars["nested_array"]  = "[[1,2],[3,4]]"_json;
    vars["nested_object"] = R"([{"a": "b"}, {"a": "c"}])"_json;

    function_registry funcs = default_functions();
    funcs["identity"][1]    = [](const json& j) { return j; };

    // Nested arrays access not supported
    CHECK(!evaluate("nested_array[0][1]", vars, funcs).has_value());
    // Array access on function output not supported
    CHECK(!evaluate("identity(array)[0]", vars, funcs).has_value());
    // Object access on function output not supported
    CHECK(!evaluate("identity(object).a", vars, funcs).has_value());
    // Object access on array access not supported
    CHECK(!evaluate("nested_object[0].c", vars, funcs).has_value());
    // No short-circuiting
    CHECK(!evaluate("size(array) >= 10 && array[9] == 1", vars, funcs).has_value());
    // No array literal
    CHECK(!evaluate("[1,2,3,4]", vars, funcs).has_value());
    // No object literal
    CHECK(!evaluate("{'a':'b'}", vars, funcs).has_value());
}
