/*
 * common.h -- Shared utility functions
 *
 * Memory allocation wrappers that exit on failure.
 * These provide consistent error handling across all modules.
 * Designed to be shared across projects.
 */

#ifndef SHARED_COMMON_H
#define SHARED_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Project identifier for error messages (can be overridden) */
#ifndef COMMON_PROJECT_NAME
#define COMMON_PROJECT_NAME "APP"
#endif

/* ================================================================ memory allocation  */

void *ncd_malloc(size_t size);
void *ncd_realloc(void *ptr, size_t size);
char *ncd_strdup(const char *s);
void *ncd_calloc(size_t count, size_t size);
void *ncd_malloc_array(size_t count, size_t size);

/* Overflow-checked arithmetic. Exits on overflow. */
size_t ncd_mul_overflow_check(size_t a, size_t b);
size_t ncd_add_overflow_check(size_t a, size_t b);

#ifdef __cplusplus
}
#endif

#endif /* SHARED_COMMON_H */
