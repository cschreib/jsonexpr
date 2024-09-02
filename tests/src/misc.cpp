#include "common.hpp"

TEST_CASE("null", "[general]") {
    CHECK(evaluate("null") == "null"_json);

    CHECK(evaluate("null == null") == "true"_json);
    CHECK(evaluate("null != null") == "false"_json);

    using namespace std::literals;
    for (const auto& val : {"1"s, "1.0"s, "''"s, "[]"s, "{}"s, "false"s}) {
        CHECK(evaluate(val + " == null") == "false"_json);
        CHECK(evaluate("null == " + val) == "false"_json);
        CHECK(evaluate(val + " != null") == "true"_json);
        CHECK(evaluate("null != " + val) == "true"_json);
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
    CHECK(evaluate("bee.has_tail or cat.has_tail", vars) == "true"_json);
    CHECK(evaluate("bee.legs + cat.legs", vars) == "10"_json);
    CHECK(evaluate("bee.legs + cat.legs == 12", vars) == "false"_json);
    CHECK(evaluate("min(bee.legs, cat.legs)", vars) == "4"_json);
    CHECK(evaluate("bee.sound == 'meow'", vars) == "false"_json);
    CHECK(evaluate("cat.sound == 'meow'", vars) == "true"_json);
    CHECK(evaluate("cat.sound + bee.sound", vars) == R"("meowbzzz")"_json);
    CHECK(evaluate("cat.sound[0:2] + bee.sound[2:4]", vars) == R"("mezz")"_json);
    CHECK(evaluate("cat.colors[0]", vars) == R"("orange")"_json);
    CHECK(evaluate("cat.colors[(bee.legs - cat.legs)/2]", vars) == R"("black")"_json);
    CHECK(evaluate("cat.sound if cat.has_tail else bee.sound", vars) == R"("meow")"_json);
    CHECK(evaluate("bee.colors[0] in cat.colors", vars) == "false"_json);
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
        CHECK_ERROR("");
        CHECK_ERROR("()");
        CHECK_ERROR("(");
        CHECK_ERROR("(1");
        CHECK_ERROR("(1]");
        CHECK_ERROR(")");
        CHECK_ERROR("1)");
        CHECK_ERROR("[1)");
        CHECK_ERROR(",");
        CHECK_ERROR("1,");
        CHECK_ERROR(",1");

        CHECK_ERROR("            ");
        CHECK_ERROR("\n\n\n\n\n\n");
        CHECK_ERROR("((((((((((((");
        CHECK_ERROR("))))))))))))");
        CHECK_ERROR("++++++++++++");
        CHECK_ERROR("------------");
        CHECK_ERROR("************");
        CHECK_ERROR("////////////");
        CHECK_ERROR(">>>>>>>>>>>>");
        CHECK_ERROR("<<<<<<<<<<<<");
        CHECK_ERROR("============");
        CHECK_ERROR("||||||||||||");
        CHECK_ERROR("~~~~~~~~~~~~");
        CHECK_ERROR("%%%%%%%%%%%%");

        CHECK_ERROR(std::string(4096, '('));
        CHECK_ERROR(std::string(4096, '+'));
        CHECK_ERROR(std::string(4096, '['));
        CHECK_ERROR(std::string(4096, '{'));

        CHECK_ERROR("1 << 2");
        CHECK_ERROR("1 >> 2");
        CHECK_ERROR("1 <> 2");
        CHECK_ERROR("1 >< 2");
        CHECK_ERROR("1 => 2");
        CHECK_ERROR("1 =< 2");
        CHECK_ERROR("1 =! 2");
        CHECK_ERROR("1 = 2");
        CHECK_ERROR("1 += 2");
        CHECK_ERROR("1 -= 2");
        CHECK_ERROR("1 *= 2");
        CHECK_ERROR("1 /= 2");
        CHECK_ERROR("1 %= 2");

        using namespace std::literals;
        for (const auto& op :
             {"=="s, "!="s, ">"s, ">="s, "<"s, "<="s, "*"s, "/"s, "+"s, "-"s, "%"s, "**"s, "and"s,
              "or"s}) {
            CAPTURE(op);

            CHECK_ERROR("arr " + op + " int", vars);
            CHECK_ERROR("arr " + op + " flt", vars);
            CHECK_ERROR("arr " + op + " str", vars);
            CHECK_ERROR("arr " + op + " obj", vars);
            CHECK_ERROR("arr " + op + " bool", vars);

            CHECK_ERROR("obj " + op + " int", vars);
            CHECK_ERROR("obj " + op + " flt", vars);
            CHECK_ERROR("obj " + op + " str", vars);
            CHECK_ERROR("obj " + op + " arr", vars);
            CHECK_ERROR("obj " + op + " bool", vars);

            if (op == "or") {
                vars["bool"] = false; // to avoid short-circuiting.
            }

            CHECK_ERROR("bool " + op + " int", vars);
            CHECK_ERROR("bool " + op + " flt", vars);
            CHECK_ERROR("bool " + op + " str", vars);
            CHECK_ERROR("bool " + op + " obj", vars);
            CHECK_ERROR("bool " + op + " arr", vars);

            CHECK_ERROR("str " + op + " int", vars);
            CHECK_ERROR("str " + op + " flt", vars);
            CHECK_ERROR("str " + op + " bool", vars);
            CHECK_ERROR("str " + op + " obj", vars);
            CHECK_ERROR("str " + op + " arr", vars);

            CHECK_ERROR("int " + op + " str", vars);
            CHECK_ERROR("int " + op + " bool", vars);
            CHECK_ERROR("int " + op + " obj", vars);
            CHECK_ERROR("int " + op + " arr", vars);

            CHECK_ERROR("flt " + op + " str", vars);
            CHECK_ERROR("flt " + op + " bool", vars);
            CHECK_ERROR("flt " + op + " obj", vars);
            CHECK_ERROR("flt " + op + " arr", vars);

            if (op != "==" && op != "!=") {
                CHECK_ERROR("obj " + op + " obj", vars);
                CHECK_ERROR("arr " + op + " arr", vars);
                CHECK_ERROR("null " + op + " null", vars);

                CHECK_ERROR("null " + op + " int", vars);
                CHECK_ERROR("null " + op + " flt", vars);
                CHECK_ERROR("null " + op + " bool", vars);
                CHECK_ERROR("null " + op + " obj", vars);
                CHECK_ERROR("null " + op + " arr", vars);
                CHECK_ERROR("null " + op + " str", vars);

                CHECK_ERROR("int " + op + " null", vars);
                CHECK_ERROR("flt " + op + " null", vars);
                CHECK_ERROR("bool " + op + " null", vars);
                CHECK_ERROR("obj " + op + " null", vars);
                CHECK_ERROR("arr " + op + " null", vars);
                CHECK_ERROR("str " + op + " null", vars);

                if (op != "and" && op != "or") {
                    CHECK_ERROR("bool " + op + " bool", vars);
                }
            }

            if (op == "+" || op == "-") {
                CHECK_ERROR(op + " str", vars);
                CHECK_ERROR(op + " arr", vars);
                CHECK_ERROR(op + " obj", vars);
                CHECK_ERROR(op + " bool", vars);
                CHECK_ERROR(op + " null", vars);
            }
        }

        CHECK_ERROR("not int", vars);
        CHECK_ERROR("not flt", vars);
        CHECK_ERROR("not str", vars);
        CHECK_ERROR("not arr", vars);
        CHECK_ERROR("not obj", vars);
        CHECK_ERROR("not null", vars);
    }

    SECTION("good") {
        CHECK(evaluate("((((((((((1+2)*3)-4)+2)/2)+6)*2)-7)+1)+0)") == "12"_json);

        CHECK(evaluate("objarr[0].a", vars) == "1"_json);
        CHECK(evaluate("objarr[1].a", vars) == "2"_json);
        CHECK(evaluate("objarr[2].d[objarr[0].a]", vars) == "4"_json);

        CHECK(evaluate("+++++++++++++++++1") == "1"_json);
        CHECK(evaluate("++++++++++++++++++1") == "1"_json);
        CHECK(evaluate("-----------------1") == "-1"_json);
        CHECK(evaluate("------------------1") == "1"_json);

        using namespace std::literals;
        for (const auto& op : {"=="s, "!="s, ">"s, ">="s, "<"s, "<="s}) {
            CAPTURE(op);

            CHECK(evaluate("str " + op + " str", vars).has_value());
            CHECK(evaluate("int " + op + " int", vars).has_value());
            CHECK(evaluate("flt " + op + " flt", vars).has_value());

            if (op == "==" || op == "!=") {
                CHECK(evaluate("arr " + op + " arr", vars).has_value());
                CHECK(evaluate("obj " + op + " obj", vars).has_value());
                CHECK(evaluate("bool " + op + " bool", vars).has_value());
                CHECK(evaluate("null " + op + " null", vars).has_value());

                CHECK(evaluate("null " + op + " int", vars).has_value());
                CHECK(evaluate("null " + op + " flt", vars).has_value());
                CHECK(evaluate("null " + op + " bool", vars).has_value());
                CHECK(evaluate("null " + op + " obj", vars).has_value());
                CHECK(evaluate("null " + op + " arr", vars).has_value());
                CHECK(evaluate("null " + op + " str", vars).has_value());

                CHECK(evaluate("int " + op + " null", vars).has_value());
                CHECK(evaluate("flt " + op + " null", vars).has_value());
                CHECK(evaluate("bool " + op + " null", vars).has_value());
                CHECK(evaluate("obj " + op + " null", vars).has_value());
                CHECK(evaluate("arr " + op + " null", vars).has_value());
                CHECK(evaluate("str " + op + " null", vars).has_value());
            }

            CHECK(evaluate("int " + op + " flt", vars).has_value());
            CHECK(evaluate("flt " + op + " int", vars).has_value());
        }

        for (const auto& op : {"+"s, "-"s, "%"s, "**"s}) {
            CAPTURE(op);

            CHECK(evaluate("int " + op + " int", vars).has_value());
            CHECK(evaluate("flt " + op + " flt", vars).has_value());
            CHECK(evaluate("int " + op + " flt", vars).has_value());
            CHECK(evaluate("flt " + op + " int", vars).has_value());

            if (op == "+") {
                CHECK(evaluate("str " + op + " str", vars).has_value());
            }

            if (op == "+" || op == "-") {
                CHECK(evaluate(op + " int", vars).has_value());
                CHECK(evaluate(op + " flt", vars).has_value());
            }
        }

        CHECK(evaluate("not bool", vars).has_value());
        CHECK(evaluate("bool and bool", vars).has_value());
        CHECK(evaluate("bool or bool", vars).has_value());
    }
}
