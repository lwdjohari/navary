#pragma once

#define NVR_NAMESPACE namespace navary
#define NVR_BEGIN_NAMESPACE NVR_NAMESPACE {
#define NVR_MAKE_NAMESPACE(arg) namespace arg {
#define NVR_END_NAMESPACE }
#define NVR_INNER_NAMESPACE(arg) \
  NVR_BEGIN_NAMESPACE            \
  namespace arg {
#define NVR_INNER_END_NAMESPACE \
  }                             \
  }

// Architecture
#define NVR_ARCH_32BIT 0
#define NVR_ARCH_64BIT 0

// Compiler
#define NVR_COMPILER_CLANG 0
#define NVR_COMPILER_CLANG_ANALYZER 0
#define NVR_COMPILER_GCC 0
#define NVR_COMPILER_MSVC 0

// Endianness
#define NVR_ENDIAN_BIG 0
#define NVR_ENDIAN_LITTLE 0

// CPU
#define NVR_CPU_ARM 0
#define NVR_CPU_JIT 0
#define NVR_CPU_MIPS 0
#define NVR_CPU_PPC 0
#define NVR_CPU_RISCV 0
#define NVR_CPU_X86 0

// C Runtime
#define NVR_CRT_BIONIC 0
#define NVR_CRT_BSD 0
#define NVR_CRT_GLIBC 0
#define NVR_CRT_LIBCXX 0
#define NVR_CRT_MINGW 0
#define NVR_CRT_MSVC 0
#define NVR_CRT_NEWLIB 0

#ifndef NVR_CRT_NONE
#define NVR_CRT_NONE 0
#endif  // NVR_CRT_NONE

// Language standard version
#define NVR_CPP17 201703L
#define NVR_CPP20 202002L
#define NVR_CPP23 202207L

// Platform
#define NVR_PLATFORM_ANDROID 0
#define NVR_PLATFORM_BSD 0
#define NVR_PLATFORM_WEBGL 0
#define NVR_PLATFORM_HAIKU 0
#define NVR_PLATFORM_HURD 0
#define NVR_PLATFORM_IOS 0
#define NVR_PLATFORM_LINUX 0
#define NVR_PLATFORM_NX 0
#define NVR_PLATFORM_OSX 0
#define NVR_PLATFORM_PS4 0
#define NVR_PLATFORM_PS5 0
#define NVR_PLATFORM_RPI 0
#define NVR_PLATFORM_VISIONOS 0
#define NVR_PLATFORM_WINDOWS 0
#define NVR_PLATFORM_WINRT 0
#define NVR_PLATFORM_XBOXONE 0

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Compilers
#if defined(__clang__)
// clang defines __GNUC__ or _MSC_VER
#undef NVR_COMPILER_CLANG
#define NVR_COMPILER_CLANG (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#if defined(__clang_analyzer__)
#undef NVR_COMPILER_CLANG_ANALYZER
#define NVR_COMPILER_CLANG_ANALYZER 1
#endif  // defined(__clang_analyzer__)
#elif defined(_MSC_VER)
#undef NVR_COMPILER_MSVC
#define NVR_COMPILER_MSVC _MSC_VER
#elif defined(__GNUC__)
#undef NVR_COMPILER_GCC
#define NVR_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#error "NVR_COMPILER_* is not defined!"
#endif  //

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Architectures
#if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
#undef NVR_CPU_ARM
#define NVR_CPU_ARM 1
#define NVR_CACHE_LINE_SIZE 64
#elif defined(__MIPSEL__) || defined(__mips_isa_rev) || defined(__mips64)
#undef NVR_CPU_MIPS
#define NVR_CPU_MIPS 1
#define NVR_CACHE_LINE_SIZE 64
#elif defined(_M_PPC) || defined(__powerpc__) || defined(__powerpc64__)
#undef NVR_CPU_PPC
#define NVR_CPU_PPC 1
#define NVR_CACHE_LINE_SIZE 128
#elif defined(__riscv) || defined(__riscv__) || defined(RISCVEL)
#undef NVR_CPU_RISCV
#define NVR_CPU_RISCV 1
#define NVR_CACHE_LINE_SIZE 64
#elif defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#undef NVR_CPU_X86
#define NVR_CPU_X86 1
#define NVR_CACHE_LINE_SIZE 64
#else  // Doesn't have CPU defined.
#undef NVR_CPU_JIT
#define NVR_CPU_JIT 1
#define NVR_CACHE_LINE_SIZE 64
#endif  //

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(__64BIT__) || \
    defined(__mips64) || defined(__powerpc64__) || defined(__ppc64__) || defined(__LP64__)
