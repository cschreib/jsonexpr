#ifndef JSONEXPR_EXPECTED_HPP
#define JSONEXPR_EXPECTED_HPP

#include "jsonexpr/config.hpp"

#if !JSONEXPR_USE_STD_EXPECTED
#    include <tl/expected.hpp>
#endif

namespace jsonexpr {
#if JSONEXPR_USE_STD_EXPECTED
using std::expected;
using std::unexpected;
#else
using tl::expected;
using tl::unexpected;
#endif
} // namespace jsonexpr

#endif
