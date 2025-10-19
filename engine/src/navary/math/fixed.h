#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace navary::math {

#if !defined(NDEBUG)
#define NVR_DEBUG 1
#else
#define NVR_DEBUG 0
#endif

/**
 * Fixed<Rep, kFracBits>
 * ---------------------
 * A simple power-of-two fixed-point number:
 *   real_value = raw_ / (1 << kFracBits)
 *
 * Example aliases:
 *   using Fixed15p16 = Fixed<int32_t, 16>;  // ~±32768 range, ~1.5e-5 step
 *
 * Goals:
 *   - Deterministic math across platforms
 *   - Debug-friendly overflow guards
 *   - Multiplying two Fixed uses widened arithmetic, then scales back.
 *   - Floating overloads for operators are intentionally not provided
 *     to avoid implicit float mixing causes nondeterministic behavior.
 */
template <typename Rep, int kFracBits>
class Fixed final {
  static_assert(std::is_integral_v<Rep> && std::is_signed_v<Rep>,
                "Rep must be a signed integer type.");
  static_assert(kFracBits > 0 && kFracBits < (int)(sizeof(Rep) * 8 - 1),
                "kFracBits must be within (0, bitwidth(Rep)-1).");

 public:
  using RepType                      = Rep;
  static constexpr int kFractionBits = kFracBits;
  static constexpr Rep kOneRaw       = static_cast<Rep>(Rep{1} << kFracBits);

  // Constructors
  constexpr Fixed() : raw_(0) {}
  explicit constexpr Fixed(int v) : raw_(static_cast<Rep>(v) << kFracBits) {}

  // Factory: from raw scaled integer
  static constexpr Fixed FromRaw(Rep raw) {
    Fixed f;
    f.raw_ = raw;
    return f;
  }

  // Factory: from integers/floats
  static constexpr Fixed FromInt(int v) {
    return Fixed(v);
  }

  static constexpr Fixed FromFraction(int num, int den) {
    // (num << kFracBits) / den, rounded toward zero
    assert(den != 0);
    using Wide  = std::conditional_t<(sizeof(Rep) <= 4), int64_t, __int128_t>;
    Wide scaled = static_cast<Wide>(num) << kFracBits;
    return FromRaw(static_cast<Rep>(scaled / den));
  }

  static constexpr Fixed FromFloat(float v) {
    if (!std::isfinite(v))
      return Fixed();  // 0
    const float scaled = v * static_cast<float>(kOneRaw);
    return FromRaw(RoundAwayFromZero<Rep>(scaled));
  }

  static constexpr Fixed FromDouble(double v) {
    if (!std::isfinite(v))
      return Fixed();  // 0
    const double scaled = v * static_cast<double>(kOneRaw);
    return FromRaw(RoundAwayFromZero<Rep>(scaled));
  }

  // Constants
  static constexpr Fixed Zero() {
    return FromRaw(0);
  }
  static constexpr Fixed Epsilon() {
    return FromRaw(1);
  }  // LSB of fraction

  // Accessors
  constexpr Rep Raw() const {
    return raw_;
  }

  // Conversions
  float ToFloat() const {
    return static_cast<float>(raw_) / static_cast<float>(kOneRaw);
  }
  
  double ToDouble() const {
    return static_cast<double>(raw_) / static_cast<double>(kOneRaw);
  }

  // Integer rounding modes (return int)
  constexpr int ToIntTrunc() const {  // toward zero
    if (raw_ >= 0)
      return static_cast<int>(raw_ >> kFracBits);
    // (raw_ + (2^k - 1)) >> k to truncate toward zero
    return static_cast<int>((raw_ + (kOneRaw - 1)) >> kFracBits);
  }
  constexpr int ToIntCeil() const {  // toward +inf
    return static_cast<int>((raw_ + (kOneRaw - 1)) >> kFracBits);
  }
  constexpr int ToIntFloor() const {  // toward -inf
    return static_cast<int>(raw_ >> kFracBits);
  }
  constexpr int ToIntNearestTiesUp()
      const {  // ties away from zero (like bankless ceil)
    // add half and shift
    return static_cast<int>((raw_ + (kOneRaw >> 1)) >> kFracBits);
  }

