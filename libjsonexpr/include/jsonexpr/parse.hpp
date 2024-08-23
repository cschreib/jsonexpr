#ifndef JSONEXPR_PARSE_HPP
#define JSONEXPR_PARSE_HPP

#include "jsonexpr/ast.hpp"
#include "jsonexpr/expected.hpp"

#include <cstdint>

namespace jsonexpr {

struct token {
    source_location location;
    enum {
        IDENTIFIER,
        OPERATOR,
        NUMBER,
        STRING,
        GROUP_OPEN,
        GROUP_CLOSE,
        SEPARATOR,
        ARRAY_ACCESS_OPEN,
        ARRAY_ACCESS_CLOSE
    } type;
};

struct parse_error {
    std::size_t position = 0;
    std::size_t length   = 0;
    std::string message;

    explicit parse_error(std::size_t p, std::size_t l, std::string msg) noexcept :
        position(p), length(l), message(std::move(msg)) {}

    explicit parse_error(const token& t, std::string msg) noexcept :
        position(t.location.position), length(t.location.content.size()), message(std::move(msg)) {}

    explicit parse_error(std::string msg) noexcept : message(std::move(msg)) {}
};

tl::expected<ast::node, parse_error> parse(std::string_view expression) noexcept;
} // namespace jsonexpr

#endif
