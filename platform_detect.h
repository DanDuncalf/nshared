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
 *   Windows ARM64: _M_ARM64 (MSVC) or __aarch64__ (MinGW/Clang)
 *   Linux:   __linux__
 *   x64:     _WIN64 (MSVC) or __x86_64__ / __amd64__ (GCC/Clang)
 *   ARM64:   __aarch64__ (GCC/Clang) or _M_ARM64 (MSVC)
 *   RISC-V:  __riscv && __riscv_xlen == 64
 */
#if defined(_WIN64)
#   if defined(_M_ARM64) || defined(__aarch64__)
#       define PLATFORM_WINDOWS 1
#       define PLATFORM_LINUX   0
#       define ARCH_X64         0
#       define ARCH_ARM64       1
#       define ARCH_RISCV64     0
#   else
#       define PLATFORM_WINDOWS 1
#       define PLATFORM_LINUX   0
#       define ARCH_X64         1
#       define ARCH_ARM64       0
#       define ARCH_RISCV64     0
#   endif
#elif defined(_WIN32)
#   define PLATFORM_WINDOWS 1
#   define PLATFORM_LINUX   0
#   define ARCH_X64         0
#   define ARCH_ARM64       0
#   define ARCH_RISCV64     0
#elif defined(__linux__) && (defined(__x86_64__) || defined(__amd64__))
#   define PLATFORM_WINDOWS 0
#   define PLATFORM_LINUX   1
#   define ARCH_X64         1
#   define ARCH_ARM64       0
#   define ARCH_RISCV64     0
#elif defined(__linux__) && defined(__aarch64__)
#   define PLATFORM_WINDOWS 0
#   define PLATFORM_LINUX   1
#   define ARCH_X64         0
#   define ARCH_ARM64       1
#   define ARCH_RISCV64     0
#elif defined(__linux__) && defined(__riscv) && (__riscv_xlen == 64)
#   define PLATFORM_WINDOWS 0
#   define PLATFORM_LINUX   1
#   define ARCH_X64         0
#   define ARCH_ARM64       0
#   define ARCH_RISCV64     1
#elif defined(__linux__)
#   define PLATFORM_WINDOWS 0
#   define PLATFORM_LINUX   1
#   define ARCH_X64         0
#   define ARCH_ARM64       0
#   define ARCH_RISCV64     0
#else
#error Unsupported platform. Expected Windows or Linux.
#endif

/* Legacy NCD-compatible aliases (for backward compatibility) */
#define NCD_PLATFORM_WINDOWS PLATFORM_WINDOWS
#define NCD_PLATFORM_LINUX   PLATFORM_LINUX
#define NCD_ARCH_X64         ARCH_X64
#define NCD_ARCH_ARM64       ARCH_ARM64
#define NCD_ARCH_RISCV64     ARCH_RISCV64

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_DETECT_H */
