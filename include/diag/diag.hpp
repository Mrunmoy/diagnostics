#pragma once

#include "diag/features.hpp"

#if DIAG_FEATURE_CAPSULE
#include "diag/capsule.hpp"
#endif

#include "diag/context.hpp"

#if DIAG_FEATURE_DTC
#include "diag/dtc.hpp"
#endif

#if DIAG_FEATURE_IDENTITY
#include "diag/identity.hpp"
#endif

#if DIAG_FEATURE_LIFECYCLE
#include "diag/lifecycle.hpp"
#endif

#include "diag/result.hpp"

#if DIAG_FEATURE_STORAGE
#include "diag/storage.hpp"
#endif

#include "diag/types.hpp"