  // Comparisons
  friend constexpr bool operator==(Fixed a, Fixed b) {
    return a.raw_ == b.raw_;
  }

  friend constexpr bool operator!=(Fixed a, Fixed b) {
    return a.raw_ != b.raw_;
  }

  friend constexpr bool operator<(Fixed a, Fixed b) {
    return a.raw_ < b.raw_;
  }

  friend constexpr bool operator<=(Fixed a, Fixed b) {
    return a.raw_ <= b.raw_;
  }

  friend constexpr bool operator>(Fixed a, Fixed b) {
    return a.raw_ > b.raw_;
  }

  friend constexpr bool operator>=(Fixed a, Fixed b) {
    return a.raw_ >= b.raw_;
  }

  // Basic arithmetic (Fixed ± Fixed)
  friend constexpr Fixed operator+(Fixed a, Fixed b) {
    // Debug-ish overflow awareness: signed add overflow check
#if NVR_DEBUG
    if (((b.raw_ > 0) && (a.raw_ > std::numeric_limits<Rep>::max() - b.raw_)) ||
        ((b.raw_ < 0) && (a.raw_ < std::numeric_limits<Rep>::min() - b.raw_))) {
      assert(false && "Fixed add overflow");
    }
#endif
    return FromRaw(static_cast<Rep>(a.raw_ + b.raw_));
  }

  friend constexpr Fixed operator-(Fixed a, Fixed b) {
#if NVR_DEBUG
    if (((b.raw_ < 0) && (a.raw_ > std::numeric_limits<Rep>::max() + b.raw_)) ||
        ((b.raw_ > 0) && (a.raw_ < std::numeric_limits<Rep>::min() + b.raw_))) {
      assert(false && "Fixed sub overflow");
    }
#endif
    return FromRaw(static_cast<Rep>(a.raw_ - b.raw_));
  }
  constexpr Fixed& operator+=(Fixed v) {
    *this = *this + v;
    return *this;
  }
  constexpr Fixed& operator-=(Fixed v) {
    *this = *this - v;
    return *this;
  }

  // Unary
  constexpr Fixed operator-() const {
#if NVR_DEBUG
    if (raw_ == std::numeric_limits<Rep>::min()) {
      assert(false && "Fixed negate overflow");
#endif
    }
    return FromRaw(static_cast<Rep>(-raw_));
  }
  constexpr Fixed operator+() const {
    return *this;
  }

  // Shift raw (advanced, keep scale)
  constexpr Fixed ShiftRight(int n) const {
    assert(n >= 0 && n < static_cast<int>(sizeof(Rep) * 8));
    return FromRaw(static_cast<Rep>(raw_ >> n));
  }
  constexpr Fixed ShiftLeft(int n) const {
    assert(n >= 0 && n < static_cast<int>(sizeof(Rep) * 8));
    // Best-effort debug overflow check
#if NVR_DEBUG
    if (n > 0) {
      const Rep max_before = (std::numeric_limits<Rep>::max() >> n);
      const Rep min_before = (std::numeric_limits<Rep>::min() >> n);
      if (raw_ > max_before || raw_ < min_before) {
        assert(false && "Fixed shift-left overflow");
      }
    }
#endif
    return FromRaw(static_cast<Rep>(raw_ << n));
  }

  // Divide by integer
  friend constexpr Fixed operator/(Fixed a, int b) {
    assert(b != 0);
#if NVR_DEBUG
    if (b == -1 && a.raw_ == std::numeric_limits<Rep>::min()) {
      assert(false && "Fixed / -1 overflow");
    }
#endif
    return FromRaw(static_cast<Rep>(a.raw_ / b));
  }

  // Multiply by integer
  friend constexpr Fixed operator*(Fixed a, int b) {
#if NVR_DEBUG
    using Wide = std::conditional_t<(sizeof(Rep) <= 4), int64_t, __int128_t>;
    Wide w     = static_cast<Wide>(a.raw_) * static_cast<Wide>(b);
    if (w > std::numeric_limits<Rep>::max() ||
        w < std::numeric_limits<Rep>::min()) {
      assert(false && "Fixed * int overflow");
    }
#endif
    return FromRaw(static_cast<Rep>(a.raw_ * b));
  }

