#include "common.hpp"

TEST_CASE("function", "[general]") {
    function_registry funcs;
    register_function(funcs, "std_except", []() -> json { throw std::runtime_error("bad"); });
    register_function(funcs, "unk_except", []() -> json { throw 1; });

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

TEST_CASE("min", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("min()").has_value());
        CHECK(!evaluate("min(1)").has_value());
        CHECK(!evaluate("min(1,2,3)").has_value());

        CHECK(!evaluate("min(1,'a')").has_value());
        CHECK(!evaluate("min('a',1)").has_value());
        CHECK(!evaluate("min(1,true)").has_value());
        CHECK(!evaluate("min(true,1)").has_value());
        CHECK(!evaluate("min(1,[])").has_value());
        CHECK(!evaluate("min([],1)").has_value());
        CHECK(!evaluate("min(1,{})").has_value());
        CHECK(!evaluate("min({},1)").has_value());

        CHECK(!evaluate("min('a',true)").has_value());
        CHECK(!evaluate("min(true,'a')").has_value());
        CHECK(!evaluate("min('a',[])").has_value());
        CHECK(!evaluate("min([],'a')").has_value());
        CHECK(!evaluate("min('a',{})").has_value());
        CHECK(!evaluate("min({},'a')").has_value());

        CHECK(!evaluate("min(true,true)").has_value());
        CHECK(!evaluate("min(true,[])").has_value());
        CHECK(!evaluate("min([],true)").has_value());
        CHECK(!evaluate("min(true,{})").has_value());
        CHECK(!evaluate("min({},true)").has_value());

        CHECK(!evaluate("min([],[])").has_value());
        CHECK(!evaluate("min([],{})").has_value());
        CHECK(!evaluate("min({},[])").has_value());

        CHECK(!evaluate("min({},{})").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("min(1,2)") == "1"_json);
        CHECK(evaluate("min(2,1)") == "1"_json);
        CHECK(evaluate("min(1.0,2.0)") == "1.0"_json);
        CHECK(evaluate("min(2.0,1.0)") == "1.0"_json);
        CHECK(evaluate("min(1.0,2)") == "1.0"_json);
        CHECK(evaluate("min(2.0,1)") == "1.0"_json);
        CHECK(evaluate("min('a','b')") == R"("a")"_json);
        CHECK(evaluate("min('b','a')") == R"("a")"_json);
    }
}

TEST_CASE("max", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("max()").has_value());
        CHECK(!evaluate("max(1)").has_value());
        CHECK(!evaluate("max(1,2,3)").has_value());

        CHECK(!evaluate("max(1,'a')").has_value());
        CHECK(!evaluate("max('a',1)").has_value());
        CHECK(!evaluate("max(1,true)").has_value());
        CHECK(!evaluate("max(true,1)").has_value());
        CHECK(!evaluate("max(1,[])").has_value());
        CHECK(!evaluate("max([],1)").has_value());
        CHECK(!evaluate("max(1,{})").has_value());
        CHECK(!evaluate("max({},1)").has_value());

        CHECK(!evaluate("max('a',true)").has_value());
        CHECK(!evaluate("max(true,'a')").has_value());
        CHECK(!evaluate("max('a',[])").has_value());
        CHECK(!evaluate("max([],'a')").has_value());
        CHECK(!evaluate("max('a',{})").has_value());
        CHECK(!evaluate("max({},'a')").has_value());

        CHECK(!evaluate("max(true,true)").has_value());
        CHECK(!evaluate("max(true,[])").has_value());
        CHECK(!evaluate("max([],true)").has_value());
        CHECK(!evaluate("max(true,{})").has_value());
        CHECK(!evaluate("max({},true)").has_value());

        CHECK(!evaluate("max([],[])").has_value());
        CHECK(!evaluate("max([],{})").has_value());
        CHECK(!evaluate("max({},[])").has_value());

        CHECK(!evaluate("max({},{})").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("max(1,2)") == "2"_json);
        CHECK(evaluate("max(2,1)") == "2"_json);
        CHECK(evaluate("max(1.0,2.0)") == "2.0"_json);
        CHECK(evaluate("max(2.0,1.0)") == "2.0"_json);
        CHECK(evaluate("max(1.0,2)") == "2.0"_json);
        CHECK(evaluate("max(2.0,1)") == "2.0"_json);
        CHECK(evaluate("max('a','b')") == R"("b")"_json);
        CHECK(evaluate("max('b','a')") == R"("b")"_json);
    }
}

