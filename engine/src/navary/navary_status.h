#pragma once

#include "navary/result.h"

namespace navary {

enum class NavaryStatus : uint8_t {
  kOk = 0,
  kInvalidArgument,
  kNotFound,
  kOutOfMemory,
  kParseError,
  kIoError,
  kInternal,
};

using NavaryRC = Result<NavaryStatus, NavaryStatus::kOk>;

template <typename T>
using NavaryResult = ResultOr<T, NavaryRC>;

#define NAVARY_RETURN_IF_ERROR(expr) \
  do {                               \
    NavaryRC _st = (expr);           \
    if (!_st.ok()) {                 \
      return _st;                    \
    }                                \
  } while (false)
}  // namespace navary
