#ifndef JSONEXPR_EXPECTED_HPP
#define JSONEXPR_EXPECTED_HPP

#include "jsonexpr/config.hpp"

#if !JSONEXPR_USE_STD_EXPECTED
#    include <tl/expected.hpp>
#endif

namespace jsonexpr {
#if JSONEXPR_USE_STD_EXPECTED
using std::expected;

template<typename T>
std::unexpected<T> unexpected(const T& value) {
    return std::unexpected<T>(value);
}
#else
using tl::expected;

template<typename T>
tl::unexpected<T> unexpected(const T& value) {
    return tl::unexpected<T>(value);
}
#endif
} // namespace jsonexpr

#endif