TEST_CASE("abs", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("abs()").has_value());
        CHECK(!evaluate("abs(1,2)").has_value());

        CHECK(!evaluate("abs('a')").has_value());
        CHECK(!evaluate("abs(true)").has_value());
        CHECK(!evaluate("abs([])").has_value());
        CHECK(!evaluate("abs({})").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("abs(1)") == "1"_json);
        CHECK(evaluate("abs(-1)") == "1"_json);
        CHECK(evaluate("abs(0)") == "0"_json);
        CHECK(evaluate("abs(1.0)") == "1.0"_json);
        CHECK(evaluate("abs(-1.0)") == "1.0"_json);
        CHECK(evaluate("abs(0.0)") == "0.0"_json);
    }
}

TEST_CASE("sqrt", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("sqrt()").has_value());
        CHECK(!evaluate("sqrt(1,2)").has_value());

        CHECK(!evaluate("sqrt('a')").has_value());
        CHECK(!evaluate("sqrt(true)").has_value());
        CHECK(!evaluate("sqrt([])").has_value());
        CHECK(!evaluate("sqrt({})").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("sqrt(1.0)") == "1.0"_json);
        CHECK(evaluate("sqrt(4.0)") == "2.0"_json);
        CHECK(evaluate("sqrt(0.0)") == "0.0"_json);
        CHECK(std::isnan(evaluate("sqrt(-1.0)").value().get<double>()));
        CHECK(evaluate("sqrt(1)") == "1.0"_json);
        CHECK(std::isnan(evaluate("sqrt(-1)").value().get<double>()));
    }
}

TEST_CASE("round", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("round()").has_value());
        CHECK(!evaluate("round(1,2)").has_value());

        CHECK(!evaluate("round('a')").has_value());
        CHECK(!evaluate("round(true)").has_value());
        CHECK(!evaluate("round([])").has_value());
        CHECK(!evaluate("round({})").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("round(1.0)") == "1.0"_json);
        CHECK(evaluate("round(1.4)") == "1.0"_json);
        CHECK(evaluate("round(1.5)") == "2.0"_json);
        CHECK(evaluate("round(1.6)") == "2.0"_json);
        CHECK(evaluate("round(2.0)") == "2.0"_json);
        CHECK(evaluate("round(0.0)") == "0.0"_json);
        CHECK(evaluate("round(-1.0)") == "-1.0"_json);
        CHECK(evaluate("round(-1.4)") == "-1.0"_json);
        CHECK(evaluate("round(-1.5)") == "-2.0"_json);
        CHECK(evaluate("round(-1.6)") == "-2.0"_json);
        CHECK(evaluate("round(-2.0)") == "-2.0"_json);
        CHECK(evaluate("round(1)") == "1.0"_json);
        CHECK(evaluate("round(-1)") == "-1.0"_json);
    }
}

TEST_CASE("floor", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("floor()").has_value());
        CHECK(!evaluate("floor(1,2)").has_value());

        CHECK(!evaluate("floor('a')").has_value());
        CHECK(!evaluate("floor(true)").has_value());
        CHECK(!evaluate("floor([])").has_value());
        CHECK(!evaluate("floor({})").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("floor(1.0)") == "1.0"_json);
        CHECK(evaluate("floor(1.4)") == "1.0"_json);
        CHECK(evaluate("floor(1.5)") == "1.0"_json);
        CHECK(evaluate("floor(1.6)") == "1.0"_json);
        CHECK(evaluate("floor(2.0)") == "2.0"_json);
        CHECK(evaluate("floor(0.0)") == "0.0"_json);
        CHECK(evaluate("floor(-1.0)") == "-1.0"_json);
        CHECK(evaluate("floor(-1.4)") == "-2.0"_json);
        CHECK(evaluate("floor(-1.5)") == "-2.0"_json);
        CHECK(evaluate("floor(-1.6)") == "-2.0"_json);
        CHECK(evaluate("floor(-2.0)") == "-2.0"_json);
        CHECK(evaluate("floor(1)") == "1.0"_json);
        CHECK(evaluate("floor(-1)") == "-1.0"_json);
    }
}

