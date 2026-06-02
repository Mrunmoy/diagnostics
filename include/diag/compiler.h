#ifndef DIAG_COMPILER_H
#define DIAG_COMPILER_H

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define DIAG_ALIGNAS_PREFIX(alignment) _Alignas(alignment)
#define DIAG_ALIGNAS_SUFFIX(alignment)
#elif defined(_MSC_VER)
#define DIAG_ALIGNAS_PREFIX(alignment) __declspec(align(alignment))
#define DIAG_ALIGNAS_SUFFIX(alignment)
#elif defined(__GNUC__) || defined(__clang__)
#define DIAG_ALIGNAS_PREFIX(alignment)
#define DIAG_ALIGNAS_SUFFIX(alignment) __attribute__((aligned(alignment)))
#else
#define DIAG_ALIGNAS_PREFIX(alignment)
#define DIAG_ALIGNAS_SUFFIX(alignment)
#endif

#if defined(__GNUC__) || defined(__clang__)
#define DIAG_LIKELY(expression) __builtin_expect(!!(expression), 1)
#define DIAG_UNLIKELY(expression) __builtin_expect(!!(expression), 0)
#else
#define DIAG_LIKELY(expression) (expression)
#define DIAG_UNLIKELY(expression) (expression)
#endif

#endif
