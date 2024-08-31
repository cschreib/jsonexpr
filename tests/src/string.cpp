#include "common.hpp"

TEST_CASE("string literal", "[string]") {
    SECTION("bad") {
        CHECK_ERROR(R"('")");
        CHECK_ERROR(R"("'')");
        CHECK_ERROR(R"(")");
        CHECK_ERROR(R"(')");
        CHECK_ERROR(R"("abc"def")");
        CHECK_ERROR(R"('abc'def')");
#if !defined(_MSC_VER)
        CHECK_ERROR(R"('\')");
        CHECK_ERROR(R"("\")");
        CHECK_ERROR(R"("0\1")");
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
        CHECK_ERROR("'a'+");
        CHECK_ERROR("+'b'");
        CHECK_ERROR("-'b'");
        CHECK_ERROR("'a'*'b'");
        CHECK_ERROR("'a'/'b'");
        CHECK_ERROR("'a'%'b'");
        CHECK_ERROR("'a'**'b'");
        CHECK_ERROR("'a' and 'b'");
        CHECK_ERROR("'a' or 'b'");
        CHECK_ERROR("'a' < 1");
        CHECK_ERROR("'a' <= 1");
        CHECK_ERROR("'a' > 1");
        CHECK_ERROR("'a' >= 1");
        CHECK_ERROR("not 'a'");
        CHECK_ERROR("a[6]", vars);
        CHECK_ERROR("a[-7]", vars);
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