TEST_CASE("ceil", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("ceil()").has_value());
        CHECK(!evaluate("ceil(1,2)").has_value());

        CHECK(!evaluate("ceil('a')").has_value());
        CHECK(!evaluate("ceil(true)").has_value());
        CHECK(!evaluate("ceil([])").has_value());
        CHECK(!evaluate("ceil({})").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("ceil(1.0)") == "1.0"_json);
        CHECK(evaluate("ceil(1.4)") == "2.0"_json);
        CHECK(evaluate("ceil(1.5)") == "2.0"_json);
        CHECK(evaluate("ceil(1.6)") == "2.0"_json);
        CHECK(evaluate("ceil(2.0)") == "2.0"_json);
        CHECK(evaluate("ceil(0.0)") == "0.0"_json);
        CHECK(evaluate("ceil(-1.0)") == "-1.0"_json);
        CHECK(evaluate("ceil(-1.4)") == "-1.0"_json);
        CHECK(evaluate("ceil(-1.5)") == "-1.0"_json);
        CHECK(evaluate("ceil(-1.6)") == "-1.0"_json);
        CHECK(evaluate("ceil(-2.0)") == "-2.0"_json);
        CHECK(evaluate("ceil(1)") == "1.0"_json);
        CHECK(evaluate("ceil(-1)") == "-1.0"_json);
    }
}

TEST_CASE("len", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("len()").has_value());
        CHECK(!evaluate("len('a','b')").has_value());

        CHECK(!evaluate("len(true)").has_value());
        CHECK(!evaluate("len(1)").has_value());
        CHECK(!evaluate("len(1.0)").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("len('')") == "0"_json);
        CHECK(evaluate("len('abc')") == "3"_json);
        CHECK(evaluate("len([])") == "0"_json);
        CHECK(evaluate("len([1,2,3])") == "3"_json);
        CHECK(evaluate("len({})") == "0"_json);
        CHECK(evaluate("len({'a':1, 'b':2, 'c':3})") == "3"_json);
    }
}

TEST_CASE("in", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("in").has_value());
        CHECK(!evaluate("1 in").has_value());
        CHECK(!evaluate("in []").has_value());

        CHECK(!evaluate("1 in 1").has_value());
        CHECK(!evaluate("1 in 'a'").has_value());
        CHECK(!evaluate("1 in {}").has_value());
        CHECK(!evaluate("1 in null").has_value());
        CHECK(!evaluate("1 in true").has_value());

        CHECK(!evaluate("'' in 1").has_value());
        CHECK(!evaluate("'' in null").has_value());
        CHECK(!evaluate("'' in true").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("1 in []") == "false"_json);
        CHECK(evaluate("1 in [2,3,4]") == "false"_json);
        CHECK(evaluate("1 in [1,2,3,4]") == "true"_json);
        CHECK(evaluate("'a' in ['b','c','d']") == "false"_json);
        CHECK(evaluate("'e' in ['b','c','d','e']") == "true"_json);
        CHECK(evaluate("'a' in ''") == "false"_json);
        CHECK(evaluate("'a' in 'bcd'") == "false"_json);
        CHECK(evaluate("'e' in 'bec'") == "true"_json);
        CHECK(evaluate("'a' in {}") == "false"_json);
        CHECK(evaluate("'a' in {'b':1}") == "false"_json);
        CHECK(evaluate("'c' in {'b':1,'c':2}") == "true"_json);
    }

    SECTION("precedence") {
        CHECK(evaluate("2+3 in [1,2,3]") == "false"_json);
        CHECK(evaluate("2-3 in [1,2,3]") == "false"_json);
        CHECK(evaluate("2*3 in [1,2,3]") == "false"_json);
        CHECK(evaluate("2/3 in [1,2,3]") == "false"_json);
        CHECK(evaluate("2**3 in [1,2,3]") == "false"_json);
        CHECK(evaluate("2%3 in [1,2,3]") == "true"_json);
        CHECK(evaluate("1<2 in [true,false]") == "true"_json);
        CHECK(evaluate("1<=2 in [true,false]") == "true"_json);
        CHECK(evaluate("1>2 in [true,false]") == "true"_json);
        CHECK(evaluate("1>=2 in [true,false]") == "true"_json);
        CHECK(evaluate("1==2 in [true,false]") == "true"_json);
        CHECK(evaluate("1!=2 in [true,false]") == "true"_json);
        CHECK(evaluate("true and 1 in []") == "false"_json);
        CHECK(evaluate("false or 1 in []") == "false"_json);
    }
}

