#include "jsonexpr/jsonexpr.hpp"

#include <fstream>
#include <iostream>

int main(int argc, const char* argv[]) {
    jsonexpr::variable_registry vars;
    jsonexpr::function_registry funcs = jsonexpr::default_functions();

    std::string_view expression;
    if (argc > 2) {
        expression           = argv[2];
        std::string filename = argv[1];

        std::ifstream f(filename);
        if (!f.is_open()) {
            std::cerr << "error: could not open '" << filename << "' for reading" << std::endl;
            return 1;
        }

        jsonexpr::json data;
        try {
            data = jsonexpr::json::parse(f);
        } catch (const std::exception& e) {
            std::cerr << "error: parsing '" << filename << "': " << e.what() << std::endl;
            return 1;
        }

        if (data.type() != jsonexpr::json::value_t::object) {
            std::cerr << "error: json in input file must be an object" << std::endl;
            return 1;
        }

        for (const auto& [key, value] : data.items()) {
            vars[key] = value;
        }
    } else if (argc == 2) {
        expression = argv[1];
    } else {
        std::cerr << "error: no expression provided" << std::endl;
        return 1;
    }

    const auto result = jsonexpr::evaluate(expression, vars, funcs);
    if (!result.has_value()) {
        std::cerr << jsonexpr::format_error(expression, result.error()) << std::endl;
        return 1;
    }

    std::cout << result.value() << std::endl;
    return 0;
}
