// Span (dynamic extent) for contiguous ranges.
//
// Features:
// - Non-owning view over contiguous memory
// - Works with T[], std::array<T,N>, and containers exposing data()/size()
// - begin/end iteration; size/empty; data; operator[]
// - subspan/first/last helpers
// - const-correctness: Span<const T> from Span<T> is allowed
//
// Notes:
// - Like std::span, operator[] is unchecked. Use at() if you want a debug
// check.
// - This is a dynamic-extent span; no static-extent template parameter.

#pragma once

#include <cstddef>      // size_t, ptrdiff_t
#include <type_traits>  // std::remove_cv_t, std::enable_if_t, std::is_convertible_v
#include <iterator>  // std::contiguous_iterator (C++20)
#include <array>     // std::array
#include <cassert>   // assert

namespace navary::memory {

// Detection helpers (SFINAE) for data()/size() containers.
namespace span_internal {
template <typename C>
using has_data_t = decltype(std::data(std::declval<C&>()));
template <typename C>
using has_size_t = decltype(std::size(std::declval<C&>()));

template <typename, typename = void>
struct is_contiguous_range : std::false_type {};

template <typename C>
struct is_contiguous_range<C, std::void_t<has_data_t<C>, has_size_t<C>>>
    : std::true_type {};

template <typename C>
inline constexpr bool is_contiguous_range_v = is_contiguous_range<C>::value;

template <typename From, typename To>
inline constexpr bool is_cv_compatible_v =
    std::is_convertible_v<From (*)[], To (*)[]>;

}  // namespace span_internal

template <typename T>
class Span {
 public:
  using element_type    = T;
  using value_type      = std::remove_cv_t<T>;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer         = T*;
  using reference       = T&;

  using iterator       = T*;
  using const_iterator = const T*;

  // -- ctors ---------------------------------------------------------------

  // Empty span.
  constexpr Span() noexcept : ptr_(nullptr), len_(0) {}

  // From pointer + size.
  constexpr Span(pointer p, size_type n) noexcept : ptr_(p), len_(n) {}

  // From pointer range [first, last). last must be >= first.
  constexpr Span(pointer first, pointer last) noexcept
      : ptr_(first), len_(static_cast<size_type>(last - first)) {}

  // From C array.
  template <typename U, std::size_t N,
            std::enable_if_t<span_internal::is_cv_compatible_v<U, T>, int> = 0>
  constexpr Span(U (&arr)[N]) noexcept : ptr_(arr), len_(N) {}

  // From std::array (const and non-const).
  template <typename U, std::size_t N,
            std::enable_if_t<span_internal::is_cv_compatible_v<U, T>, int> = 0>
  constexpr Span(std::array<U, N>& a) noexcept : ptr_(a.data()), len_(N) {}

  template <
      typename U, std::size_t N,
      std::enable_if_t<span_internal::is_cv_compatible_v<const U, T>, int> = 0>
  constexpr Span(const std::array<U, N>& a) noexcept
      : ptr_(a.data()), len_(N) {}

  // From any contiguous range with data()/size().
  template <typename Range,
            std::enable_if_t<span_internal::is_contiguous_range_v<Range> &&
                                 span_internal::is_cv_compatible_v<
                                     std::remove_pointer_t<decltype(std::data(
                                         std::declval<Range&>()))>,
                                     T>,
                             int> = 0>
  constexpr Span(Range& r) noexcept
      : ptr_(std::data(r)), len_(static_cast<size_type>(std::size(r))) {}

  // Converting copy ctor: Span<U> -> Span<T> if cv-compatible.
  template <typename U,
            std::enable_if_t<span_internal::is_cv_compatible_v<U, T>, int> = 0>
  constexpr Span(const Span<U>& other) noexcept
      : ptr_(other.data()), len_(other.size()) {}

  // -- observers -----------------------------------------------------------

  constexpr size_type size() const noexcept {
    return len_;
  }
  constexpr bool empty() const noexcept {
    return len_ == 0;
  }
  constexpr pointer data() const noexcept {
    return ptr_;
  }

  // Unchecked element access (like std::span).
  constexpr reference operator[](size_type i) const noexcept {
    return *(ptr_ + i);
  }

  // Checked access (debug).
  reference at(size_type i) const {
    assert(i < len_ && "Span::at out of range");
    return (*this)[i];
  }

  // Iterators
  constexpr iterator begin() const noexcept {
    return ptr_;
  }
  constexpr iterator end() const noexcept {
    return ptr_ + len_;
  }
  constexpr const_iterator cbegin() const noexcept {
    return ptr_;
  }
  constexpr const_iterator cend() const noexcept {
    return ptr_ + len_;
  }

  // -- slicing -------------------------------------------------------------

  // subspan starting at offset (to end)
  constexpr Span<T> subspan(size_type offset) const noexcept {
    return Span<T>(ptr_ + offset, len_ - offset);
  }

  // subspan with offset + count (clamped)
  constexpr Span<T> subspan(size_type offset, size_type count) const noexcept {
    const size_type n = (offset + count <= len_) ? count : (len_ - offset);
    return Span<T>(ptr_ + offset, n);
  }

  // first N elements
  constexpr Span<T> first(size_type count) const noexcept {
    return Span<T>(ptr_, count <= len_ ? count : len_);
  }

  // last N elements
  constexpr Span<T> last(size_type count) const noexcept {
    const size_type n = (count <= len_) ? count : len_;
    return Span<T>(ptr_ + (len_ - n), n);
  }

 private:
  pointer ptr_;
  size_type len_;
};

// Deduction guides
template <class T, std::size_t N>
Span(T (&)[N]) -> Span<T>;

template <class T, std::size_t N>
Span(std::array<T, N>&) -> Span<T>;

template <class T, std::size_t N>
Span(const std::array<T, N>&) -> Span<const T>;

// Helper: make_span(range) to avoid spelling template args.
template <typename Range>
constexpr auto make_span(Range& r) noexcept {
  using T = std::remove_pointer_t<decltype(std::data(r))>;
  return Span<T>(r);
}

template <typename Range>
constexpr auto make_cspan(const Range& r) noexcept {
  using T = std::remove_pointer_t<decltype(std::data(r))>;
  return Span<const T>(r);
}

}  // namespace navary::memory
