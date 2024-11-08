// Copyright 2018-20 Glyn Matthews.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SKYR_VERSION_HPP
#define SKYR_VERSION_HPP

/// \file skyr/version.hpp

#include <tuple>
#include <string_view>
#include <skyr/config.hpp>

#define SKYR_VERSION_MAJOR 1
#define SKYR_VERSION_MINOR 13
#define SKYR_VERSION_PATCH 0

#define SKYR_VERSION_STRING \
  SKYR_PREPROCESSOR_TO_STRING(SKYR_VERSION_MAJOR) "." \
  SKYR_PREPROCESSOR_TO_STRING(SKYR_VERSION_MINOR)

#define SKYR_RELEASE_STRING \
  SKYR_PREPROCESSOR_TO_STRING(SKYR_VERSION_MAJOR) "." \
  SKYR_PREPROCESSOR_TO_STRING(SKYR_VERSION_MINOR) "." \
  SKYR_PREPROCESSOR_TO_STRING(SKYR_VERSION_PATCH)

namespace skyr {
/// \returns The majo, minor version as a tuple
[[maybe_unused]] static constexpr auto version() noexcept -> std::tuple<int, int> {
  return {SKYR_VERSION_MAJOR, SKYR_VERSION_MINOR};
}

/// \returns The major, minor, patch as a tuple
[[maybe_unused]] static constexpr auto release() noexcept -> std::tuple<int, int, int> {
  return {SKYR_VERSION_MAJOR, SKYR_VERSION_MINOR, SKYR_VERSION_PATCH};
}

/// \returns The version as a string in the form MAJOR.MINOR
[[maybe_unused]] static constexpr auto version_string() noexcept -> std::string_view {
  return SKYR_VERSION_STRING;
}

/// \returns The version as a string in the form MAJOR.MINOR.PATCH
[[maybe_unused]] static constexpr auto release_string() noexcept -> std::string_view {
  return SKYR_RELEASE_STRING;
}
}  // namespace skyr

#endif // SKYR_VERSION_HPP