#undef NVR_ARCH_64BIT
#define NVR_ARCH_64BIT 64
#else
#undef NVR_ARCH_32BIT
#define NVR_ARCH_32BIT 32
#endif  //

#if NVR_CPU_PPC
// __BIG_ENDIAN__ is gcc predefined macro
#if defined(__BIG_ENDIAN__)
#undef NVR_ENDIAN_BIG
#define NVR_ENDIAN_BIG 1
#else
#undef NVR_ENDIAN_LITTLE
#define NVR_ENDIAN_LITTLE 1
#endif
#else
#undef NVR_ENDIAN_LITTLE
#define NVR_ENDIAN_LITTLE 1
#endif  // NVR_CPU_PPC

// http://sourceforge.net/apps/mediawiki/predef/index.php?title=Operating_Systems
#if defined(_DURANGO) || defined(_XBOX_ONE)
#undef NVR_PLATFORM_XBOXONE
#define NVR_PLATFORM_XBOXONE 1
#elif defined(_WIN32) || defined(_WIN64)
// http://msdn.microsoft.com/en-us/library/6sehtctf.aspx
#ifndef NOMINMAX
#define NOMINMAX
#endif  // NOMINMAX
//  If _USING_V110_SDK71_ is defined it means we are using the v110_xp or
//  v120_xp toolset.
#if defined(_MSC_VER) && (_MSC_VER >= 1700) && !defined(_USING_V110_SDK71_)
#include <winapifamily.h>
#endif  // defined(_MSC_VER) && (_MSC_VER >= 1700) && (!_USING_V110_SDK71_)
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP)
#undef NVR_PLATFORM_WINDOWS
#if !defined(WINVER) && !defined(_WIN32_WINNT)
#if NVR_ARCH_64BIT
//				When building 64-bit target Win7 and above.
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#else
//				Windows Server 2003 with SP1, Windows XP with
// SP2 and above
#define WINVER 0x0502
#define _WIN32_WINNT 0x0502
#endif  // NVR_ARCH_64BIT
#endif  // !defined(WINVER) && !defined(_WIN32_WINNT)
#define NVR_PLATFORM_WINDOWS _WIN32_WINNT
#else
#undef NVR_PLATFORM_WINRT
#define NVR_PLATFORM_WINRT 1
#endif
#elif defined(__ANDROID__)
// Android compiler defines __linux__
#include <sys/cdefs.h>  // Defines __BIONIC__ and includes android/api-level.h
#undef NVR_PLATFORM_ANDROID
#define NVR_PLATFORM_ANDROID __ANDROID_API__
#elif defined(__VCCOREVER__)
// RaspberryPi compiler defines __linux__
#undef NVR_PLATFORM_RPI
#define NVR_PLATFORM_RPI 1
#elif defined(__linux__)
#undef NVR_PLATFORM_LINUX
#define NVR_PLATFORM_LINUX 1
#elif defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || \
    defined(__ENVIRONMENT_TV_OS_VERSION_MIN_REQUIRED__)
#undef NVR_PLATFORM_IOS
#define NVR_PLATFORM_IOS 1
#elif defined(__has_builtin) && __has_builtin(__is_target_os) && __is_target_os(xros)
#undef NVR_PLATFORM_VISIONOS
#define NVR_PLATFORM_VISIONOS 1
#elif defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__)
#undef NVR_PLATFORM_OSX
#define NVR_PLATFORM_OSX __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__
#elif defined(__EMSCRIPTEN__)
#include <emscripten/version.h>
#undef NVR_PLATFORM_WEBGL
#define NVR_PLATFORM_WEBGL \
  (__EMSCRIPTEN_major__ * 10000 + __EMSCRIPTEN_minor__ * 100 + __EMSCRIPTEN_tiny__)
