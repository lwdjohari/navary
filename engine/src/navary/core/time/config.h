#pragma once
// Navary Engine - Timing Subsystem
// File: navary/core/time/config.h
// Purpose: Compile-time configuration flags and lightweight assert helpers.
// Policy: C++20, Google style, no exceptions, no RTTI, cross-platform.
//
// Design:
//   - Allows enabling/disabling optional features (telemetry, soft resync, raw
//   clocks).
//   - Central place for compile-time constants and diagnostic macros.
//   - Avoids platform #ifdefs scattered across headers.
//   - Every flag has a default OFF/ON policy that can be overridden via CMake.
//
// Usage in CMakeLists.txt:
//   target_compile_definitions(navary_core_time PUBLIC
//   NAVARY_TIME_ENABLE_TELEMETRY=1)
//
//   // Example conditional in code:
//   #if NAVARY_TIME_ENABLE_TELEMETRY
//     telemetry.RecordDeadlineMiss(...);
//   #endif
//
// Author:
// Linggawasistha Djohari              [2025-Present]

#include <cassert>
#include <cstdint>

//------------------------------------------------------------------------------
// Feature Toggles (default OFF unless otherwise noted)
//------------------------------------------------------------------------------

// Enable lightweight telemetry ring buffer (adds <3 KB footprint).
#ifndef NAVARY_TIME_ENABLE_TELEMETRY
#define NAVARY_TIME_ENABLE_TELEMETRY 1
#endif

// Prefer CLOCK_MONOTONIC_RAW on Linux/Android if available.
#ifndef NAVARY_TIME_USE_MONOTONIC_RAW
#define NAVARY_TIME_USE_MONOTONIC_RAW 1
#endif

// Compile the soft resync drift corrector.
#ifndef NAVARY_TIME_ENABLE_SOFT_RESYNC
#define NAVARY_TIME_ENABLE_SOFT_RESYNC 1
#endif

// Allow steady_clock fallback when OS-specific clocks fail.
#ifndef NAVARY_TIME_ALLOW_FALLBACK
#define NAVARY_TIME_ALLOW_FALLBACK 1
#endif

//------------------------------------------------------------------------------
// Build Policies
//------------------------------------------------------------------------------

// Disable exceptions in this subsystem (use Status/return codes instead).
#ifndef NAVARY_TIME_DISABLE_EXCEPTIONS
#define NAVARY_TIME_DISABLE_EXCEPTIONS 1
#endif

// Disable RTTI.
#ifndef NAVARY_TIME_DISABLE_RTTI
#define NAVARY_TIME_DISABLE_RTTI 1
#endif

// Treat warnings as errors in this subsystem.
#ifndef NAVARY_STRICT_WARNINGS
#define NAVARY_STRICT_WARNINGS 1
#endif

//------------------------------------------------------------------------------
// Assert & Debug Macros (platform-independent, minimal overhead)
//------------------------------------------------------------------------------

// Compile-time debug flag; can be overridden externally.
#if !defined(NAVARY_DEBUG)
#if defined(_DEBUG) || !defined(NDEBUG)
#define NAVARY_DEBUG 1
#else
#define NAVARY_DEBUG 0
#endif
#endif

#if NAVARY_DEBUG
#define NVR_ASSERT(expr) assert(expr)
#else
#define NVR_ASSERT(expr) ((void)0)
#endif

//------------------------------------------------------------------------------
// Logging (optional stubs; real logging can be injected by host engine)
//------------------------------------------------------------------------------
#if NAVARY_DEBUG
#include <cstdio>
#define NVR_LOG(fmt, ...) \
  std::fprintf(stderr, "[NVR-TIME] " fmt "\n", ##__VA_ARGS__)
#else
#define NVR_LOG(fmt, ...) ((void)0)
#endif

//------------------------------------------------------------------------------
// Platform Detection (used by internal clocks)
//------------------------------------------------------------------------------
#if defined(_WIN32)
#define NVR_PLATFORM_WINDOWS 1
#elif defined(__APPLE__)
#define NVR_PLATFORM_APPLE 1
#elif defined(__ANDROID__)
#define NVR_PLATFORM_ANDROID 1
#elif defined(__linux__)
#define NVR_PLATFORM_LINUX 1
#else
#define NVR_PLATFORM_UNKNOWN 1
#endif

//------------------------------------------------------------------------------
// Default Constants
//------------------------------------------------------------------------------
inline constexpr std::uint64_t NVR_TIME_DEFAULT_STALL_CLAMP_NS =
    50'000'000ull;                                                // 50 ms
inline constexpr std::uint32_t NVR_TIME_DEFAULT_MAX_CATCHUP = 4;  // steps
inline constexpr double NVR_TIME_DEFAULT_HZ = 60.0;               // target Hz
