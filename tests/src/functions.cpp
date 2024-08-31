#include "common.hpp"

TEST_CASE("function", "[general]") {
    function_registry funcs;
    register_function(funcs, "std_except", []() -> json { throw std::runtime_error("bad"); });
    register_function(funcs, "unk_except", []() -> json { throw 1; });

    SECTION("bad") {
        CHECK_ERROR("foo()");
        CHECK_ERROR("abs(");
        CHECK_ERROR("abs)");
        CHECK_ERROR("abs(1]");
        CHECK_ERROR("abs(-1");
        CHECK_ERROR("abs-1)");
        CHECK_ERROR("abs(-1]");
        CHECK_ERROR("abs(-1,2)");
        CHECK_ERROR("min(1)");
        CHECK_ERROR("abs(+)");
        CHECK_ERROR("abs(()");
        CHECK_ERROR("abs(())");
        CHECK_ERROR("abs([)");
        CHECK_ERROR("abs(>)");
        CHECK_ERROR("abs(#)");
        CHECK_ERROR("std_except()", {}, funcs);
        CHECK_ERROR("unk_except()", {}, funcs);
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

TEST_CASE("function registration", "[general]") {
    function_registry funcs;

    SECTION("return type") {
        register_function(funcs, "make_int", []() { return "1"_json; });
        register_function(funcs, "make_float", []() { return "1.0"_json; });
        register_function(funcs, "make_string", []() { return R"("")"_json; });
        register_function(funcs, "make_array", []() { return "[]"_json; });
        register_function(funcs, "make_object", []() { return "{}"_json; });
        register_function(funcs, "make_null", []() { return "null"_json; });

        CHECK(evaluate("make_int()", {}, funcs) == "1"_json);
        CHECK(evaluate("make_float()", {}, funcs) == "1.0"_json);
        CHECK(evaluate("make_string()", {}, funcs) == R"("")"_json);
        CHECK(evaluate("make_array()", {}, funcs) == "[]"_json);
        CHECK(evaluate("make_object()", {}, funcs) == "{}"_json);
        CHECK(evaluate("make_null()", {}, funcs) == "null"_json);
    }

    SECTION("argument type deduction (unary)") {
        register_function(funcs, "one_arg", [](number_integer_t) { return "int"; });
        register_function(funcs, "one_arg", [](number_float_t) { return "float"; });
        register_function(funcs, "one_arg", [](boolean_t) { return "bool"; });
        register_function(funcs, "one_arg", [](const string_t&) { return "string"; });
        register_function(funcs, "one_arg", [](const array_t&) { return "array"; });
        register_function(funcs, "one_arg", [](const object_t&) { return "object"; });
        register_function(funcs, "one_arg", [](null_t) { return "null"; });

        CHECK(evaluate("one_arg(1)", {}, funcs) == "int");
        CHECK(evaluate("one_arg(1.0)", {}, funcs) == "float");
        CHECK(evaluate("one_arg(true)", {}, funcs) == "bool");
        CHECK(evaluate("one_arg('')", {}, funcs) == "string");
        CHECK(evaluate("one_arg([])", {}, funcs) == "array");
        CHECK(evaluate("one_arg({})", {}, funcs) == "object");
        CHECK(evaluate("one_arg(null)", {}, funcs) == "null");
    }

    SECTION("argument type deduction (binary)") {
        // clang-format off
        register_function(funcs, "two_args", [](number_integer_t, number_integer_t) { return "int,int"; });
        register_function(funcs, "two_args", [](number_integer_t, number_float_t) { return "int,float"; });
        register_function(funcs, "two_args", [](number_integer_t, boolean_t) { return "int,bool"; });
        register_function(funcs, "two_args", [](number_integer_t, const string_t&) { return "int,string"; });
        register_function(funcs, "two_args", [](number_integer_t, const array_t&) { return "int,array"; });
        register_function(funcs, "two_args", [](number_integer_t, const object_t&) { return "int,object"; });
        register_function(funcs, "two_args", [](number_integer_t, null_t) { return "int,null"; });
        register_function(funcs, "two_args", [](number_float_t, number_integer_t) { return "float,int"; });
        register_function(funcs, "two_args", [](number_float_t, number_float_t) { return "float,float"; });
        register_function(funcs, "two_args", [](number_float_t, boolean_t) { return "float,bool"; });
        register_function(funcs, "two_args", [](number_float_t, const string_t&) { return "float,string"; });
        register_function(funcs, "two_args", [](number_float_t, const array_t&) { return "float,array"; });
        register_function(funcs, "two_args", [](number_float_t, const object_t&) { return "float,object"; });
        register_function(funcs, "two_args", [](number_float_t, null_t) { return "float,null"; });
        register_function(funcs, "two_args", [](boolean_t, number_integer_t) { return "bool,int"; });
        register_function(funcs, "two_args", [](boolean_t, number_float_t) { return "bool,float"; });
        register_function(funcs, "two_args", [](boolean_t, boolean_t) { return "bool,bool"; });
        register_function(funcs, "two_args", [](boolean_t, const string_t&) { return "bool,string"; });
        register_function(funcs, "two_args", [](boolean_t, const array_t&) { return "bool,array"; });
        register_function(funcs, "two_args", [](boolean_t, const object_t&) { return "bool,object"; });
        register_function(funcs, "two_args", [](boolean_t, null_t) { return "bool,null"; });
        register_function(funcs, "two_args", [](const string_t&, number_integer_t) { return "string,int"; });
        register_function(funcs, "two_args", [](const string_t&, number_float_t) { return "string,float"; });
        register_function(funcs, "two_args", [](const string_t&, boolean_t) { return "string,bool"; });
        register_function(funcs, "two_args", [](const string_t&, const string_t&) { return "string,string"; });
        register_function(funcs, "two_args", [](const string_t&, const array_t&) { return "string,array"; });
        register_function(funcs, "two_args", [](const string_t&, const object_t&) { return "string,object"; });
        register_function(funcs, "two_args", [](const string_t&, null_t) { return "string,null"; });
        register_function(funcs, "two_args", [](const array_t&, number_integer_t) { return "array,int"; });
        register_function(funcs, "two_args", [](const array_t&, number_float_t) { return "array,float"; });
        register_function(funcs, "two_args", [](const array_t&, boolean_t) { return "array,bool"; });
        register_function(funcs, "two_args", [](const array_t&, const string_t&) { return "array,string"; });
        register_function(funcs, "two_args", [](const array_t&, const array_t&) { return "array,array"; });
        register_function(funcs, "two_args", [](const array_t&, const object_t&) { return "array,object"; });
        register_function(funcs, "two_args", [](const array_t&, null_t) { return "array,null"; });
        register_function(funcs, "two_args", [](const object_t&, number_integer_t) { return "object,int"; });
        register_function(funcs, "two_args", [](const object_t&, number_float_t) { return "object,float"; });
        register_function(funcs, "two_args", [](const object_t&, boolean_t) { return "object,bool"; });
        register_function(funcs, "two_args", [](const object_t&, const string_t&) { return "object,string"; });
        register_function(funcs, "two_args", [](const object_t&, const array_t&) { return "object,array"; });
        register_function(funcs, "two_args", [](const object_t&, const object_t&) { return "object,object"; });
        register_function(funcs, "two_args", [](const object_t&, null_t) { return "object,null"; });
        register_function(funcs, "two_args", [](const null_t&, number_integer_t) { return "null,int"; });
        register_function(funcs, "two_args", [](const null_t&, number_float_t) { return "null,float"; });
        register_function(funcs, "two_args", [](const null_t&, boolean_t) { return "null,bool"; });
        register_function(funcs, "two_args", [](const null_t&, const string_t&) { return "null,string"; });
        register_function(funcs, "two_args", [](const null_t&, const array_t&) { return "null,array"; });
        register_function(funcs, "two_args", [](const null_t&, const object_t&) { return "null,object"; });
        register_function(funcs, "two_args", [](const null_t&, null_t) { return "null,null"; });
        // clang-format on

        CHECK(evaluate("two_args(1,1)", {}, funcs) == "int,int");
        CHECK(evaluate("two_args(1,1.0)", {}, funcs) == "int,float");
        CHECK(evaluate("two_args(1,true)", {}, funcs) == "int,bool");
        CHECK(evaluate("two_args(1,'')", {}, funcs) == "int,string");
        CHECK(evaluate("two_args(1,[])", {}, funcs) == "int,array");
        CHECK(evaluate("two_args(1,{})", {}, funcs) == "int,object");
        CHECK(evaluate("two_args(1,null)", {}, funcs) == "int,null");
        CHECK(evaluate("two_args(1.0,1)", {}, funcs) == "float,int");
        CHECK(evaluate("two_args(1.0,1.0)", {}, funcs) == "float,float");
        CHECK(evaluate("two_args(1.0,true)", {}, funcs) == "float,bool");
        CHECK(evaluate("two_args(1.0,'')", {}, funcs) == "float,string");
        CHECK(evaluate("two_args(1.0,[])", {}, funcs) == "float,array");
        CHECK(evaluate("two_args(1.0,{})", {}, funcs) == "float,object");
        CHECK(evaluate("two_args(1.0,null)", {}, funcs) == "float,null");
        CHECK(evaluate("two_args(true,1)", {}, funcs) == "bool,int");
        CHECK(evaluate("two_args(true,1.0)", {}, funcs) == "bool,float");
        CHECK(evaluate("two_args(true,true)", {}, funcs) == "bool,bool");
        CHECK(evaluate("two_args(true,'')", {}, funcs) == "bool,string");
        CHECK(evaluate("two_args(true,[])", {}, funcs) == "bool,array");
        CHECK(evaluate("two_args(true,{})", {}, funcs) == "bool,object");
        CHECK(evaluate("two_args(true,null)", {}, funcs) == "bool,null");
        CHECK(evaluate("two_args('',1)", {}, funcs) == "string,int");
        CHECK(evaluate("two_args('',1.0)", {}, funcs) == "string,float");
        CHECK(evaluate("two_args('',true)", {}, funcs) == "string,bool");
        CHECK(evaluate("two_args('','')", {}, funcs) == "string,string");
        CHECK(evaluate("two_args('',[])", {}, funcs) == "string,array");
        CHECK(evaluate("two_args('',{})", {}, funcs) == "string,object");
        CHECK(evaluate("two_args('',null)", {}, funcs) == "string,null");
        CHECK(evaluate("two_args([],1)", {}, funcs) == "array,int");
        CHECK(evaluate("two_args([],1.0)", {}, funcs) == "array,float");
        CHECK(evaluate("two_args([],true)", {}, funcs) == "array,bool");
        CHECK(evaluate("two_args([],'')", {}, funcs) == "array,string");
        CHECK(evaluate("two_args([],[])", {}, funcs) == "array,array");
        CHECK(evaluate("two_args([],{})", {}, funcs) == "array,object");
        CHECK(evaluate("two_args([],null)", {}, funcs) == "array,null");
        CHECK(evaluate("two_args({},1)", {}, funcs) == "object,int");
        CHECK(evaluate("two_args({},1.0)", {}, funcs) == "object,float");
        CHECK(evaluate("two_args({},true)", {}, funcs) == "object,bool");
        CHECK(evaluate("two_args({},'')", {}, funcs) == "object,string");
        CHECK(evaluate("two_args({},[])", {}, funcs) == "object,array");
        CHECK(evaluate("two_args({},{})", {}, funcs) == "object,object");
        CHECK(evaluate("two_args({},null)", {}, funcs) == "object,null");
        CHECK(evaluate("two_args(null,1)", {}, funcs) == "null,int");
        CHECK(evaluate("two_args(null,1.0)", {}, funcs) == "null,float");
        CHECK(evaluate("two_args(null,true)", {}, funcs) == "null,bool");
        CHECK(evaluate("two_args(null,'')", {}, funcs) == "null,string");
        CHECK(evaluate("two_args(null,[])", {}, funcs) == "null,array");
        CHECK(evaluate("two_args(null,{})", {}, funcs) == "null,object");
        CHECK(evaluate("two_args(null,null)", {}, funcs) == "null,null");
    }

    SECTION("any") {
        register_function(funcs, "any", [](const json&) { return "any"; });

        CHECK(evaluate("any(1)", {}, funcs) == "any");
        CHECK(evaluate("any(1.0)", {}, funcs) == "any");
        CHECK(evaluate("any('')", {}, funcs) == "any");
        CHECK(evaluate("any([])", {}, funcs) == "any");
        CHECK(evaluate("any({})", {}, funcs) == "any");
        CHECK(evaluate("any(null)", {}, funcs) == "any");
    }

    SECTION("any overloads") {
        register_function(funcs, "any", [](const json&) { return "any"; });
        register_function(funcs, "any", [](number_integer_t) { return "int"; });

        CHECK(evaluate("any(1)", {}, funcs) == "int");
        CHECK(evaluate("any(1.0)", {}, funcs) == "any");
        CHECK(evaluate("any('')", {}, funcs) == "any");
        CHECK(evaluate("any([])", {}, funcs) == "any");
        CHECK(evaluate("any({})", {}, funcs) == "any");
        CHECK(evaluate("any(null)", {}, funcs) == "any");
    }

    SECTION("any ambiguous") {
        register_function(funcs, "any", [](const json&, number_integer_t) { return "any1"; });
        register_function(funcs, "any", [](number_integer_t, const json&) { return "any2"; });

        CHECK(evaluate("any(1,1.0)", {}, funcs) == "any2");
        CHECK(evaluate("any(1.0,1)", {}, funcs) == "any1");
        CHECK_ERROR("any(1,1)", {}, funcs);
    }
}

TEST_CASE("min", "[functions]") {
    SECTION("bad") {
        CHECK_ERROR("min()");
        CHECK_ERROR("min(1)");
        CHECK_ERROR("min(1,2,3)");

        CHECK_ERROR("min(1,'a')");
        CHECK_ERROR("min('a',1)");
        CHECK_ERROR("min(1,true)");
        CHECK_ERROR("min(true,1)");
        CHECK_ERROR("min(1,[])");
        CHECK_ERROR("min([],1)");
        CHECK_ERROR("min(1,{})");
        CHECK_ERROR("min({},1)");

        CHECK_ERROR("min('a',true)");
        CHECK_ERROR("min(true,'a')");
        CHECK_ERROR("min('a',[])");
        CHECK_ERROR("min([],'a')");
        CHECK_ERROR("min('a',{})");
        CHECK_ERROR("min({},'a')");

        CHECK_ERROR("min(true,true)");
        CHECK_ERROR("min(true,[])");
        CHECK_ERROR("min([],true)");
        CHECK_ERROR("min(true,{})");
        CHECK_ERROR("min({},true)");

        CHECK_ERROR("min([],[])");
        CHECK_ERROR("min([],{})");
        CHECK_ERROR("min({},[])");

        CHECK_ERROR("min({},{})");
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
        CHECK_ERROR("max()");
        CHECK_ERROR("max(1)");
        CHECK_ERROR("max(1,2,3)");

        CHECK_ERROR("max(1,'a')");
        CHECK_ERROR("max('a',1)");
        CHECK_ERROR("max(1,true)");
        CHECK_ERROR("max(true,1)");
        CHECK_ERROR("max(1,[])");
        CHECK_ERROR("max([],1)");
        CHECK_ERROR("max(1,{})");
        CHECK_ERROR("max({},1)");

        CHECK_ERROR("max('a',true)");
        CHECK_ERROR("max(true,'a')");
        CHECK_ERROR("max('a',[])");
        CHECK_ERROR("max([],'a')");
        CHECK_ERROR("max('a',{})");
        CHECK_ERROR("max({},'a')");

        CHECK_ERROR("max(true,true)");
        CHECK_ERROR("max(true,[])");
        CHECK_ERROR("max([],true)");
        CHECK_ERROR("max(true,{})");
        CHECK_ERROR("max({},true)");

        CHECK_ERROR("max([],[])");
        CHECK_ERROR("max([],{})");
        CHECK_ERROR("max({},[])");

        CHECK_ERROR("max({},{})");
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
        CHECK_ERROR("abs()");
        CHECK_ERROR("abs(1,2)");

        CHECK_ERROR("abs('a')");
        CHECK_ERROR("abs(true)");
        CHECK_ERROR("abs([])");
        CHECK_ERROR("abs({})");
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
        CHECK_ERROR("sqrt()");
        CHECK_ERROR("sqrt(1,2)");

        CHECK_ERROR("sqrt('a')");
        CHECK_ERROR("sqrt(true)");
        CHECK_ERROR("sqrt([])");
        CHECK_ERROR("sqrt({})");
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
        CHECK_ERROR("round()");
        CHECK_ERROR("round(1,2)");

        CHECK_ERROR("round('a')");
        CHECK_ERROR("round(true)");
        CHECK_ERROR("round([])");
        CHECK_ERROR("round({})");
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
        CHECK_ERROR("floor()");
        CHECK_ERROR("floor(1,2)");

        CHECK_ERROR("floor('a')");
        CHECK_ERROR("floor(true)");
        CHECK_ERROR("floor([])");
        CHECK_ERROR("floor({})");
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
        CHECK_ERROR("ceil()");
        CHECK_ERROR("ceil(1,2)");

        CHECK_ERROR("ceil('a')");
        CHECK_ERROR("ceil(true)");
        CHECK_ERROR("ceil([])");
        CHECK_ERROR("ceil({})");
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
        CHECK_ERROR("len()");
        CHECK_ERROR("len('a','b')");

        CHECK_ERROR("len(true)");
        CHECK_ERROR("len(1)");
        CHECK_ERROR("len(1.0)");
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
        CHECK_ERROR("in");
        CHECK_ERROR("1 in");
        CHECK_ERROR("in []");

        CHECK_ERROR("1 in 1");
        CHECK_ERROR("1 in 'a'");
        CHECK_ERROR("1 in {}");
        CHECK_ERROR("1 in null");
        CHECK_ERROR("1 in true");

        CHECK_ERROR("'' in 1");
        CHECK_ERROR("'' in null");
        CHECK_ERROR("'' in true");
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
        CHECK_ERROR("not in");
        CHECK_ERROR("1 not in");
        CHECK_ERROR("not in []");

        CHECK_ERROR("1 not in 1");
        CHECK_ERROR("1 not in 'a'");
        CHECK_ERROR("1 not in {}");

        CHECK_ERROR("'' not in 1");
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
        CHECK_ERROR("int()");
        CHECK_ERROR("int(1,2)");

        CHECK_ERROR("int(0.0/0.0)");
        CHECK_ERROR("int(1.0/0.0)");
        CHECK_ERROR("int(-1.0/0.0)");
        CHECK_ERROR("int(1e40)");
        CHECK_ERROR("int(-1e40)");

        CHECK_ERROR("int([])");
        CHECK_ERROR("int({})");
        CHECK_ERROR("int(null)");

        CHECK_ERROR("int('')");
        CHECK_ERROR("int('abc')");
        CHECK_ERROR("int('1a')");
        CHECK_ERROR("int('1.0')");
        CHECK_ERROR("int(' 1')");
        CHECK_ERROR("int(' +1')");
        CHECK_ERROR("int(' -1')");
        CHECK_ERROR("int('+ 1')");
        CHECK_ERROR("int('- 1')");
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
        CHECK_ERROR("float()");
        CHECK_ERROR("float(1,2)");

        CHECK_ERROR("float([])");
        CHECK_ERROR("float({})");
        CHECK_ERROR("float(null)");

        CHECK_ERROR("float('')");
        CHECK_ERROR("float('abc')");
        CHECK_ERROR("float('1a')");
        CHECK_ERROR("float(' 1')");
        CHECK_ERROR("float(' +1')");
        CHECK_ERROR("float(' -1')");
        CHECK_ERROR("float('+ 1')");
        CHECK_ERROR("float('- 1')");
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
        CHECK_ERROR("bool()");
        CHECK_ERROR("bool(1,2)");

        CHECK_ERROR("bool(0.0/0.0)");
        CHECK_ERROR("bool(1.0/0.0)");
        CHECK_ERROR("bool(-1.0/0.0)");

        CHECK_ERROR("bool([])");
        CHECK_ERROR("bool({})");
        CHECK_ERROR("bool(null)");

        CHECK_ERROR("bool('')");
        CHECK_ERROR("bool('abc')");
        CHECK_ERROR("bool(' true')");
        CHECK_ERROR("bool(' false')");
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
        CHECK_ERROR("str()");
        CHECK_ERROR("str(1,2)");
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