#elif defined(__ORBIS__)
#undef NVR_PLATFORM_PS4
#define NVR_PLATFORM_PS4 1
#elif defined(__PROSPERO__)
#undef NVR_PLATFORM_PS5
#define NVR_PLATFORM_PS5 1
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__) || \
    defined(__OpenBSD__) || defined(__DragonFly__)
#undef NVR_PLATFORM_BSD
#define NVR_PLATFORM_BSD 1
#elif defined(__GNU__)
#undef NVR_PLATFORM_HURD
#define NVR_PLATFORM_HURD 1
#elif defined(__NX__)
#undef NVR_PLATFORM_NX
#define NVR_PLATFORM_NX 1
#elif defined(__HAIKU__)
#undef NVR_PLATFORM_HAIKU
#define NVR_PLATFORM_HAIKU 1
#endif  //

#if !NVR_CRT_NONE
// https://sourceforge.net/p/predef/wiki/Libraries/
#if defined(__BIONIC__)
#undef NVR_CRT_BIONIC
#define NVR_CRT_BIONIC 1
#elif defined(_MSC_VER)
#undef NVR_CRT_MSVC
#define NVR_CRT_MSVC 1
#elif defined(__GLIBC__)
#undef NVR_CRT_GLIBC
#define NVR_CRT_GLIBC (__GLIBC__ * 10000 + __GLIBC_MINOR__ * 100)
#elif defined(__MINGW32__) || defined(__MINGW64__)
#undef NVR_CRT_MINGW
#define NVR_CRT_MINGW 1
#elif defined(__apple_build_version__) || defined(__ORBIS__) || defined(__EMSCRIPTEN__) || \
    defined(__llvm__) || defined(__HAIKU__)
#undef NVR_CRT_LIBCXX
#define NVR_CRT_LIBCXX 1
#elif NVR_PLATFORM_BSD
#undef NVR_CRT_BSD
#define NVR_CRT_BSD 1
#endif  //

#if !NVR_CRT_BIONIC && !NVR_CRT_BSD && !NVR_CRT_GLIBC && !NVR_CRT_LIBCXX && !NVR_CRT_MINGW && \
    !NVR_CRT_MSVC && !NVR_CRT_NEWLIB
#undef NVR_CRT_NONE
#define NVR_CRT_NONE 1
#endif  // NVR_CRT_*
#endif  // !NVR_CRT_NONE

///
#define NVR_PLATFORM_POSIX                                                                      \
  (0 || NVR_PLATFORM_ANDROID || NVR_PLATFORM_BSD || NVR_PLATFORM_WEBGL || NVR_PLATFORM_HAIKU || \
   NVR_PLATFORM_HURD || NVR_PLATFORM_IOS || NVR_PLATFORM_LINUX || NVR_PLATFORM_NX ||            \
   NVR_PLATFORM_OSX || NVR_PLATFORM_PS4 || NVR_PLATFORM_PS5 || NVR_PLATFORM_RPI ||              \
   NVR_PLATFORM_VISIONOS)

///
#define NVR_PLATFORM_NONE                                                                        \
  !(0 || NVR_PLATFORM_ANDROID || NVR_PLATFORM_BSD || NVR_PLATFORM_WEBGL || NVR_PLATFORM_HAIKU || \
    NVR_PLATFORM_HURD || NVR_PLATFORM_IOS || NVR_PLATFORM_LINUX || NVR_PLATFORM_NX ||            \
    NVR_PLATFORM_OSX || NVR_PLATFORM_PS4 || NVR_PLATFORM_PS5 || NVR_PLATFORM_RPI ||              \
    NVR_PLATFORM_VISIONOS || NVR_PLATFORM_WINDOWS || NVR_PLATFORM_WINRT || NVR_PLATFORM_XBOXONE)

