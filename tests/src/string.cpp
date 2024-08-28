#include "common.hpp"

TEST_CASE("string literal", "[string]") {
    SECTION("bad") {
        CHECK(!evaluate(R"('")").has_value());
        CHECK(!evaluate(R"("'')").has_value());
        CHECK(!evaluate(R"(")").has_value());
        CHECK(!evaluate(R"(')").has_value());
        CHECK(!evaluate(R"("abc"def")").has_value());
        CHECK(!evaluate(R"('abc'def')").has_value());
#if !defined(_MSC_VER)
        CHECK(!evaluate(R"('\')").has_value());
        CHECK(!evaluate(R"("\")").has_value());
        CHECK(!evaluate(R"("0\1")").has_value());
#endif
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
#if !defined(_MSC_VER)
        CHECK(evaluate(R"("\"")") == R"("\"")"_json);
        CHECK(evaluate(R"('\'')") == R"("'")"_json);
        CHECK(evaluate(R"('0\\1')") == R"("0\\1")"_json);
        CHECK(evaluate(R"('0/1')") == R"("0/1")"_json);
        CHECK(evaluate(R"('0\/1')") == R"("0\/1")"_json);
#endif
    }
}

TEST_CASE("string operations", "[string]") {
    variable_registry vars;
    vars["a"] = R"("abcdef")"_json;

    SECTION("bad") {
        CHECK(!evaluate("'a'+").has_value());
        CHECK(!evaluate("+'b'").has_value());
        CHECK(!evaluate("-'b'").has_value());
        CHECK(!evaluate("'a'*'b'").has_value());
        CHECK(!evaluate("'a'/'b'").has_value());
        CHECK(!evaluate("'a'%'b'").has_value());
        CHECK(!evaluate("'a'**'b'").has_value());
        CHECK(!evaluate("'a' and 'b'").has_value());
        CHECK(!evaluate("'a' or 'b'").has_value());
        CHECK(!evaluate("'a' < 1").has_value());
        CHECK(!evaluate("'a' <= 1").has_value());
        CHECK(!evaluate("'a' > 1").has_value());
        CHECK(!evaluate("'a' >= 1").has_value());
        CHECK(!evaluate("not 'a'").has_value());
        CHECK(!evaluate("a[6]", vars).has_value());
        CHECK(!evaluate("a[-7]", vars).has_value());
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
        CHECK(evaluate("'b' >= 'a'") == "true"_json);
        CHECK(evaluate("a[0]", vars) == R"("a")"_json);
        CHECK(evaluate("a[1]", vars) == R"("b")"_json);
        CHECK(evaluate("a[2]", vars) == R"("c")"_json);
        CHECK(evaluate("a[-1]", vars) == R"("f")"_json);
        CHECK(evaluate("a[-2]", vars) == R"("e")"_json);
        CHECK(evaluate("a[-3]", vars) == R"("d")"_json);
    }
}
