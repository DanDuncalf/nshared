/*
 * common.c -- Shared utility functions
 *
 * Memory allocation wrappers that exit on failure.
 * These provide consistent error handling across all modules.
 * Designed to be shared across projects.
 */

#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef COMMON_PROJECT_NAME
#define COMMON_PROJECT_NAME "APP"
#endif

#define XSTR(s) STR(s)
#define STR(s) #s

/* ================================================================ memory allocation  */

void *ncd_malloc(size_t size)
{
    if (size == 0) size = 1;  /* Avoid implementation-defined behavior */
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, XSTR(COMMON_PROJECT_NAME) ": fatal: out of memory (allocating %zu bytes)\n", size);
        exit(1);
    }
    return p;
}

void *ncd_realloc(void *ptr, size_t size)
{
    if (size == 0) size = 1;
    void *p = realloc(ptr, size);
    if (!p) {
        fprintf(stderr, XSTR(COMMON_PROJECT_NAME) ": fatal: out of memory (reallocating %zu bytes)\n", size);
        exit(1);
    }
    return p;
}

char *ncd_strdup(const char *s)
{
    if (!s) s = "";
    size_t n = strlen(s) + 1;
    char *p = ncd_malloc(n);
    memcpy(p, s, n);
    return p;
}

void *ncd_calloc(size_t count, size_t size)
{
    if (count == 0) count = 1;
    if (size == 0) size = 1;
    void *p = calloc(count, size);
    if (!p) {
        fprintf(stderr, XSTR(COMMON_PROJECT_NAME) ": fatal: out of memory (calloc %zu * %zu)\n", count, size);
        exit(1);
    }
    return p;
}

void *ncd_malloc_array(size_t count, size_t size)
{
    if (count == 0 || size == 0) return ncd_malloc(1);
    /* Check for overflow */
    if (count > SIZE_MAX / size) {
        fprintf(stderr, XSTR(COMMON_PROJECT_NAME) ": fatal: array size overflow (%zu * %zu)\n", count, size);
        exit(1);
    }
    return ncd_malloc(count * size);
}

/* Overflow-checked multiplication. Exits on overflow. */
size_t ncd_mul_overflow_check(size_t a, size_t b)
{
    if (a == 0 || b == 0) return 0;
    if (a > SIZE_MAX / b) {
        fprintf(stderr, XSTR(COMMON_PROJECT_NAME) ": fatal: multiplication overflow (%zu * %zu)\n", a, b);
        exit(1);
    }
    return a * b;
}

/* Overflow-checked addition. Exits on overflow. */
size_t ncd_add_overflow_check(size_t a, size_t b)
{
    if (b > SIZE_MAX - a) {
        fprintf(stderr, XSTR(COMMON_PROJECT_NAME) ": fatal: addition overflow (%zu + %zu)\n", a, b);
        exit(1);
    }
    return a + b;
}