///
#define NVR_PLATFORM_OS_CONSOLE                                                          \
  (0 || NVR_PLATFORM_NX || NVR_PLATFORM_PS4 || NVR_PLATFORM_PS5 || NVR_PLATFORM_WINRT || \
   NVR_PLATFORM_XBOXONE)

///
#define NVR_PLATFORM_OS_DESKTOP                                                              \
  (0 || NVR_PLATFORM_BSD || NVR_PLATFORM_HAIKU || NVR_PLATFORM_HURD || NVR_PLATFORM_LINUX || \
   NVR_PLATFORM_OSX || NVR_PLATFORM_WINDOWS)

///
#define NVR_PLATFORM_OS_EMBEDDED (0 || NVR_PLATFORM_RPI)

///
#define NVR_PLATFORM_OS_MOBILE (0 || NVR_PLATFORM_ANDROID || NVR_PLATFORM_IOS)

///
#define NVR_PLATFORM_OS_WEB (0 || NVR_PLATFORM_WEBGL)

///
#if NVR_COMPILER_GCC
#define NVR_COMPILER_NAME                                                             \
  "GCC " NVR_STRINGIZE(__GNUC__) "." NVR_STRINGIZE(__GNUC_MINOR__) "." NVR_STRINGIZE( \
      __GNUC_PATCHLEVEL__)
#elif NVR_COMPILER_CLANG
#define NVR_COMPILER_NAME                                                                       \
  "Clang " NVR_STRINGIZE(__clang_major__) "." NVR_STRINGIZE(__clang_minor__) "." NVR_STRINGIZE( \
      __clang_patchlevel__)
#elif NVR_COMPILER_MSVC
#if NVR_COMPILER_MSVC >= 1930  // Visual Studio 2022
#define NVR_COMPILER_NAME "MSVC 17.0"
#elif NVR_COMPILER_MSVC >= 1920  // Visual Studio 2019
#define NVR_COMPILER_NAME "MSVC 16.0"
#elif NVR_COMPILER_MSVC >= 1910  // Visual Studio 2017
#define NVR_COMPILER_NAME "MSVC 15.0"
#elif NVR_COMPILER_MSVC >= 1900  // Visual Studio 2015
#define NVR_COMPILER_NAME "MSVC 14.0"
#elif NVR_COMPILER_MSVC >= 1800  // Visual Studio 2013
#define NVR_COMPILER_NAME "MSVC 12.0"
#elif NVR_COMPILER_MSVC >= 1700  // Visual Studio 2012
#define NVR_COMPILER_NAME "MSVC 11.0"
#elif NVR_COMPILER_MSVC >= 1600  // Visual Studio 2010
#define NVR_COMPILER_NAME "MSVC 10.0"
#elif NVR_COMPILER_MSVC >= 1500  // Visual Studio 2008
#define NVR_COMPILER_NAME "MSVC 9.0"
#else
#define NVR_COMPILER_NAME "MSVC"
#endif  //
#endif  // NVR_COMPILER_

#if NVR_PLATFORM_ANDROID
#define NVR_PLATFORM_NAME "Android " NVR_STRINGIZE(NVR_PLATFORM_ANDROID)
#elif NVR_PLATFORM_BSD
#define NVR_PLATFORM_NAME "BSD"
#elif NVR_PLATFORM_WEBGL
#define NVR_PLATFORM_NAME                                              \
  "Emscripten " NVR_STRINGIZE(__EMSCRIPTEN_major__) "." NVR_STRINGIZE( \
      __EMSCRIPTEN_minor__) "." NVR_STRINGIZE(__EMSCRIPTEN_tiny__)