TEST_CASE("not in", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("not in").has_value());
        CHECK(!evaluate("1 not in").has_value());
        CHECK(!evaluate("not in []").has_value());

        CHECK(!evaluate("1 not in 1").has_value());
        CHECK(!evaluate("1 not in 'a'").has_value());
        CHECK(!evaluate("1 not in {}").has_value());

        CHECK(!evaluate("'' not in 1").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("1 not in []") == "true"_json);
        CHECK(evaluate("1 not in [2,3,4]") == "true"_json);
        CHECK(evaluate("1 not in [1,2,3,4]") == "false"_json);
        CHECK(evaluate("'a' not in ['b','c','d']") == "true"_json);
        CHECK(evaluate("'e' not in ['b','c','d','e']") == "false"_json);
        CHECK(evaluate("'a' not in ''") == "true"_json);
        CHECK(evaluate("'a' not in 'bcd'") == "true"_json);
        CHECK(evaluate("'e' not in 'bec'") == "false"_json);
        CHECK(evaluate("'a' not in {}") == "true"_json);
        CHECK(evaluate("'a' not in {'b':1}") == "true"_json);
        CHECK(evaluate("'c' not in {'b':1,'c':2}") == "false"_json);
    }

    SECTION("precedence") {
        CHECK(evaluate("2+3 not in [1,2,3]") == "true"_json);
        CHECK(evaluate("2-3 not in [1,2,3]") == "true"_json);
        CHECK(evaluate("2*3 not in [1,2,3]") == "true"_json);
        CHECK(evaluate("2/3 not in [1,2,3]") == "true"_json);
        CHECK(evaluate("2**3 not in [1,2,3]") == "true"_json);
        CHECK(evaluate("2%3 not in [1,2,3]") == "false"_json);
        CHECK(evaluate("1<2 not in [true,false]") == "false"_json);
        CHECK(evaluate("1<=2 not in [true,false]") == "false"_json);
        CHECK(evaluate("1>2 not in [true,false]") == "false"_json);
        CHECK(evaluate("1>=2 not in [true,false]") == "false"_json);
        CHECK(evaluate("1==2 not in [true,false]") == "false"_json);
        CHECK(evaluate("1!=2 not in [true,false]") == "false"_json);
        CHECK(evaluate("true and 1 not in []") == "true"_json);
        CHECK(evaluate("false or 1 not in []") == "true"_json);
    }
}

TEST_CASE("int", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("int()").has_value());
        CHECK(!evaluate("int(1,2)").has_value());

        CHECK(!evaluate("int(0.0/0.0)").has_value());
        CHECK(!evaluate("int(1.0/0.0)").has_value());
        CHECK(!evaluate("int(-1.0/0.0)").has_value());
        CHECK(!evaluate("int(1e40)").has_value());
        CHECK(!evaluate("int(-1e40)").has_value());

        CHECK(!evaluate("int([])").has_value());
        CHECK(!evaluate("int({})").has_value());
        CHECK(!evaluate("int(null)").has_value());

        CHECK(!evaluate("int('')").has_value());
        CHECK(!evaluate("int('abc')").has_value());
        CHECK(!evaluate("int('1a')").has_value());
        CHECK(!evaluate("int('1.0')").has_value());
        CHECK(!evaluate("int(' 1')").has_value());
        CHECK(!evaluate("int(' +1')").has_value());
        CHECK(!evaluate("int(' -1')").has_value());
        CHECK(!evaluate("int('+ 1')").has_value());
        CHECK(!evaluate("int('- 1')").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("int(1)") == "1"_json);

        CHECK(evaluate("int(1.4)") == "1"_json);
        CHECK(evaluate("int(1.5)") == "1"_json);
        CHECK(evaluate("int(1.6)") == "1"_json);
        CHECK(evaluate("int(0.0)") == "0"_json);
        CHECK(evaluate("int(-1.4)") == "-1"_json);
        CHECK(evaluate("int(-1.5)") == "-1"_json);
        CHECK(evaluate("int(-1.6)") == "-1"_json);

        CHECK(evaluate("int('0')") == "0"_json);
        CHECK(evaluate("int('1')") == "1"_json);
        CHECK(evaluate("int('01')") == "1"_json);
        CHECK(evaluate("int('+1')") == "1"_json);
        CHECK(evaluate("int('-1')") == "-1"_json);
        CHECK(evaluate("int('1024')") == "1024"_json);
        CHECK(evaluate("int('+1024')") == "1024"_json);
        CHECK(evaluate("int('-1024')") == "-1024"_json);

        CHECK(evaluate("int(true)") == "1"_json);
        CHECK(evaluate("int(false)") == "0"_json);
    }
}

