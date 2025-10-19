//  navary/result.h
//  Part of Navary Core Runtime
//  --------------------------------------------------------------------
//  Purpose:
//    Defines common type aliases for templated Result<ResultCode, kOk>
//    and ResultOr<T, R> types, enabling flexible status handling across
//    various domains (enum-based, int-based, and unsigned int-based).
//
//  Example Usage:
//
//    ResultRC foo();       // enum-based ResultCode version
//    ResultInt bar();      // int-based version (0 == OK)
//    ResultUInt baz();     // unsigned int-based version (0 == OK)
//
//    ResultOrRC<int> v = SomeFunc();     // ResultOr with ResultRC
//    ResultOrInt<std::string> w = Other();
//
//    NVR_RETURN_IF_ERROR(DoSomething());
//    NVR_ASSIGN_OR_RETURN(value, ComputeValue());
//
//  Author: Linggawasistha Djohari
//  Date  : Oct, 2025  
//  --------------------------------------------------------------------

#pragma once

#include <cstdint>

#include "navary/macro.h"
#include "navary/resultcode.h"

NVR_BEGIN_NAMESPACE

// Result<CodeT, kOkValue>: CodeT can be enum or integral.
// Example aliases are provided below.
template <typename CodeT, CodeT kOkValue>
class Result {
 public:
  using CodeType = CodeT;

  constexpr Result() noexcept : code_(kOkValue), msg_(nullptr) {}
  constexpr Result(CodeT code, const char* msg = nullptr) noexcept
      : code_(code), msg_(msg) {}

  static constexpr Result OK() noexcept {
    return {};
  }

  constexpr bool ok() const noexcept {
    return code_ == kOkValue;
  }
  constexpr CodeT code() const noexcept {
    return code_;
  }
  constexpr const char* message() const noexcept {
    return msg_ ? msg_ : "";
  }
  constexpr explicit operator bool() const noexcept {
    return ok();
  }

 private:
  CodeT code_;
  const char* msg_;  // points to static/literal; no ownership
};

// StatusOr<T, R>: R is a Result<...> type (defaults to ResultCode-based alias
// below)
template <typename T, typename R>
class ResultOr {
 public:
  using ResultType = R;

  constexpr ResultOr(const R& s) noexcept : status_(s), has_value_(false) {}
  constexpr ResultOr(T v) noexcept : value_(std::move(v)), has_value_(true) {}

  constexpr bool ok() const noexcept {
    return has_value_;
  }
  constexpr const R& status() const noexcept {
    return status_;
  }

  constexpr T& value() & noexcept {
    return value_;
  }
  constexpr const T& value() const& noexcept {
    return value_;
  }
  constexpr T&& value() && noexcept {
    return std::move(value_);
  }

 private:
  R status_{};
  T value_{};
  bool has_value_{false};
};

// ---------- Generic macros (work with any Result/ResultOr types) ----------
#define NVR_RETURN_IF_ERROR(expr) \
  do {                            \
    auto _s = (expr);             \
    if (!_s.ok())                 \
      return _s;                  \
  } while (0)

#define NVR_ASSIGN_OR_RETURN(lhs, expr)                  \
  do {                                                   \
    auto _tmp_or = (expr);                               \
    if (!_tmp_or.ok())                                   \
      return _tmp_or.status();                           \
    lhs = static_cast<decltype(lhs)&&>(_tmp_or.value()); \
  } while (0)

// ---------- Convenient aliases ----------

// Your original ResultCode-based Result:
using ResultRC = Result<ResultCode, ResultCode::kOk>;

// Int-based Result where 0 means OK:
using ResultInt = Result<int32_t, 0>;

// Unsigned Int-based Result where 0 means OK:
using ResultUInt = Result<uint32_t, 0>;

// Default ResultOr using ResultRC (you can typedef more as needed)
template <typename T>
using ResultOrRC = ResultOr<T, ResultRC>;

template <typename T>
using ResultOrInt = ResultOr<T, ResultInt>;

template <typename T>
using ResultOrUInt = ResultOr<T, ResultUInt>;

// ---------- Helper constructors for ResultRC (enum version) ----------
constexpr ResultRC Invalid(const char* msg) noexcept {
  return ResultRC(ResultCode::kInvalidArgument, msg);
}

constexpr ResultRC Failed(const char* msg) noexcept {
  return ResultRC(ResultCode::kInternal, msg);
}

constexpr ResultRC Unavail(const char* msg) noexcept {
  return ResultRC(ResultCode::kUnavailable, msg);
}

constexpr ResultRC NotFound(const char* msg) noexcept {
  return ResultRC(ResultCode::kNotFound, msg);
}
NVR_END_NAMESPACE
