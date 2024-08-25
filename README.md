# jsonexpr

## Introduction

Simple expression language implemented in C++, meant to operate on JSON values. It understands:
 - The following types: numbers (float and integers), strings (single or double-quoted), booleans, objects.
 - The usual mathematical operators for numbers (`*` `/` `+` `-`), modulo (`%`) and exponentiation (`^` or `**`).
 - The usual boolean operators (`&&` `||` `!`) and comparison operators (`>` `>=` `<` `<=` `!=` `==`).
 - Array access (`a[1]`).
 - Sub-object access (`a.b`).
 - Custom functions registered in C++ (any arity).
 - Immutable variables registered in C++ (as JSON values).

The intended use it to evaluate constraints on a set of JSON values, or apply simple transformations to JSON values.

Limitations (on purpose):
 - This is not a full blown programming language.
 - There is no assignment or control flow. Just one-liner expressions.
 - Objects are just data (think JSON object), they have no member functions.

Limitations (for lack of motivation/time):
 - Array access is only possible on named variables (`a[1]`), not on function calls (`f(a)[1]`) or nested arrays (`a[1][2]`).
 - Object access is only possible on named variables (`a.b`), not on function calls (`f(a).b`) or nested arrays (`a[1].b`).
 - Arrays and objects can only be created by reading variables (`[1,2,3]` or '{"abc": "def"}` isn't legal code).
 - Boolean operators do not short-circuit, so `(expr1) && (expr2)` will always evaluate both expressions even if the first one evaluates to `false`.


## Basic example usage

```c++
#include <jsonexpr/jsonexpr.hpp>

int main() {
    // Define your JSON objects here.
    jsonexpr::variable_registry vars;

    vars["cat"] = R"({
        "legs": 4, "has_tail": true, "sound": "meow", "colors": ["orange", "black"]
    })"_json;

    vars["bee"] = R"({
        "legs": 6, "has_tail": false, "sound": "bzzz", "colors": ["yellow"]
    })"_json;

    // Evaluate some expressions.
    jsonexpr::evaluate("bee.legs != cat.legs", vars).value();                // true
    jsonexpr::evaluate("bee.has_tail || cat.has_tail", vars).value();        // true
    jsonexpr::evaluate("bee.legs + cat.legs", vars).value();                 // 6
    jsonexpr::evaluate("bee.legs + cat.legs == 12", vars).value();           // false
    jsonexpr::evaluate("min(bee.legs, cat.legs)", vars).value();             // 4
    jsonexpr::evaluate("bee.sound == 'meow'", vars).value();                 // false
    jsonexpr::evaluate("cat.sound == 'meow'", vars).value();                 // true
    jsonexpr::evaluate("cat.sound + bee.sound", vars).value();               // "meowbzzz"
    jsonexpr::evaluate("cat.colors[0]", vars).value();                       // "orange"
    jsonexpr::evaluate("cat.colors[(bee.legs - cat.legs)/2]", vars).value(); // "black"
}
```

## Error handling

The `evaluate()` function returns an "expected" type:
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

To keep the library lightweight, jsonexpr comes with only the most basic functions by default. But this can be extended with custom functions:

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

    // Define some variables to play with.
    jsonexpr::variable_registry vars;
    vars["my_array"] = R"(['some', 'string', 'here'])"_json;

    // Use the function.
    jsonexpr::evaluate("join(my_array, ',')", vars).value(); // "some,string,here"
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
