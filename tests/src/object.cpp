#include "common.hpp"

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
    register_function(
        funcs, "identity",
        +[]() -> basic_function_result { return R"({"a":1, "b":2, "c":3, "de": 4})"_json; });

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
        CHECK(!evaluate("obj[false]", vars).has_value());
        CHECK(!evaluate("obj[{}]", vars).has_value());
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
        CHECK(evaluate("identity()", vars, funcs) == R"({"a":1, "b":2, "c":3, "de":
        4})"_json);
        CHECK(evaluate("identity()['a']", vars, funcs) == "1"_json);
        CHECK(evaluate("identity().a", vars, funcs) == "1"_json);
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
        CHECK(evaluate("obj['b']**3", vars) == "8"_json);
        CHECK(evaluate("3**obj['b']", vars) == "9"_json);

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
        CHECK(evaluate("obj.b**3", vars) == "8"_json);
        CHECK(evaluate("3**obj.b", vars) == "9"_json);
    }
}
