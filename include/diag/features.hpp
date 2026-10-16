#pragma once

// Compile-time feature switches for the diagnostics C++ API.
//
// These values are supplied by CMake for normal builds and default to enabled
// for direct header use. Build the library and all including translation units
// with the same values.

#ifndef DIAG_FEATURE_DTC
#define DIAG_FEATURE_DTC (1)
#endif

#ifndef DIAG_FEATURE_LIFECYCLE
#define DIAG_FEATURE_LIFECYCLE (1)
#endif

#ifndef DIAG_FEATURE_IDENTITY
#define DIAG_FEATURE_IDENTITY (1)
#endif

#ifndef DIAG_FEATURE_STORAGE
#define DIAG_FEATURE_STORAGE (1)
#endif

#ifndef DIAG_FEATURE_CAPSULE
#define DIAG_FEATURE_CAPSULE (1)
#endif

namespace diag::features
{

static constexpr bool kDtc = (DIAG_FEATURE_DTC != 0);
static constexpr bool kLifecycle = (DIAG_FEATURE_LIFECYCLE != 0);
static constexpr bool kIdentity = (DIAG_FEATURE_IDENTITY != 0);
static constexpr bool kStorage = (DIAG_FEATURE_STORAGE != 0);
static constexpr bool kCapsule = (DIAG_FEATURE_CAPSULE != 0);
static constexpr bool kPersistence = kStorage && kCapsule;

} // namespace diag::features
