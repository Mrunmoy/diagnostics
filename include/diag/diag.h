/// @file
/// Convenience umbrella include for the diagnostics C API.
///
/// @mainpage Diagnostics C API
///
/// The diagnostics library is an embedded-first C99 API for fixed-capacity
/// diagnostic state. The core owns DTC and lifecycle behavior while callers own
/// all memory, storage adapters, transport adapters, and product-specific
/// catalogs or strings.
///
/// Include `<diag/diag.h>` to pull in the complete public C API, or include the
/// narrower headers under `include/diag/` when a module only needs one surface.
/// Runtime state stays in caller-provided RAM; persistent storage is accessed
/// only through explicit APIs and caller-supplied adapters.

#ifndef DIAG_DIAG_H
#define DIAG_DIAG_H

#include "diag/compiler.h"
#include "diag/features.h"
#include "diag/context.h"
#include "diag/result.h"

#if DIAG_FEATURE_CAPSULE
#include "diag/capsule.h"
#endif

#if DIAG_FEATURE_DTC
#include "diag/dtc.h"
#endif

#if DIAG_FEATURE_IDENTITY
#include "diag/identity.h"
#endif

#if DIAG_FEATURE_LIFECYCLE
#include "diag/lifecycle.h"
#endif

#if DIAG_FEATURE_STORAGE
#include "diag/storage.h"
#endif

#if DIAG_FEATURE_TRANSPORT
#include "diag/transport.h"
#endif

#endif
