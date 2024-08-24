# jsonexpr

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
 - Objects are just data (think JSON object), no member functions.

Limitations (for lack of motivation/time):
 - Array access is only possible on named variables (`a[1]`), not on function calls (`f(a)[1]`) or nested arrays (`a[1][2]`).
 - Object access is only possible on named variables (`a.b`), not on function calls (`f(a).b`) or nested arrays (`a[1].b`).