  // Fixed × Fixed → Fixed  (widen then downscale)
  // round to nearest (±half before shift) to avoid step-wise truncation bias
  // (round-to-nearest, symmetric)
  friend constexpr Fixed Mul(Fixed a, Fixed b) {
    using Wide  = std::conditional_t<(sizeof(Rep) <= 4), int64_t, __int128_t>;
    using UWide = std::make_unsigned_t<Wide>;
    constexpr Wide kHalf = static_cast<Wide>(Wide{1} << (kFracBits - 1));
    Wide w = static_cast<Wide>(a.raw_) * static_cast<Wide>(b.raw_);
    // Round to nearest: add/sub half depending on sign.
    if (w >= 0)
      w += kHalf;
    else
      w -= kHalf;
    w >>= kFracBits;
#ifndef NDEBUG
    if (w > std::numeric_limits<Rep>::max() ||
        w < std::numeric_limits<Rep>::min()) {
      assert(false && "Fixed Mul overflow");
    }
#endif
    return FromRaw(static_cast<Rep>(w));
  }

  //  MulDiv: compute raw_out = (a.raw * m.raw) / d.raw (no extra >> k)
  friend constexpr Fixed MulDiv(Fixed a, Fixed m, Fixed d) {
    assert(d.raw_ != 0);
    using Wide = std::conditional_t<(sizeof(Rep) <= 4), int64_t, __int128_t>;
    Wide num   = static_cast<Wide>(a.raw_) * static_cast<Wide>(m.raw_);
    // Desired: raw_out = (a.raw * m.raw) / d.raw
    num /= static_cast<Wide>(d.raw_);
#ifndef NDEBUG
    if (num > std::numeric_limits<Rep>::max() ||
        num < std::numeric_limits<Rep>::min()) {
      assert(false && "Fixed MulDiv overflow");
    }
#endif
    return FromRaw(static_cast<Rep>(num));
  }

  // Remainder with sign of divisor (like original intent)
  friend constexpr Fixed Mod(Fixed a, Fixed b) {
    assert(b.raw_ != 0);
    Rep t = static_cast<Rep>(a.raw_ % b.raw_);
    if (b.raw_ > 0 && t < 0)
      t = static_cast<Rep>(t + b.raw_);
    else if (b.raw_ < 0 && t > 0)
      t = static_cast<Rep>(t + b.raw_);
    return FromRaw(t);
  }

  // Integer sqrt on fixed: sqrt(a) where a is fixed
  // Uses Newton iteration on raw scaled to keep result in fixed domain.
  friend Fixed Sqrt(Fixed a) {
    if (a.raw_ <= 0)
      return Zero();
    using UWide =
        std::conditional_t<(sizeof(Rep) <= 4), uint64_t, unsigned __int128>;
    // We want sqrt(a.raw_ << kFracBits)
    UWide scaled =
        static_cast<UWide>(static_cast<std::make_unsigned_t<Rep>>(a.raw_))
        << kFracBits;
    // Integer sqrt via Newton / bit method (simple and deterministic)
    UWide x = scaled;
    UWide y = (x + 1) >> 1;
    while (y < x) {
      x = y;
      y = (x + scaled / x) >> 1;
    }
    return FromRaw(static_cast<Rep>(x));
  }

  // Helpers
  constexpr bool IsZero() const {
    return raw_ == 0;
  }

  // Intentionally delete float operator overloads to avoid accidental
  // promotion.
  Fixed operator*(float) const = delete;
  Fixed operator/(float) const = delete;

 private:
  template <typename R, typename F>
  static constexpr R RoundAwayFromZero(F v) {
    // v can be float/double scaled value; we round away from zero.
    return static_cast<R>(v >= static_cast<F>(0) ? v + static_cast<F>(0.5)
                                                 : v - static_cast<F>(0.5));
  }

  Rep raw_;
};

// Common alias that mirrors 15.16 format from many engines
using Fixed15p16 = Fixed<int32_t, 16>;

}  // namespace navary::math
