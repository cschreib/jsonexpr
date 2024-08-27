#include <jsonexpr/jsonexpr.hpp>
#include <jsonexpr/parse.hpp>
#include <snitch/snitch_macros_check.hpp>
#include <snitch/snitch_macros_misc.hpp>
#include <snitch/snitch_macros_test_case.hpp>

using namespace jsonexpr;
using namespace snitch::matchers;

namespace snitch {
bool append(snitch::small_string_span ss, const json& j) {
    return append(ss, std::string_view{j.dump()});
}

bool append(snitch::small_string_span ss, const error& e) {
    return append(ss, std::string_view{e.message});
}

bool append(snitch::small_string_span ss, const std::string& s) {
    return append(ss, std::string_view{s});
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

TEST_CASE("type name", "[general]") {
    CHECK(get_type_name("true"_json) == "bool");
    CHECK(get_type_name(true) == "bool");

    CHECK(get_type_name("null"_json) == "null");

    CHECK(get_type_name("1"_json) == "int");
    CHECK(get_type_name("-1"_json) == "int");
    CHECK(get_type_name(std::int64_t{1}) == "int");

    CHECK(get_type_name("1.0"_json) == "float");
    CHECK(get_type_name(1.0) == "float");

    CHECK(get_type_name(R"("abc")"_json) == "string");
    CHECK(get_type_name(std::string("abc")) == "string");

    CHECK(get_type_name("[1,2,3]"_json) == "array");
    CHECK(get_type_name(std::vector<json>{}) == "array");

    CHECK(get_type_name(R"({"a":1})"_json) == "object");
    CHECK(get_type_name(std::unordered_map<std::string, json>{}) == "object");
}

TEST_CASE("format error", "[general]") {
    const auto expression   = "1+x";
    const auto the_error    = error{2, 1, "unknown variable"};
    const auto error_string = format_error(expression, the_error);

    CHECK(error_string == contains_substring{expression});
    CHECK(error_string == contains_substring{the_error.message});
}

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
        CHECK(!evaluate("'a'^'b'").has_value());
        CHECK(!evaluate("'a' && 'b'").has_value());
        CHECK(!evaluate("'a' || 'b'").has_value());
        CHECK(!evaluate("'a' < 1").has_value());
        CHECK(!evaluate("'a' <= 1").has_value());
        CHECK(!evaluate("'a' > 1").has_value());
        CHECK(!evaluate("'a' >= 1").has_value());
        CHECK(!evaluate("!'a'").has_value());
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

    function_registry funcs = default_functions();
    register_function(funcs, "identity", 1, [](const json& j) { return j[0]; });

    SECTION("bad") {
        CHECK(!evaluate("d", vars).has_value());
        CHECK(!evaluate("ab", vars).has_value());
        CHECK(!evaluate("a b", vars).has_value());
    }

    SECTION("good") {
        CHECK(evaluate("a", vars) == "1"_json);
        CHECK(evaluate("b", vars) == "2"_json);
        CHECK(evaluate("a+b", vars) == "3"_json);
        CHECK(evaluate("(a)", vars) == "1"_json);
        CHECK(evaluate("(a)+(b)", vars) == "3"_json);
    }
}

TEST_CASE("array literal", "[array]") {
    SECTION("bad") {
        CHECK(!evaluate("[").has_value());
        CHECK(!evaluate("]").has_value());
        CHECK(!evaluate("[1").has_value());
        CHECK(!evaluate("[1,").has_value());
        CHECK(!evaluate("1]").has_value());
        CHECK(!evaluate(",1]").has_value());
        CHECK(!evaluate("[1+'a']").has_value());
        CHECK(!evaluate("[1,1+'a']").has_value());
        CHECK(!evaluate("[1+'a',1]").has_value());
        CHECK(!evaluate("[#]").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("[]") == "[]"_json);
        CHECK(evaluate("[ ]") == "[ ]"_json);
        CHECK(evaluate("[1]") == "[1]"_json);
        CHECK(evaluate("[1,2]") == "[1,2]"_json);
        CHECK(evaluate("[ 1,2]") == "[1,2]"_json);
        CHECK(evaluate("[1 ,2]") == "[1,2]"_json);
        CHECK(evaluate("[1, 2]") == "[1,2]"_json);
        CHECK(evaluate("[1,2 ]") == "[1,2]"_json);
        CHECK(evaluate("['a']") == R"(["a"])"_json);
        CHECK(evaluate("[1,'a']") == R"([1,"a"])"_json);
        CHECK(evaluate("[1,'a']") == R"([1,"a"])"_json);
        CHECK(evaluate("[[1,2],[3,4]]") == "[[1,2],[3,4]]"_json);
        CHECK(evaluate("[1+2]") == "[3]"_json);
    }
}

TEST_CASE("array access", "[array]") {
    variable_registry vars;
    vars["obj"]          = "[1,2,3,4,5]"_json;
    vars["deep"]         = R"({"sub": [6,7,8]})"_json;
    vars["nested_array"] = "[[1,2],[3,4]]"_json;

    function_registry funcs = default_functions();
    register_function(funcs, "identity", 1, [](const json& j) { return j[0]; });

    SECTION("bad") {
        CHECK(!evaluate("foo[1]", vars).has_value());
        CHECK(!evaluate("obj[]", vars).has_value());
        CHECK(!evaluate("obj[", vars).has_value());
        CHECK(!evaluate("obj]", vars).has_value());
        CHECK(!evaluate("obj[1)", vars).has_value());
        CHECK(!evaluate("obj[1,2]", vars).has_value());
        CHECK(!evaluate("obj[5]", vars).has_value());
        CHECK(!evaluate("obj[-6]", vars).has_value());
        CHECK(!evaluate("obj[[0]]", vars).has_value());
        CHECK(!evaluate("obj['a']", vars).has_value());
        CHECK(!evaluate("obj[+]", vars).has_value());
        CHECK(!evaluate("obj[(]", vars).has_value());
        CHECK(!evaluate("obj[>]", vars).has_value());
        CHECK(!evaluate("obj[#]", vars).has_value());
        CHECK(!evaluate("obj[1+'a']", vars).has_value());
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
        CHECK(evaluate("obj[-1]", vars) == "5"_json);
        CHECK(evaluate("obj[-2]", vars) == "4"_json);
        CHECK(evaluate("obj[-3]", vars) == "3"_json);
        CHECK(evaluate("obj[-4]", vars) == "2"_json);
        CHECK(evaluate("obj[-5]", vars) == "1"_json);
        CHECK(evaluate("obj[1+1]", vars) == "3"_json);
        CHECK(evaluate("obj[round(-1)]", vars) == "5"_json);
        CHECK(evaluate("obj[obj[0]]", vars) == "2"_json);
        CHECK(evaluate("deep.sub[1]", vars) == "7"_json);
        CHECK(evaluate("nested_array[0][0]", vars) == "1"_json);
        CHECK(evaluate("nested_array[0][1]", vars) == "2"_json);
        CHECK(evaluate("nested_array[1][0]", vars) == "3"_json);
        CHECK(evaluate("nested_array[1][1]", vars) == "4"_json);
        CHECK(evaluate("identity(obj)", vars, funcs) == "[1,2,3,4,5]"_json);
        CHECK(evaluate("identity(obj)[0]", vars, funcs) == "1"_json);
        CHECK(evaluate("[[1,2],[3,4]][1][1]", vars) == "4"_json);
    }

    SECTION("precedence") {
        CHECK(evaluate("1+obj[1]", vars) == "3"_json);
        CHECK(evaluate("obj[1]+1", vars) == "3"_json);
        CHECK(evaluate("obj[1]-1", vars) == "1"_json);
        CHECK(evaluate("1-obj[1]", vars) == "-1"_json);
        CHECK(evaluate("obj[1]*2", vars) == "4"_json);
        CHECK(evaluate("2*obj[1]", vars) == "4"_json);
        CHECK(evaluate("obj[1]/2", vars) == "1"_json);
        CHECK(evaluate("2/obj[1]", vars) == "1"_json);
        CHECK(evaluate("obj[1]%2", vars) == "0"_json);
        CHECK(evaluate("2%obj[1]", vars) == "0"_json);
        CHECK(evaluate("obj[1]^3", vars) == "8"_json);
        CHECK(evaluate("3^obj[1]", vars) == "9"_json);
    }
}

TEST_CASE("object literal", "[object]") {
    SECTION("bad") {
        CHECK(!evaluate("{").has_value());
        CHECK(!evaluate("{'a'").has_value());
        CHECK(!evaluate("{'a':").has_value());
        CHECK(!evaluate("{'a',").has_value());
        CHECK(!evaluate("{'a':1").has_value());
        CHECK(!evaluate("{'a':1,").has_value());
        CHECK(!evaluate("{'a':1:'b'").has_value());
        CHECK(!evaluate("1}").has_value());
        CHECK(!evaluate(":1}").has_value());
        CHECK(!evaluate("'a':1}").has_value());
        CHECK(!evaluate(",'a':1}").has_value());
        CHECK(!evaluate("{1:'a'}").has_value());
        CHECK(!evaluate("{1+'a':1}").has_value());
        CHECK(!evaluate("{'a':1+'a'}").has_value());
        CHECK(!evaluate("{#:1}").has_value());
        CHECK(!evaluate("{'a':#}").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("{}") == "{}"_json);
        CHECK(evaluate("{ }") == "{}"_json);
        CHECK(evaluate("{'a':1}") == R"({"a":1})"_json);
        CHECK(evaluate("{ 'a':1}") == R"({"a":1})"_json);
        CHECK(evaluate("{'a' :1}") == R"({"a":1})"_json);
        CHECK(evaluate("{'a': 1}") == R"({"a":1})"_json);
        CHECK(evaluate("{'a':1 }") == R"({"a":1})"_json);
        CHECK(evaluate("{'a':1,'b':2,'c':'d'}") == R"({"a":1,"b":2,"c":"d"})"_json);
        CHECK(evaluate("{'a':{'b':{'c':'d'}}}") == R"({"a":{"b":{"c":"d"}}})"_json);
        CHECK(evaluate("{'a'+'b':'c'+'d'}") == R"({"ab":"cd"})"_json);
        CHECK(evaluate("{'a':'a','a':'b'}") == R"({"a":"b"})"_json);
    }
}

TEST_CASE("object access", "[object]") {
    variable_registry vars;
    vars["obj"]    = R"({"a":1, "b":2, "c":3, "de": 4})"_json;
    vars["nested"] = R"({"a":{"b":{"c":1, "d":2}}})"_json;

    function_registry funcs = default_functions();
    register_function(funcs, "identity", 1, [](const json& j) { return j[0]; });

    SECTION("bad") {
        CHECK(!evaluate("obj['d']", vars).has_value());
        CHECK(!evaluate("obj.d", vars).has_value());
        CHECK(!evaluate("obj[]", vars).has_value());
        CHECK(!evaluate("obj[", vars).has_value());
        CHECK(!evaluate("obj]", vars).has_value());
        CHECK(!evaluate("obj.", vars).has_value());
        CHECK(!evaluate("obj['a')", vars).has_value());
        CHECK(!evaluate("obj['a','b']", vars).has_value());
        CHECK(!evaluate("obj[['a']]", vars).has_value());
        CHECK(!evaluate("obj..a", vars).has_value());
        CHECK(!evaluate("obj[0]", vars).has_value());
        CHECK(!evaluate("obj[+]", vars).has_value());
        CHECK(!evaluate("obj[(]", vars).has_value());
        CHECK(!evaluate("obj[>]", vars).has_value());
        CHECK(!evaluate("obj[#]", vars).has_value());
    }

    SECTION("good") {
        CHECK(evaluate("obj['a']", vars) == "1"_json);
        CHECK(evaluate("obj ['a']", vars) == "1"_json);
        CHECK(evaluate("obj[ 'a']", vars) == "1"_json);
        CHECK(evaluate("obj['a' ]", vars) == "1"_json);
        CHECK(evaluate("obj['b']", vars) == "2"_json);
        CHECK(evaluate("obj['c']", vars) == "3"_json);
        CHECK(evaluate("obj.a", vars) == "1"_json);
        CHECK(evaluate("obj.b", vars) == "2"_json);
        CHECK(evaluate("obj.c", vars) == "3"_json);
        CHECK(evaluate("obj['d'+'e']", vars) == "4"_json);
        CHECK(evaluate("nested['a']", vars) == R"({"b":{"c":1, "d":2}})"_json);
        CHECK(evaluate("nested['a']['b']", vars) == R"({"c":1, "d":2})"_json);
        CHECK(evaluate("nested['a']['b']['c']", vars) == "1"_json);
        CHECK(evaluate("nested['a']['b']['d']", vars) == "2"_json);
        CHECK(evaluate("nested.a", vars) == R"({"b":{"c":1, "d":2}})"_json);
        CHECK(evaluate("nested.a.b", vars) == R"({"c":1, "d":2})"_json);
        CHECK(evaluate("nested.a.b.c", vars) == "1"_json);
        CHECK(evaluate("nested.a.b.d", vars) == "2"_json);
        CHECK(evaluate("identity(obj)", vars, funcs) == R"({"a":1, "b":2, "c":3, "de": 4})"_json);
        CHECK(evaluate("identity(obj)['a']", vars, funcs) == "1"_json);
        CHECK(evaluate("identity(obj).a", vars, funcs) == "1"_json);
        CHECK(evaluate("{'a1':{'b1':1},'a2':{'b1':3}}['a2']['b1']", vars) == "3"_json);
        CHECK(evaluate("{'a1':{'b1':1},'a2':{'b1':3}}.a2.b1", vars) == "3"_json);
    }

    SECTION("precedence") {
        CHECK(evaluate("1+obj['b']", vars) == "3"_json);
        CHECK(evaluate("obj['b']+1", vars) == "3"_json);
        CHECK(evaluate("obj['b']-1", vars) == "1"_json);
        CHECK(evaluate("1-obj['b']", vars) == "-1"_json);
        CHECK(evaluate("obj['b']*2", vars) == "4"_json);
        CHECK(evaluate("2*obj['b']", vars) == "4"_json);
        CHECK(evaluate("obj['b']/2", vars) == "1"_json);
        CHECK(evaluate("2/obj['b']", vars) == "1"_json);
        CHECK(evaluate("obj['b']%2", vars) == "0"_json);
        CHECK(evaluate("2%obj['b']", vars) == "0"_json);
        CHECK(evaluate("obj['b']^3", vars) == "8"_json);
        CHECK(evaluate("3^obj['b']", vars) == "9"_json);

        CHECK(evaluate("1+obj.b", vars) == "3"_json);
        CHECK(evaluate("obj.b+1", vars) == "3"_json);
        CHECK(evaluate("obj.b-1", vars) == "1"_json);
        CHECK(evaluate("1-obj.b", vars) == "-1"_json);
        CHECK(evaluate("obj.b*2", vars) == "4"_json);
        CHECK(evaluate("2*obj.b", vars) == "4"_json);
        CHECK(evaluate("obj.b/2", vars) == "1"_json);
        CHECK(evaluate("2/obj.b", vars) == "1"_json);
        CHECK(evaluate("obj.b%2", vars) == "0"_json);
        CHECK(evaluate("2%obj.b", vars) == "0"_json);
        CHECK(evaluate("obj.b^3", vars) == "8"_json);
        CHECK(evaluate("3^obj.b", vars) == "9"_json);
    }
}

TEST_CASE("boolean", "[maths]") {
    SECTION("bad") {
        CHECK(!evaluate("&").has_value());
        CHECK(!evaluate("&&").has_value());
        CHECK(!evaluate("|").has_value());
        CHECK(!evaluate("||").has_value());
        CHECK(!evaluate("!").has_value());
        CHECK(!evaluate("!=").has_value());
        CHECK(!evaluate("==").has_value());
        CHECK(!evaluate("=").has_value());
        CHECK(!evaluate(">").has_value());
        CHECK(!evaluate("<").has_value());
        CHECK(!evaluate("<=").has_value());
        CHECK(!evaluate(">=").has_value());
        CHECK(!evaluate("=<").has_value());
        CHECK(!evaluate("=>").has_value());
        CHECK(!evaluate("&& &&").has_value());
        CHECK(!evaluate("true &&").has_value());
        CHECK(!evaluate("&& true").has_value());
        CHECK(!evaluate("|| ||").has_value());
        CHECK(!evaluate("true ||").has_value());
        CHECK(!evaluate("|| true").has_value());
        CHECK(!evaluate("> >").has_value());
        CHECK(!evaluate("true >").has_value());
        CHECK(!evaluate("> true").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("!true") == "false"_json);
        CHECK(evaluate("!false") == "true"_json);

        CHECK(evaluate("not true") == "false"_json);
        CHECK(evaluate("not false") == "true"_json);

        CHECK(evaluate("true && true") == "true"_json);
        CHECK(evaluate("true && false") == "false"_json);
        CHECK(evaluate("false && true") == "false"_json);
        CHECK(evaluate("false && false") == "false"_json);

        CHECK(evaluate("true and true") == "true"_json);
        CHECK(evaluate("true and false") == "false"_json);
        CHECK(evaluate("false and true") == "false"_json);
        CHECK(evaluate("false and false") == "false"_json);

        CHECK(evaluate("true || true") == "true"_json);
        CHECK(evaluate("true || false") == "true"_json);
        CHECK(evaluate("false || true") == "true"_json);
        CHECK(evaluate("false || false") == "false"_json);

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
        CHECK(evaluate("true || true && true") == "true"_json);
        CHECK(evaluate("false || true && true") == "true"_json);
        CHECK(evaluate("true || false && true") == "true"_json);
        CHECK(evaluate("true || true && false") == "true"_json);
        CHECK(evaluate("false || false && true") == "false"_json);
        CHECK(evaluate("false || true && false") == "false"_json);
        CHECK(evaluate("true || false && false") == "true"_json);
        CHECK(evaluate("false || false && false") == "false"_json);

        CHECK(evaluate("true && true || true") == "true"_json);
        CHECK(evaluate("true && true || false") == "true"_json);
        CHECK(evaluate("false && true || true") == "true"_json);
        CHECK(evaluate("true && false || true") == "true"_json);
        CHECK(evaluate("false && true || false") == "false"_json);
        CHECK(evaluate("true && false || false") == "false"_json);
        CHECK(evaluate("false && false || true") == "true"_json);
        CHECK(evaluate("false && false || false") == "false"_json);

        CHECK(evaluate("1 < 2 || 1 > 2") == "true"_json);
        CHECK(evaluate("1+1 < 2") == "false"_json);
        CHECK(evaluate("1 < -2") == "false"_json);
    }

    SECTION("short-circuit") {
        CHECK(!evaluate("error()").has_value());

        CHECK(evaluate("false && error()") == "false"_json);
        CHECK(!evaluate("error() && false").has_value());

        CHECK(evaluate("true || error()") == "true"_json);
        CHECK(!evaluate("error() || true").has_value());

        CHECK(evaluate("true && false && error()") == "false"_json);
        CHECK(evaluate("false && true && error()") == "false"_json);
        CHECK(evaluate("false && error() && true") == "false"_json);
        CHECK(!evaluate("error() && false && true").has_value());
        CHECK(!evaluate("error() && true && false").has_value());
        CHECK(!evaluate("true && error() && false").has_value());

        CHECK(evaluate("true || false || error()") == "true"_json);
        CHECK(evaluate("false || true || error()") == "true"_json);
        CHECK(evaluate("true || error() || false") == "true"_json);
        CHECK(!evaluate("error() || false || true").has_value());
        CHECK(!evaluate("error() || true || false").has_value());
        CHECK(!evaluate("false || error() || true").has_value());
    }
}

TEST_CASE("null", "[general]") {
    CHECK(evaluate("null") == "null"_json);
}

TEST_CASE("function", "[general]") {
    function_registry funcs;
    register_function(
        funcs, "std_except", 0, [](const json&) -> json { throw std::runtime_error("bad"); });
    register_function(funcs, "unk_except", 0, [](const json&) -> json { throw 1; });

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
        CHECK(!evaluate("abs(+)").has_value());
        CHECK(!evaluate("abs(()").has_value());
        CHECK(!evaluate("abs(())").has_value());
        CHECK(!evaluate("abs([)").has_value());
        CHECK(!evaluate("abs(>)").has_value());
        CHECK(!evaluate("abs(#)").has_value());
        CHECK(!evaluate("std_except()", {}, funcs).has_value());
        CHECK(!evaluate("unk_except()", {}, funcs).has_value());
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
    variable_registry vars;
    vars["arr"]    = "[1,2,3]"_json;
    vars["int"]    = "1"_json;
    vars["flt"]    = "1.0"_json;
    vars["str"]    = R"("abc")"_json;
    vars["obj"]    = "{}"_json;
    vars["bool"]   = "true"_json;
    vars["objarr"] = R"([{"a": 1}, {"a": 2}, {"d": [3, 4]}])"_json;

    SECTION("bad") {
        CHECK(!evaluate("").has_value());
        CHECK(!evaluate("()").has_value());
        CHECK(!evaluate("(").has_value());
        CHECK(!evaluate("(1").has_value());
        CHECK(!evaluate("(1]").has_value());
        CHECK(!evaluate(")").has_value());
        CHECK(!evaluate("1)").has_value());
        CHECK(!evaluate("[1)").has_value());
        CHECK(!evaluate(",").has_value());
        CHECK(!evaluate("1,").has_value());
        CHECK(!evaluate(",1").has_value());

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

        using namespace std::literals;
        for (const auto& op :
             {"=="s, "!="s, ">"s, ">="s, "<"s, "<="s, "*"s, "/"s, "+"s, "-"s, "%"s, "**"s, "^"s}) {
            CAPTURE(op);

            CHECK(!evaluate("arr " + op + " int", vars).has_value());
            CHECK(!evaluate("arr " + op + " flt", vars).has_value());
            CHECK(!evaluate("arr " + op + " str", vars).has_value());
            CHECK(!evaluate("arr " + op + " obj", vars).has_value());
            CHECK(!evaluate("arr " + op + " bool", vars).has_value());

            CHECK(!evaluate("obj " + op + " int", vars).has_value());
            CHECK(!evaluate("obj " + op + " flt", vars).has_value());
            CHECK(!evaluate("obj " + op + " str", vars).has_value());
            CHECK(!evaluate("obj " + op + " arr", vars).has_value());
            CHECK(!evaluate("obj " + op + " bool", vars).has_value());

            CHECK(!evaluate("bool " + op + " int", vars).has_value());
            CHECK(!evaluate("bool " + op + " flt", vars).has_value());
            CHECK(!evaluate("bool " + op + " str", vars).has_value());
            CHECK(!evaluate("bool " + op + " obj", vars).has_value());
            CHECK(!evaluate("bool " + op + " arr", vars).has_value());

            CHECK(!evaluate("str " + op + " int", vars).has_value());
            CHECK(!evaluate("str " + op + " flt", vars).has_value());
            CHECK(!evaluate("str " + op + " bool", vars).has_value());
            CHECK(!evaluate("str " + op + " obj", vars).has_value());
            CHECK(!evaluate("str " + op + " arr", vars).has_value());
        }

        CHECK(!evaluate("bool < bool", vars).has_value());
        CHECK(!evaluate("bool <= bool", vars).has_value());
        CHECK(!evaluate("bool > bool", vars).has_value());
        CHECK(!evaluate("bool >= bool", vars).has_value());

        CHECK(!evaluate("!int", vars).has_value());
        CHECK(!evaluate("!flt", vars).has_value());
        CHECK(!evaluate("!str", vars).has_value());
        CHECK(!evaluate("!arr", vars).has_value());
        CHECK(!evaluate("!obj", vars).has_value());
    }

    SECTION("good") {
        CHECK(evaluate("((((((((((1+2)*3)-4)+2)/2)+6)*2)-7)+1)+0)") == "12"_json);

        CHECK(evaluate("objarr[0].a", vars) == "1"_json);
        CHECK(evaluate("objarr[1].a", vars) == "2"_json);
        CHECK(evaluate("objarr[2].d[objarr[0].a]", vars) == "4"_json);

        using namespace std::literals;
        for (const auto& op : {"=="s, "!="s, ">"s, ">="s, "<"s, "<="s}) {
            CAPTURE(op);

            CHECK(evaluate("str " + op + " str", vars).has_value());
            CHECK(evaluate("int " + op + " int", vars).has_value());
            CHECK(evaluate("flt " + op + " flt", vars).has_value());
            CHECK(evaluate("arr " + op + " arr", vars).has_value());
            CHECK(evaluate("obj " + op + " obj", vars).has_value());

            CHECK(evaluate("int " + op + " flt", vars).has_value());
            CHECK(evaluate("flt " + op + " int", vars).has_value());
        }

        for (const auto& op : {"+"s, "-"s, "%"s, "**"s, "^"s}) {
            CAPTURE(op);

            CHECK(evaluate("int " + op + " int", vars).has_value());
            CHECK(evaluate("flt " + op + " flt", vars).has_value());
            CHECK(evaluate("int " + op + " flt", vars).has_value());
            CHECK(evaluate("flt " + op + " int", vars).has_value());
        }

        CHECK(evaluate("bool == bool", vars).has_value());
        CHECK(evaluate("bool != bool", vars).has_value());
        CHECK(evaluate("!bool", vars).has_value());

        CHECK(evaluate("str + str", vars).has_value());
    }
}

TEST_CASE("readme examples", "[general]") {
    jsonexpr::variable_registry vars;

    vars["cat"] = R"({
        "legs": 4, "has_tail": true, "sound": "meow", "colors": ["orange", "black"]
    })"_json;

    vars["bee"] = R"({
        "legs": 6, "has_tail": false, "sound": "bzzz", "colors": ["yellow"]
    })"_json;

    CHECK(evaluate("bee.legs != cat.legs", vars) == "true"_json);
    CHECK(evaluate("bee.has_tail || cat.has_tail", vars) == "true"_json);
    CHECK(evaluate("bee.legs + cat.legs", vars) == "10"_json);
    CHECK(evaluate("bee.legs + cat.legs == 12", vars) == "false"_json);
    CHECK(evaluate("min(bee.legs, cat.legs)", vars) == "4"_json);
    CHECK(evaluate("bee.sound == 'meow'", vars) == "false"_json);
    CHECK(evaluate("cat.sound == 'meow'", vars) == "true"_json);
    CHECK(evaluate("cat.sound + bee.sound", vars) == R"("meowbzzz")"_json);
    CHECK(evaluate("cat.colors[0]", vars) == R"("orange")"_json);
    CHECK(evaluate("cat.colors[(bee.legs - cat.legs)/2]", vars) == R"("black")"_json);
}

TEST_CASE("dump", "[debug]") {
    // Testing for coverage only. This is just debug utilities, we don't care as much.
    const auto expected = "object({\n"
                          "  literal(\"a\") :\n"
                          "  array({\n"
                          "    function(min, args={\n"
                          "      literal(1)\n"
                          "      identifier(a)})})})";

    CHECK(dump(parse("{'a': [min(1,a)]}").value()) == expected);
}
