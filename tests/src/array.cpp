#include "common.hpp"

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
        CHECK(!evaluate("obj[1.0]", vars).has_value());
        CHECK(!evaluate("obj[-6]", vars).has_value());
        CHECK(!evaluate("obj[[0]]", vars).has_value());
        CHECK(!evaluate("obj['a']", vars).has_value());
        CHECK(!evaluate("obj[false]", vars).has_value());
        CHECK(!evaluate("obj[{}]", vars).has_value());
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
        CHECK(evaluate("obj[1]**3", vars) == "8"_json);
        CHECK(evaluate("3**obj[1]", vars) == "9"_json);
    }
}
