#ifndef JSONEXPR_PARSE_HPP
#define JSONEXPR_PARSE_HPP

#include "jsonexpr/ast.hpp"
#include "jsonexpr/expected.hpp"

#include <cstdint>

namespace jsonexpr {
expected<ast::node, error> parse(std::string_view expression) noexcept;
} // namespace jsonexpr

#endif
