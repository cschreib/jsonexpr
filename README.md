# jsonexpr

![Build Status](https://github.com/cschreib/jsonexpr/actions/workflows/cmake.yml/badge.svg) [![codecov](https://codecov.io/gh/cschreib/jsonexpr/graph/badge.svg?token=9XE89TMDJ8)](https://codecov.io/gh/cschreib/jsonexpr)

## Introduction

Simple expression language with Python-like syntax, implemented in C++, and meant to operate on JSON values. It understands:
 - The following types: numbers (float and integers), strings (single or double-quoted), booleans, arrays, objects.
 - The usual mathematical operators for numbers (`*` `/` `+` `-`), modulo (`%`) and exponentiation (`**`).
 - The usual boolean operators (`and` `or` `not`), with short-circuiting.
 - The usual comparison operators (`>` `>=` `<` `<=` `!=` `==`).
 - Array literals (`[1,2,3]`) and array access (`a[1]`), with arbitrary nesting depth.
 - Object literals (`{'a':1, 'b':'c'}`) and sub-object access (`a.b` or `a['b']`).
 - Custom functions registered in C++ (with arbitrary number of arguments).
 - Immutable variables registered in C++ (as JSON values).

The intended use is to evaluate constraints on a set of JSON values, or apply simple transformations to JSON values.

Limitations (on purpose):
 - This is not a full blown programming language.
 - There is no assignment or control flow. Just one-liner expressions.
 - Objects are just data (think JSON object), there is are no classes, inheritance etc.


## Example expression

The example expressions below assume the following JSON values are registered as variables:
 - `cat`:
    ```json
    {"legs": 4, "has_tail": true, "sound": "meow", "colors": ["orange", "black"]}
    ```
 - `bee`:
    ```json
    {"legs": 6, "has_tail": false, "sound": "bzzz", "colors": ["yellow"]}
    ```

Examples (and their value after `->`):
```
bee.legs != cat.legs                  -> true
bee.has_tail or cat.has_tail          -> true
bee.legs + cat.legs                   -> 10
bee.legs + cat.legs == 12             -> false
min(bee.legs, cat.legs)               -> 4
bee.sound == 'meow'                   -> false
cat.sound == 'meow'                   -> true
cat.sound + bee.sound                 -> "meowbzzz"
cat.colors[0]                         -> "orange"
cat.colors[(bee.legs - cat.legs)/2]   -> "black"
```


## Language specification

### General

White spaces are not significant and will be ignored (except inside strings).


### Types

 - Integer number (e.g., `1`). 64 bits signed integer.
 - Floating point number (e.g., `1.0`). 64 bits IEEE floating point number. Special floating point values "infinity" and "not-a-number" are supported.
 - Boolean (e.g., `true` or `false`).
 - String (e.g., `"abc"` or `'abc'`). Encoding is UTF-8. Can be double-quoted (`"a"`) or single-quoted (`'a'`). Otherwise, same escaping rules as for standard JSON (use backslash).
 - Array (e.g., `[1,2,3]`). Contiguous and ordered list of elements. Can be of any length (including zero), and can be heterogeneous (elements of different types). Can be nested.
 - Object (e.g., `{'a':1, 'b':4}`). Key-value dictionary. Keys can only be strings (case-sensitive). Can store any number of entries (including zero), each of different type. Can be nested.
 - Null (e.g., `null`).


### Operators

 - Integers and floating point numbers can be mixed in all operations. If an operation involves an integer and a floating point number, the integer number is first converted to floating point before the operation. Available operations: `+`, `-`, `*`, `/`, `%` (modulo), `**` (exponentiation/power). Integer division and modulo by zero will raise an error. Integer overflow is undefined.
 - Numbers can also be tested for equality (`==` and `!=`) and compared for ordering (`>`, `>=`, `<`, `<=`), with the same conversion rules.
 - Strings can be concatenated with `+`. They can be tested for equality and compared for ordering (using simple alphabetical ordering). Individual characters can be accessed with angled brackets (`'str'[0]`); the index must be an integer, and is zero-based (first character had index zero). Range access is also possible with `str[a:b]` (`a` is the index of the first character to extract, and `b` is 1 plus the index of the last character to extract, so `'abc'[0:2]` is `'ab'`; an empty string is returned if `a >= b`).
 - Booleans can only be tested for equality, and combined with the boolean operators `and` and `or`. The boolean operators are "short-circuiting"; namely, when evaluating `a and b` and `a` evaluates to `false`, `b` will not be evaluated. This allows bypassing evaluation of operations that would otherwise be invalid (e.g. accessing elements beyond the length of an array). Finally, booleans can also be negated (`not a`). No other operation is possible.
 - Arrays can only be tested for equality. Individual elements can be accessed with angled brackets (`[1,2,3][0]`); the index must be an integer, and is zero-based (first character had index zero). Range access is also possible with `arr[a:b]` (`a` is the index of the first element to extract, and `b` is 1 plus the index of the last element to extract, so `[1,2,3][0:2]` is `[1,2]`; an empty array is returned if `a >= b`).
 - Object can only be tested for equality. Sub-objects (or fields, or values) can be accessed with angled brackets (`{'a':1}['a']`); the index must be a string. Equivalently, sub-objects can also be accessed with a single dot (`{'a':1}.a`).
 - Null values can only be tested for equality (and it is always true).


### Default functions

To keep the library lightweight, jsonexpr comes with only the most basic functions by default. This includes:
 - `int(a)`: convert or parse `a` into an `int`.
 - `float(a)`: convert or parse `a` into a `float`.
 - `bool(a)`: convert or parse `a` into a `bool` (for strings: only `"true"` and `"false"` are accepted).
 - `str(a)`: serialise `a` into a `string`.
 - `min(a,b)`: return the minimum of `a` and `b`.
 - `max(a,b)`: return the minimum of `a` and `b`.
 - `abs(a)`: return the absolute value of `a`.
 - `sqrt(a)`: return the square root of `a`.
 - `round(a)`: return the nearest integer value to `a` (preserves the type of `a`).
 - `floor(a)`: return the nearest integer value to `a` (round down, preserves the type of `a`).
 - `ceil(a)`: return nearest integer value to `a` (round up, preserves the type of `a`).
 - `len(a)`: return the size (length) or an array, object, or string.
 - `a in b`: return true if `b` contains `a`, false otherwise.
 - `a not in b`: return true if `b` does not contains `a`, false otherwise.

This list can be extended with your own functions, see below.


### Differences with Python

 - Boolean constants are spelled `true` and `false`, not `True` and `False`.
 - The return value of the modulo operation `%` has the same sign as the *left* operand (in Python, it takes the sign of the *right* operand).
 - When the division operation `/` is used with two integers, this results in integer division (Python's `//`).


## C++ API

### Basic example usage

```c++
#include <jsonexpr/jsonexpr.hpp>

int main() {
    // Define your JSON objects here. These could be read from a file, an HTTP request, ...
    jsonexpr::variable_registry vars;

    vars["cat"] = R"({
        "legs": 4, "has_tail": true, "sound": "meow", "colors": ["orange", "black"]
    })"_json;

    vars["bee"] = R"({
        "legs": 6, "has_tail": false, "sound": "bzzz", "colors": ["yellow"]
    })"_json;

    // Evaluate some expressions.
    jsonexpr::evaluate("bee.legs != cat.legs", vars).value();                // true
    jsonexpr::evaluate("bee.has_tail or cat.has_tail", vars).value();        // true
    jsonexpr::evaluate("bee.legs + cat.legs", vars).value();                 // 10
    jsonexpr::evaluate("bee.legs + cat.legs == 12", vars).value();           // false
    jsonexpr::evaluate("min(bee.legs, cat.legs)", vars).value();             // 4
    jsonexpr::evaluate("bee.sound == 'meow'", vars).value();                 // false
    jsonexpr::evaluate("cat.sound == 'meow'", vars).value();                 // true
    jsonexpr::evaluate("cat.sound + bee.sound", vars).value();               // "meowbzzz"
    jsonexpr::evaluate("cat.colors[0]", vars).value();                       // "orange"
    jsonexpr::evaluate("cat.colors[(bee.legs - cat.legs)/2]", vars).value(); // "black"
}
```

Note: in the example above we simply call `jsonexpr::evaluate(expr, vars)` to immediately get the value of the expression given the current variables. If the same expression needs to be evaluated  multiple times for different sets of variables, the following will be faster:
 - `ast = jsonexpr::parse(expr);` to compile the expression (parse and build the abstract syntax tree)
 - `result = jsonexpr::evaluate(ast, vars);` to evaluate the pre-compiled expression.


## Error handling

The `evaluate()` and `parse()` functions return an "expected" type:
```c++
const auto result = jsonexpr::evaluate(expression);
if (result.has_value()) {
    // Success, can use '.value()':
    std::cout << result.value() << std::endl;
} else {
    // Failure, can't use '.value()', get the error message instead:
    std::cerr << jsonexpr::format_error(expression, result.error()) << std::endl;
}
```

## Custom functions

Custom C++ functions can be registered for use in expressions:

```c++
#include <jsonexpr/jsonexpr.hpp>

int main() {
    jsonexpr::function_registry funcs;
    // Define the function 'join' taking 2 arguments.
    register_function(funcs, "join", 2, [](const jsonexpr::json& args) {
        // Custom functions take a single JSON object as argument,
        // with all actual arguments stored as a JSON array.
        std::string result;
        for (const auto& elem : args[0]) {
            if (!result.empty()) { result += args[1].get<std::string>(); }
            result += elem.get<std::string>();
        }

        return result;
    });

    // Use the function.
    jsonexpr::evaluate("join(['some', 'string', 'here'], ',')").value(); // "some,string,here"
}
```

The above relies on automatic type checks from the JSON library, which is safe but will results in ugly error messages when the types are incorrect. If this is an issue, explicit type checks can be done and custom detailed error messages can be returned:

```c++
#include <jsonexpr/jsonexpr.hpp>

int main() {
    jsonexpr::function_registry funcs;
    register_function(funcs, "join", 2,
        [](const jsonexpr::json& args) -> jsonexpr::basic_function_result {
            if (!args[0].is_array()) {
                return jsonexpr::unexpected(
                    std::string{"expected array as first argument of 'join', got "} +
                    std::string{jsonexpr::get_type_name(args[0])});
            }

            for (std::size_t i = 0; i < args[0].size(); ++i) {
                if (!args[0][i].is_string()) {
                    return jsonexpr::unexpected(
                        std::string{"expected array of strings as first argument of 'join', got "} +
                        std::string{jsonexpr::get_type_name(args[0][i])} + " for element " +
                        std::to_string(i));
                }
            }

            if (!args[1].is_string()) {
                return jsonexpr::unexpected(
                    std::string{"expected string as second argument of 'join', got "} +
                    std::string{jsonexpr::get_type_name(args[1])});
            }

            // All types are now checked, continue with actual logic...
        });

    // ...
}

```