#elif NVR_PLATFORM_HAIKU
#define NVR_PLATFORM_NAME "Haiku"
#elif NVR_PLATFORM_HURD
#define NVR_PLATFORM_NAME "Hurd"
#elif NVR_PLATFORM_IOS
#define NVR_PLATFORM_NAME "iOS"
#elif NVR_PLATFORM_LINUX
#define NVR_PLATFORM_NAME "Linux"
#elif NVR_PLATFORM_NONE
#define NVR_PLATFORM_NAME "None"
#elif NVR_PLATFORM_NX
#define NVR_PLATFORM_NAME "NX"
#elif NVR_PLATFORM_OSX
#define NVR_PLATFORM_NAME "macOS"
#elif NVR_PLATFORM_PS4
#define NVR_PLATFORM_NAME "PlayStation 4"
#elif NVR_PLATFORM_PS5
#define NVR_PLATFORM_NAME "PlayStation 5"
#elif NVR_PLATFORM_RPI
#define NVR_PLATFORM_NAME "RaspberryPi"
#elif NVR_PLATFORM_VISIONOS
#define NVR_PLATFORM_NAME "visionOS"
#elif NVR_PLATFORM_WINDOWS
#define NVR_PLATFORM_NAME "Windows"
#elif NVR_PLATFORM_WINRT
#define NVR_PLATFORM_NAME "WinRT"
#elif NVR_PLATFORM_XBOXONE
#define NVR_PLATFORM_NAME "Xbox One"
#else
#error "Unknown NVR_PLATFORM!"
#endif  // NVR_PLATFORM_

#if NVR_CPU_ARM
#define NVR_CPU_NAME "ARM"
#elif NVR_CPU_JIT
#define NVR_CPU_NAME "JIT-VM"
#elif NVR_CPU_MIPS
#define NVR_CPU_NAME "MIPS"
#elif NVR_CPU_PPC
#define NVR_CPU_NAME "PowerPC"
#elif NVR_CPU_RISCV
#define NVR_CPU_NAME "RISC-V"
#elif NVR_CPU_X86
#define NVR_CPU_NAME "x86"
#endif  // NVR_CPU_

#if NVR_CRT_BIONIC
#define NVR_CRT_NAME "Bionic libc"
#elif NVR_CRT_BSD
#define NVR_CRT_NAME "BSD libc"
#elif NVR_CRT_GLIBC
#define NVR_CRT_NAME "GNU C Library"
#elif NVR_CRT_MSVC
#define NVR_CRT_NAME "MSVC C Runtime"
#elif NVR_CRT_MINGW
#define NVR_CRT_NAME "MinGW C Runtime"
#elif NVR_CRT_LIBCXX
#define NVR_CRT_NAME "Clang C Library"
#elif NVR_CRT_NEWLIB
#define NVR_CRT_NAME "Newlib"
#elif NVR_CRT_NONE
#define NVR_CRT_NAME "None"
#else
#error "Unknown NVR_CRT!"
#endif  // NVR_CRT_

#if NVR_ARCH_32BIT
#define NVR_ARCH_NAME "32-bit"
#elif NVR_ARCH_64BIT
#define NVR_ARCH_NAME "64-bit"
#endif  // NVR_ARCH_

#if defined(__cplusplus)
#if NVR_COMPILER_MSVC && defined(_MSVC_LANG) && _MSVC_LANG != __cplusplus
#error "When using MSVC you must set /Zc:__cplusplus compiler option."
#endif  // NVR_COMPILER_MSVC && defined(_MSVC_LANG) && _MSVC_LANG !=
        // __cplusplus

#if __cplusplus < NVR_CPP17
#error "C++17 standard support is required to build."
#elif __cplusplus < NVR_CPP20
#define NVR_CPP_NAME "C++17"
#elif __cplusplus < NVR_CPP23
#define NVR_CPP_NAME "C++20"
#else
// See: https://gist.github.com/bkaradzic/2e39896bc7d8c34e042b#orthodox-c
#define NVR_CPP_NAME "C++WayTooModern"
#endif  // NVR_CPP_NAME
#else
#define NVR_CPP_NAME "C++Unknown"
#endif  // defined(__cplusplus)

#if NVR_COMPILER_MSVC && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL)
#error "When using MSVC you must set /Zc:preprocessor compiler option."
#endif  // NVR_COMPILER_MSVC && (!defined(_MSVC_TRADITIONAL) ||
        // _MSVC_TRADITIONAL)

