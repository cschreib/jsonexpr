#include "common.hpp"

#include <snitch/snitch_cli.hpp>
#include <snitch/snitch_registry.hpp>

int main(int argc, char* argv[]) {
    std::optional<snitch::cli::input> args = snitch::cli::parse_arguments(argc, argv);
    if (!args) {
        return 1;
    }

    snitch::tests.configure(*args);

    clear_errors();
    return snitch::tests.run_tests(*args) ? 0 : 1;
}
