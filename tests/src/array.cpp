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
    register_function(funcs, "make_array", []() { return "[1,2,3,4,5]"_json; });

    SECTION("bad") {
        CHECK(!evaluate("foo[1]", vars).has_value());
        CHECK(!evaluate("obj[]", vars).has_value());
        CHECK(!evaluate("obj[", vars).has_value());
        CHECK(!evaluate("obj]", vars).has_value());
        CHECK(!evaluate("obj[1", vars).has_value());
        CHECK(!evaluate("obj[1)", vars).has_value());
        CHECK(!evaluate("obj[1,2]", vars).has_value());
        CHECK(!evaluate("obj[5]", vars).has_value());
        CHECK(!evaluate("obj[15]", vars).has_value());
        CHECK(!evaluate("obj[-15]", vars).has_value());
        CHECK(!evaluate("obj[1106544]", vars).has_value());
        CHECK(!evaluate("obj[-1106544]", vars).has_value());
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
        CHECK(evaluate("make_array()", vars, funcs) == "[1,2,3,4,5]"_json);
        CHECK(evaluate("make_array()[0]", vars, funcs) == "1"_json);
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

TEST_CASE("array range access", "[array]") {
    SECTION("bad") {
        CHECK(!evaluate("[][:").has_value());
        CHECK(!evaluate("[][1:").has_value());
        CHECK(!evaluate("[][*:]").has_value());
        CHECK(!evaluate("[][:*]").has_value());
        CHECK(!evaluate("[][::]").has_value());
        CHECK(!evaluate("foo[:]").has_value());
        CHECK(!evaluate("[][foo:]").has_value());
        CHECK(!evaluate("[][:foo]").has_value());

        CHECK(!evaluate("1[:]").has_value());
        CHECK(!evaluate("1.0[:]").has_value());
        CHECK(!evaluate("true[:]").has_value());
        CHECK(!evaluate("null[:]").has_value());
        CHECK(!evaluate("{}[:]").has_value());
        CHECK(!evaluate("{'abc':1}[:]").has_value());

        CHECK(!evaluate("[1][0:5]").has_value());
        CHECK(!evaluate("[1][5:6]").has_value());
        CHECK(!evaluate("[1,2,3][0:1:2]").has_value());
        CHECK(!evaluate("[1][0:15]").has_value());
        CHECK(!evaluate("[1][-15:-14]").has_value());
        CHECK(!evaluate("[1][0:1106544]").has_value());
        CHECK(!evaluate("[1][-1106544:-1106543]").has_value());

        CHECK(!evaluate("'a'[0:5]").has_value());
        CHECK(!evaluate("'a'[5:6]").has_value());
        CHECK(!evaluate("'abc'[0:1:2]").has_value());
        CHECK(!evaluate("'a'[0:15]").has_value());
        CHECK(!evaluate("'a'[-15:-14]").has_value());
        CHECK(!evaluate("'a'[0:1106544]").has_value());
        CHECK(!evaluate("'a'[-1106544:-1106543]").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("[][:]") == "[]"_json);
        CHECK(evaluate("[1,2,3][:]") == "[1,2,3]"_json);
        CHECK(evaluate("[1,2,3][1:]") == "[2,3]"_json);
        CHECK(evaluate("[1,2,3][:2]") == "[1,2]"_json);
        CHECK(evaluate("[1,2,3][1:2]") == "[2]"_json);
        CHECK(evaluate("[1,2,3][3:2]") == "[]"_json);
        CHECK(evaluate("[1,2,3][0:-1]") == "[1,2]"_json);
        CHECK(evaluate("[1,2,3][-2:-1]") == "[2]"_json);
        CHECK(evaluate("[1,2,3][-2:3]") == "[2,3]"_json);

        CHECK(evaluate("''[:]") == R"("")"_json);
        CHECK(evaluate("'abc'[:]") == R"("abc")"_json);
        CHECK(evaluate("'abc'[1:]") == R"("bc")"_json);
        CHECK(evaluate("'abc'[:2]") == R"("ab")"_json);
        CHECK(evaluate("'abc'[1:2]") == R"("b")"_json);
        CHECK(evaluate("'abc'[3:2]") == R"("")"_json);
        CHECK(evaluate("'abc'[0:-1]") == R"("ab")"_json);
        CHECK(evaluate("'abc'[-2:-1]") == R"("b")"_json);
        CHECK(evaluate("'abc'[-2:3]") == R"("bc")"_json);
    }
}