#if NVR_PLATFORM_OSX && NVR_PLATFORM_OSX < 130000
// #error "Minimum supported macOS version is 13.00.\n"
#elif NVR_PLATFORM_IOS && NVR_PLATFORM_IOS < 160000
// #error "Minimum supported macOS version is 16.00.\n"
#endif  // NVR_PLATFORM_OSX < 130000

#if NVR_ENDIAN_BIG
static_assert(false,
              "\n\n"
              "\t** IMPORTANT! **\n\n"
              "\tThe code was not tested for big endian, and big endian "
              "CPU is considered unsupported.\n"
              "\t\n");
#endif  // NVR_ENDIAN_BIG

#if NVR_PLATFORM_BSD || NVR_PLATFORM_HAIKU || NVR_PLATFORM_HURD
static_assert(false,
              "\n\n"
              "\t** IMPORTANT! **\n\n"
              "\tYou're compiling for unsupported platform!\n"
              "\tIf you wish to support this platform, make your own "
              "fork, and modify code for _yourself_.\n"
              "\t\n"
              "\tDo not submit PR to main repo, it won't be considered, "
              "and it would code rot anyway. I have no ability\n"
              "\tto test on these platforms, and over years there "
              "wasn't any serious contributor who wanted to take\n"
              "\tburden of maintaining code for these platforms.\n"
              "\t\n");
#endif  // NVR_PLATFORM_*

#if NVR_COMPILER_CLANG || NVR_COMPILER_GCC
#define NVR_FN_EXPERIMENTAL [[gnu::warning("Experimental feature, subject to change")]]
#elif NVR_COMPILER_MSVC && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL)
#define NVR_FN_EXPERIMENTAL __declspec(deprecated("Experimental feature, subject to change"))
#else
#define NVR_FN_EXPERIMENTAL
#endif

#if NVR_COMPILER_CLANG || NVR_COMPILER_GCC
#define NVR_FN_WIP                                                  \
  [[gnu::warning(                                                   \
      "Work-in-progress feature, subject to change or not covered " \
      "all features")]]
#elif NVR_COMPILER_MSVC && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL)
#define NVR_FN_WIP                                          \
  __declspec(deprecated(                                    \
      "Work-in-progress feature, subject to change or not " \
      "covered all features"))
#else
#define NVR_FN_WIP
#endif

#include <iostream>
#include <type_traits>

#define NVR_ENUM_CLASS_ENABLE_BITMASK_OPERATORS(T)                       \
  inline T operator|(T lhs, T rhs) {                                     \
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) |  \
                          static_cast<std::underlying_type_t<T>>(rhs));  \
  }                                                                      \
                                                                         \
  inline T operator&(T lhs, T rhs) {                                     \
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) &  \
                          static_cast<std::underlying_type_t<T>>(rhs));  \
  }                                                                      \
                                                                         \
  inline T operator^(T lhs, T rhs) {                                     \
    return static_cast<T>(static_cast<std::underlying_type_t<T>>(lhs) ^  \
                          static_cast<std::underlying_type_t<T>>(rhs));  \
  }                                                                      \
                                                                         \
  inline T operator~(T rhs) {                                            \
    return static_cast<T>(~static_cast<std::underlying_type_t<T>>(rhs)); \
  }

#define NVR_ENUM_CLASS_DISPLAY_TRAIT(E)                                  \
  inline std::ostream& operator<<(std::ostream& os, E e) {               \
    return os << static_cast<typename std::underlying_type<E>::type>(e); \
  }

namespace navary {
namespace impl {
template <typename Enum, typename Lambda>
std::string enum_to_string_impl(Enum e, Lambda lambda) {
  return lambda(e);
}
}  // namespace impl
}  // namespace navary

#define NVR_ENUM_CLASS_TO_STRING_FORMATTER(EnumType, ...)            \
                                                                     \
  inline std::string ToStringEnum##EnumType(EnumType e) {            \
    static const auto toStringFunc = [](EnumType e) -> std::string { \
      switch (e) {                                                   \
        __VA_ARGS__                                                  \
        default:                                                     \
          throw std::invalid_argument("Unsupported enum value");     \
      }                                                              \
    };                                                               \
    return navary::impl::enum_to_string_impl(e, toStringFunc);       \
  }