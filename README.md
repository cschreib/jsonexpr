# jsonexpr

![Build Status](https://github.com/cschreib/jsonexpr/actions/workflows/cmake.yml/badge.svg) [![codecov](https://codecov.io/gh/cschreib/jsonexpr/graph/badge.svg?token=9XE89TMDJ8)](https://codecov.io/gh/cschreib/jsonexpr)

<!-- MarkdownTOC autolink="true" -->

- [Introduction](#introduction)
- [Example expressions](#example-expressions)
- [Language specification](#language-specification)
    - [General](#general)
    - [Types](#types)
    - [Operators](#operators)
    - [Default functions](#default-functions)
    - [Differences with Python](#differences-with-python)
- [C++ API](#c-api)
    - [Basic example usage](#basic-example-usage)
    - [Error handling](#error-handling)
    - [Custom functions](#custom-functions)
        - [Basic example](#basic-example)
        - [Error handling](#error-handling-1)
        - [Overloading](#overloading)

<!-- /MarkdownTOC -->


## Introduction

Simple expression language with Python-like syntax, implemented in C++, and meant to operate on JSON values. It understands:
 - The following types: numbers (float and integers), strings (single or double-quoted), booleans, arrays, objects.
 - The usual mathematical operators for numbers (`*` `/` `+` `-`), modulo (`%`) and exponentiation (`**`).
 - The usual boolean operators (`and` `or` `not`), with short-circuiting.
 - The usual comparison operators (`>` `>=` `<` `<=` `!=` `==`).
 - Array literals (`[1,2,3]`) and array access (`a[1]`), with arbitrary nesting depth.
 - Object literals (`{'a':1, 'b':'c'}`) and sub-object access (`a.b` or `a['b']`).
 - Custom functions with arbitrary number of arguments (registered in C++).
 - Immutable variables as JSON values (registered in C++, e.g., as read from a file or data set).

The intended use is to evaluate constraints on a set of JSON values, or apply simple transformations to JSON values.

Limitations (on purpose):
 - This is not a full blown programming language.
 - There is no assignment or control flow (other than the `a if c else b` ternary expression); the code is just a one-liner expression.
 - Objects are just data (think JSON object), there are no classes (member functions, inheritance, ...).


## Example expressions

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
bee.legs != cat.legs                      -> true
bee.has_tail or cat.has_tail              -> true
bee.legs + cat.legs                       -> 10
bee.legs + cat.legs == 12                 -> false
min(bee.legs, cat.legs)                   -> 4
bee.sound == 'meow'                       -> false
cat.sound == 'meow'                       -> true
cat.sound + bee.sound                     -> "meowbzzz"
cat.sound[0:2] + bee.sound[2:4]           -> "mezz"
cat.colors[0]                             -> "orange"
cat.colors[(bee.legs - cat.legs)/2]       -> "black"
cat.sound if cat.has_tail else bee.sound  -> "meow"
bee.colors[0] in cat.colors               -> false
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
 - Null values can only be tested for equality with null itself (always true) and values of other types (always false). No other operation is possible.

In addition to the above, the following generic operators are also available:
 - `a if c else b`: evaluate and return `a` if condition `c` evaluates to `true`, else evaluate and return `b` (NB: this short-circuits evaluation of the unused operand).
 - `a in b`: return true if `b` (a string, array, or object) contains `a`, false otherwise.
 - `a not in b`: return true if `b` (a string, array, or object) does not contains `a`, false otherwise.


### Default functions

To keep the library lightweight, jsonexpr comes with only the most basic functions by default. This includes:
 - `int(a)`: convert or parse `a` into an `int`.
 - `float(a)`: convert or parse `a` into a `float`.
 - `bool(a)`: convert or parse `a` into a `bool` (for strings: only `"true"` and `"false"` are accepted).
 - `str(a)`: serialise `a` into a `string`.
 - `min(a,b)`: return the minimum of `a` and `b`.
 - `max(a,b)`: return the maximum of `a` and `b`.
 - `abs(a)`: return the absolute value of `a` (preserves the type of `a`).
 - `sqrt(a)`: return the square root of `a` as a floating point value.
 - `round(a)`: return the nearest integer value to `a` (rounding to nearest).
 - `floor(a)`: return the nearest integer value to `a` (rounding down).
 - `ceil(a)`: return nearest integer value to `a` (rounding up).
 - `len(a)`: return the size (length) or an array, object, or string.

This list can be extended with your own functions, see below.


### Differences with Python

 - Boolean constants are spelled `true` and `false`, not `True` and `False`.
 - The null/none value is spelled `null`, not `None`.
 - The return value of the modulo operation `%` has the same sign as the *left* operand (in Python, it takes the sign of the *right* operand).
 - When the division operation `/` is used with two integers, this results in integer division (Python's `//`).
 - The following expressions are not implemented: `a is b`, `x for x in v`, `x for x in v if c`.
 - Bitwise operators are not implemented.


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
    jsonexpr::json result = jsonexpr::evaluate("bee.legs != cat.legs", vars).value();
    // etc.
}
```

JSON values are managed by the [`nlohmann::json`](https://json.nlohmann.me/) library; `jsonexpr::json` is just a type alias for `nlohmann::json`.

Note: in the example above we simply call `jsonexpr::evaluate(expr, vars)` to immediately get the value of the expression given the current variables. If the same expression needs to be evaluated  multiple times for different sets of variables, the following will be faster:
 - `ast = jsonexpr::parse(expr);` to compile the expression (parse and build the abstract syntax tree)
 - `result = jsonexpr::evaluate(ast, vars);` to evaluate the pre-compiled expression.


### Error handling

The `evaluate()` and `parse()` functions return an "[expected](https://en.cppreference.com/w/cpp/utility/expected)" object:
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


### Custom functions

#### Basic example

Custom C++ functions can be registered for use in expressions:

```c++
#include <jsonexpr/jsonexpr.hpp>

int main() {
    // Start with the default function set.
    jsonexpr::function_registry funcs = jsonexpr::default_functions();
    // Define the function 'join' taking 2 arguments.
    jsonexpr::register_function(
        funcs, "join", [](const std::vector<jsonexpr::json>& array, const std::string& separator) {
            std::string result;
            for (const auto& elem : array) {
                if (!result.empty()) {
                    result += separator;
                }
                result += elem.get<std::string>();
            }

            return result;
        });

    // Variable registry (as usual).
    jsonexpr::variable_registry vars;
    // ...

    // Use the function.
    // Note how we now need pass the function registry here:          vvvvv
    jsonexpr::evaluate("join(['some', 'string', 'here'], ',')", vars, funcs).value();
    // Outputs: "some,string,here"

    return 0;
}
```

Supported types for the C++ function parameters:
 - `jsonexpr::number_integer_t = std::int64_t`, for integer numbers.
 - `jsonexpr::number_float_t = double`, for floating-point numbers.
 - `jsonexpr::boolean_t = bool`, for booleans.
 - `jsonexpr::string_t = std::string`, for strings.
 - `jsonexpr::array_t = std::array<jsonexpr::json>`, for arrays.
 - `jsonexpr::object_t = std::unordered_map<std::string, jsonexpr::json>`, for objects.
 - `jsonexpr::null_t = std::nullptr_t`, for null.
 - `jsonexpr::json = nlohmann::json`, for "any of the above" (handle type checks yourself).

The return value must be (convertible to) `jsonexpr::json`, or `jsonexpr::function_result` if handling errors (see below for more information on error handling).


#### Error handling

The library will automatically take care of validating the type of each argument passed to this function, and report appropriate errors in case of a mismatch. However, if the function has error states based on the *values* of the parameters (e.g., here: the length of the array, or the types of the elements within it), these errors need to be handled and reported explicitly. This is done by returning a `jsonexpr::function_result`, which is an alias for `jsonexpr::expected<jsonexpr::json, std::string>` (either a JSON value, or an error message).

In the example below, we check that each element in the array is a string:

```c++
#include <jsonexpr/jsonexpr.hpp>

int main() {
    jsonexpr::function_registry funcs;
    jsonexpr::register_function(
        funcs, "join",
        [](const std::vector<jsonexpr::json>& array,
           const std::string&                 separator) -> jsonexpr::function_result {
            for (std::size_t i = 0; i < array.size(); ++i) {
                if (!array[i].is_string()) {
                    return jsonexpr::unexpected(
                        std::string{"expected array of strings as first argument of 'join', got "} +
                        std::string{jsonexpr::get_type_name(array[i])} + " for element " +
                        std::to_string(i));
                }
            }

            // All conditions are checked, we can proceed...
        });

    // ...
}
```


#### Overloading

It is possible to register more than one function with the same name, provided either the number of arguments or the type of each argument is different. This is done simply by calling `jsonexpr::register_function` multiple times with different function definitions:

```c++
jsonexpr::register_function(
    funcs, "is_empty", [](const std::string& str) { return str.empty(); });
jsonexpr::register_function(
    funcs, "is_empty", [](const std::vector<jsonexpr::json>& array) { return array.empty(); });
```

Note: It is possible to mix overloads with static (as above) and dynamic types (using one or more parameters of type `jsonexpr::json`). In such cases, functions with static types will always be preferred over functions with dynamic types.
