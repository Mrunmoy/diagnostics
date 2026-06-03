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

#include "diag/capsule.h"
#include "diag/compiler.h"
#include "diag/context.h"
#include "diag/dtc.h"
#include "diag/features.h"
#include "diag/identity.h"
#include "diag/lifecycle.h"
#include "diag/result.h"
#include "diag/storage.h"
#include "diag/transport.h"

#endif
