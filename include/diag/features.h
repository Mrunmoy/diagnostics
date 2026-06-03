/// @file
/// Compile-time feature switches for the diagnostics C API.

#ifndef DIAG_FEATURES_H
#define DIAG_FEATURES_H

/// Enable Diagnostic Trouble Code registration and runtime state.
#ifndef DIAG_FEATURE_DTC
#define DIAG_FEATURE_DTC (1)
#endif

/// Enable lifecycle and reset counter tracking.
#ifndef DIAG_FEATURE_LIFECYCLE
#define DIAG_FEATURE_LIFECYCLE (1)
#endif

/// Enable compact numeric device identity helpers.
#ifndef DIAG_FEATURE_IDENTITY
#define DIAG_FEATURE_IDENTITY (1)
#endif

/// Enable storage adapter validation and wrapper APIs.
#ifndef DIAG_FEATURE_STORAGE
#define DIAG_FEATURE_STORAGE (1)
#endif

/// Enable transport adapter declarations.
#ifndef DIAG_FEATURE_TRANSPORT
#define DIAG_FEATURE_TRANSPORT (1)
#endif

/// Enable versioned serialized capsule helpers.
#ifndef DIAG_FEATURE_CAPSULE
#define DIAG_FEATURE_CAPSULE (1)
#endif

#endif
