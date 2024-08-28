#include "common.hpp"

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