TEST_CASE("float", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("float()").has_value());
        CHECK(!evaluate("float(1,2)").has_value());

        CHECK(!evaluate("float([])").has_value());
        CHECK(!evaluate("float({})").has_value());
        CHECK(!evaluate("float(null)").has_value());

        CHECK(!evaluate("float('')").has_value());
        CHECK(!evaluate("float('abc')").has_value());
        CHECK(!evaluate("float('1a')").has_value());
        CHECK(!evaluate("float(' 1')").has_value());
        CHECK(!evaluate("float(' +1')").has_value());
        CHECK(!evaluate("float(' -1')").has_value());
        CHECK(!evaluate("float('+ 1')").has_value());
        CHECK(!evaluate("float('- 1')").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("float(1.0)") == "1.0"_json);

        CHECK(evaluate("float(1)") == "1.0"_json);
        CHECK(evaluate("float(0)") == "0.0"_json);
        CHECK(evaluate("float(-1)") == "-1.0"_json);

        CHECK(evaluate("float('0')") == "0.0"_json);
        CHECK(evaluate("float('1')") == "1.0"_json);
        CHECK(evaluate("float('+1')") == "1.0"_json);
        CHECK(evaluate("float('-1')") == "-1.0"_json);
        CHECK(evaluate("float('1024')") == "1024.0"_json);
        CHECK(evaluate("float('+1024')") == "1024.0"_json);
        CHECK(evaluate("float('-1024')") == "-1024.0"_json);
        CHECK(evaluate("float('1e5')") == "1e5"_json);
        CHECK(evaluate("float('1e-5')") == "1e-5"_json);
        CHECK(evaluate("float('+1e5')") == "1e5"_json);
        CHECK(evaluate("float('-1e5')") == "-1e5"_json);
        CHECK(
            evaluate("float('inf')").value().get<float>() ==
            std::numeric_limits<json::number_float_t>::infinity());
        CHECK(
            evaluate("float('+inf')").value().get<float>() ==
            std::numeric_limits<json::number_float_t>::infinity());
        CHECK(
            evaluate("float('-inf')").value().get<float>() ==
            -std::numeric_limits<json::number_float_t>::infinity());
        CHECK(std::isnan(evaluate("float('nan')").value().get<float>()));

        CHECK(evaluate("float(true)") == "1.0"_json);
        CHECK(evaluate("float(false)") == "0.0"_json);
    }
}

TEST_CASE("bool", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("bool()").has_value());
        CHECK(!evaluate("bool(1,2)").has_value());

        CHECK(!evaluate("bool(0.0/0.0)").has_value());
        CHECK(!evaluate("bool(1.0/0.0)").has_value());
        CHECK(!evaluate("bool(-1.0/0.0)").has_value());

        CHECK(!evaluate("bool([])").has_value());
        CHECK(!evaluate("bool({})").has_value());
        CHECK(!evaluate("bool(null)").has_value());

        CHECK(!evaluate("bool('')").has_value());
        CHECK(!evaluate("bool('abc')").has_value());
        CHECK(!evaluate("bool(' true')").has_value());
        CHECK(!evaluate("bool(' false')").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("bool(true)") == "true"_json);
        CHECK(evaluate("bool(false)") == "false"_json);

        CHECK(evaluate("bool(1.0)") == "true"_json);
        CHECK(evaluate("bool(0.0)") == "false"_json);
        CHECK(evaluate("bool(-1.0)") == "true"_json);

        CHECK(evaluate("bool(1)") == "true"_json);
        CHECK(evaluate("bool(0)") == "false"_json);
        CHECK(evaluate("bool(-1)") == "true"_json);

        CHECK(evaluate("bool('true')") == "true"_json);
        CHECK(evaluate("bool('false')") == "false"_json);
    }
}

TEST_CASE("str", "[functions]") {
    SECTION("bad") {
        CHECK(!evaluate("str()").has_value());
        CHECK(!evaluate("str(1,2)").has_value());
    }

    SECTION("good") {
        CHECK(evaluate("str('')") == "");
        CHECK(evaluate("str('abc')") == "abc");
        CHECK(evaluate("str(' abc')") == " abc");
        CHECK(evaluate("str('abc ')") == "abc ");

        CHECK(evaluate("str(true)") == "true");
        CHECK(evaluate("str(false)") == "false");

        CHECK(evaluate("str(0)") == "0");
        CHECK(evaluate("str(1)") == "1");
        CHECK(evaluate("str(-1)") == "-1");

        CHECK(evaluate("str(1.0)") == "1.0");
        CHECK(evaluate("str(0.0)") == "0.0");
        CHECK(evaluate("str(-1.0)") == "-1.0");

        CHECK(evaluate("str([])") == "[]");
        CHECK(evaluate("str([ ])") == "[]");
        CHECK(evaluate("str([1])") == "[1]");
        CHECK(evaluate("str([1, 2])") == "[1,2]");

        CHECK(evaluate("str({})") == "{}");
        CHECK(evaluate("str({ })") == "{}");
        CHECK(evaluate("str({'a': 1})") == "{\"a\":1}");
        CHECK(evaluate("str({'a': 1, 'b': [1,2]})") == "{\"a\":1,\"b\":[1,2]}");

        CHECK(evaluate("str(null)") == "null");
    }
}
