/*
 * platform_detect.h -- Platform and architecture detection macros
 *
 * This header is designed to be shared across projects. It provides
 * consistent platform detection without any project-specific dependencies.
 */

#ifndef PLATFORM_DETECT_H
#define PLATFORM_DETECT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform / architecture detection
 *
 * Common compiler-provided macros:
 *   Windows: _WIN32 / _WIN64 (MSVC/MinGW/Clang-cl)
 *   Linux:   __linux__
 *   x64:     _WIN64 (MSVC) or __x86_64__ / __amd64__ (GCC/Clang)
 */
#if defined(_WIN64)
#define PLATFORM_WINDOWS 1
#define PLATFORM_LINUX   0
#define ARCH_X64         1
#elif defined(_WIN32)
#define PLATFORM_WINDOWS 1
#define PLATFORM_LINUX   0
#define ARCH_X64         0
#elif defined(__linux__) && (defined(__x86_64__) || defined(__amd64__))
#define PLATFORM_WINDOWS 0
#define PLATFORM_LINUX   1
#define ARCH_X64         1
#elif defined(__linux__)
#define PLATFORM_WINDOWS 0
#define PLATFORM_LINUX   1
#define ARCH_X64         0
#else
#error Unsupported platform. Expected Windows or Linux.
#endif

/* Legacy NCD-compatible aliases (for backward compatibility) */
#define NCD_PLATFORM_WINDOWS PLATFORM_WINDOWS
#define NCD_PLATFORM_LINUX   PLATFORM_LINUX
#define NCD_ARCH_X64         ARCH_X64

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_DETECT_H */
