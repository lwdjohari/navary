#pragma once

#include <cstdint>

#include "navary/macro.h"

NVR_BEGIN_NAMESPACE

enum class ResultCode : uint32_t {
  kOk = 0,
  kInvalidArgument,
  kNotFound,
  kUnavailable,
  kInternal,
};

NVR_END_NAMESPACE