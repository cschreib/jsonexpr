#include "common.hpp"

TEST_CASE("array literal", "[array]") {
    SECTION("bad") {
        CHECK_ERROR("[");
        CHECK_ERROR("]");
        CHECK_ERROR("[1");
        CHECK_ERROR("[1,");
        CHECK_ERROR("1]");
        CHECK_ERROR(",1]");
        CHECK_ERROR("[1+'a']");
        CHECK_ERROR("[1,1+'a']");
        CHECK_ERROR("[1+'a',1]");
        CHECK_ERROR("[#]");
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
        CHECK_ERROR("foo[1]", vars);
        CHECK_ERROR("obj[]", vars);
        CHECK_ERROR("obj[", vars);
        CHECK_ERROR("obj]", vars);
        CHECK_ERROR("obj[1", vars);
        CHECK_ERROR("obj[1)", vars);
        CHECK_ERROR("obj[1,2]", vars);
        CHECK_ERROR("obj[5]", vars);
        CHECK_ERROR("obj[15]", vars);
        CHECK_ERROR("obj[-15]", vars);
        CHECK_ERROR("obj[1106544]", vars);
        CHECK_ERROR("obj[-1106544]", vars);
        CHECK_ERROR("obj[1.0]", vars);
        CHECK_ERROR("obj[-6]", vars);
        CHECK_ERROR("obj[[0]]", vars);
        CHECK_ERROR("obj['a']", vars);
        CHECK_ERROR("obj[false]", vars);
        CHECK_ERROR("obj[{}]", vars);
        CHECK_ERROR("obj[+]", vars);
        CHECK_ERROR("obj[(]", vars);
        CHECK_ERROR("obj[>]", vars);
        CHECK_ERROR("obj[#]", vars);
        CHECK_ERROR("obj[1+'a']", vars);
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
        CHECK_ERROR("[][:");
        CHECK_ERROR("[][1:");
        CHECK_ERROR("[][1:2");
        CHECK_ERROR("[][*:]");
        CHECK_ERROR("[][:*]");
        CHECK_ERROR("[][::]");
        CHECK_ERROR("foo[:]");
        CHECK_ERROR("[][foo:]");
        CHECK_ERROR("[][:foo]");

        CHECK_ERROR("1[:]");
        CHECK_ERROR("1.0[:]");
        CHECK_ERROR("true[:]");
        CHECK_ERROR("null[:]");
        CHECK_ERROR("{}[:]");
        CHECK_ERROR("{'abc':1}[:]");

        CHECK_ERROR("[1][0:5]");
        CHECK_ERROR("[1][5:6]");
        CHECK_ERROR("[1,2,3][0:1:2]");
        CHECK_ERROR("[1][0:15]");
        CHECK_ERROR("[1][-15:-14]");
        CHECK_ERROR("[1][0:1106544]");
        CHECK_ERROR("[1][-1106544:-1106543]");

        CHECK_ERROR("'a'[0:5]");
        CHECK_ERROR("'a'[5:6]");
        CHECK_ERROR("'abc'[0:1:2]");
        CHECK_ERROR("'a'[0:15]");
        CHECK_ERROR("'a'[-15:-14]");
        CHECK_ERROR("'a'[0:1106544]");
        CHECK_ERROR("'a'[-1106544:-1106543]");
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
