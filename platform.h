/*
 * platform.h -- Cross-platform abstraction layer
 *
 * Provides portable console I/O, filesystem operations, threading,
 * and utility functions. Designed to be shared across projects.
 */

#ifndef SHARED_PLATFORM_H
#define SHARED_PLATFORM_H

#include "platform_detect.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <limits.h>
#include <strings.h>
#ifndef MAX_PATH
#define MAX_PATH 4096
#endif
typedef unsigned int  UINT;
typedef unsigned long DWORD;
typedef long          LONG;
#endif

#if PLATFORM_WINDOWS && !defined(PLATFORM_IMPL)
/* Compatibility wrapper layer: remap direct Win32 calls through platform.c */
HANDLE plat_CreateFileA(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
BOOL plat_CloseHandle(HANDLE);
BOOL plat_WriteConsoleA(HANDLE,const VOID*,DWORD,LPDWORD,LPVOID);
BOOL plat_GetConsoleScreenBufferInfo(HANDLE,PCONSOLE_SCREEN_BUFFER_INFO);
BOOL plat_SetConsoleCursorPosition(HANDLE,COORD);
BOOL plat_GetConsoleMode(HANDLE,LPDWORD);
BOOL plat_SetConsoleMode(HANDLE,DWORD);
BOOL plat_SetConsoleCursorInfo(HANDLE,const CONSOLE_CURSOR_INFO*);
BOOL plat_FillConsoleOutputCharacterA(HANDLE,CHAR,DWORD,COORD,LPDWORD);
BOOL plat_FillConsoleOutputAttribute(HANDLE,WORD,DWORD,COORD,LPDWORD);
BOOL plat_WriteConsoleOutputCharacterA(HANDLE,LPCSTR,DWORD,COORD,LPDWORD);
BOOL plat_ReadConsoleInputA(HANDLE,PINPUT_RECORD,DWORD,LPDWORD);
DWORD plat_GetEnvironmentVariableA(LPCSTR,LPSTR,DWORD);
DWORD plat_GetTempPathA(DWORD,LPSTR);
DWORD plat_GetCurrentDirectoryA(DWORD,LPSTR);
DWORD plat_GetModuleFileNameA(HMODULE,LPSTR,DWORD);
BOOL plat_DeleteFileA(LPCSTR);
BOOL plat_MoveFileA(LPCSTR,LPCSTR);
BOOL plat_CreateDirectoryA(LPCSTR,LPSECURITY_ATTRIBUTES);
DWORD plat_GetFileAttributesA(LPCSTR);
UINT plat_GetDriveTypeA(LPCSTR);
DWORD plat_GetLogicalDrives(void);
BOOL plat_GetVolumeInformationA(LPCSTR,LPSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPSTR,DWORD);
BOOL plat_CreateProcessA(LPCSTR,LPSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCSTR,LPSTARTUPINFOA,LPPROCESS_INFORMATION);
VOID plat_Sleep(DWORD);
DWORD plat_GetTickCount(void);
LONG plat_InterlockedIncrement(volatile LONG *);
LONG plat_InterlockedExchange(volatile LONG *, LONG);
HANDLE plat_CreateThread(LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD);
DWORD plat_WaitForSingleObject(HANDLE,DWORD);
HANDLE plat_FindFirstFileExA(LPCSTR,FINDEX_INFO_LEVELS,LPVOID,FINDEX_SEARCH_OPS,LPVOID,DWORD);
BOOL plat_FindNextFileA(HANDLE,LPWIN32_FIND_DATAA);
BOOL plat_FindClose(HANDLE);

#define CreateFileA plat_CreateFileA
#define CloseHandle plat_CloseHandle
#define WriteConsoleA plat_WriteConsoleA
#define GetConsoleScreenBufferInfo plat_GetConsoleScreenBufferInfo
#define SetConsoleCursorPosition plat_SetConsoleCursorPosition
#define GetConsoleMode plat_GetConsoleMode
#define SetConsoleMode plat_SetConsoleMode
#define SetConsoleCursorInfo plat_SetConsoleCursorInfo
#define FillConsoleOutputCharacterA plat_FillConsoleOutputCharacterA
#define FillConsoleOutputAttribute plat_FillConsoleOutputAttribute
#define WriteConsoleOutputCharacterA plat_WriteConsoleOutputCharacterA
#define ReadConsoleInputA plat_ReadConsoleInputA
#define GetEnvironmentVariableA plat_GetEnvironmentVariableA
#define GetTempPathA plat_GetTempPathA
#define GetCurrentDirectoryA plat_GetCurrentDirectoryA
#define GetModuleFileNameA plat_GetModuleFileNameA
#define DeleteFileA plat_DeleteFileA
#define MoveFileA plat_MoveFileA
#define CreateDirectoryA plat_CreateDirectoryA
#define GetFileAttributesA plat_GetFileAttributesA
#define GetDriveTypeA plat_GetDriveTypeA
#define GetLogicalDrives plat_GetLogicalDrives
#define GetVolumeInformationA plat_GetVolumeInformationA
#define CreateProcessA plat_CreateProcessA
#define Sleep plat_Sleep
#define GetTickCount plat_GetTickCount
#ifdef InterlockedIncrement
#undef InterlockedIncrement
#endif
#define InterlockedIncrement plat_InterlockedIncrement
#ifdef InterlockedExchange
#undef InterlockedExchange
#endif
#define InterlockedExchange plat_InterlockedExchange
#define CreateThread plat_CreateThread
#define WaitForSingleObject plat_WaitForSingleObject
#define FindFirstFileExA plat_FindFirstFileExA
#define FindNextFileA plat_FindNextFileA
#define FindClose plat_FindClose
#endif

typedef void *PlatformHandle;

typedef struct {
    int window_left;
    int window_right;
    int window_top;
    int window_bottom;
    int cursor_x;
    int cursor_y;
    int buffer_w;
    int buffer_h;
} PlatformConsoleInfo;

typedef struct {
    PlatformHandle h;
} PlatformFindHandle;

typedef struct {
    unsigned long attrs;
    char          name[MAX_PATH];
} PlatformFindData;

enum {
    PLATFORM_DRIVE_UNKNOWN     = 0,
    PLATFORM_DRIVE_NO_ROOT_DIR = 1,
    PLATFORM_DRIVE_REMOVABLE   = 2,
    PLATFORM_DRIVE_FIXED       = 3,
    PLATFORM_DRIVE_REMOTE      = 4,
    PLATFORM_DRIVE_CDROM       = 5,
    PLATFORM_DRIVE_RAMDISK     = 6
};

enum {
    PLATFORM_VK_UP    = 0x26,
    PLATFORM_VK_DOWN  = 0x28,
    PLATFORM_VK_LEFT  = 0x25,
    PLATFORM_VK_RIGHT = 0x27,
    PLATFORM_VK_PRIOR = 0x21,
    PLATFORM_VK_NEXT  = 0x22,
    PLATFORM_VK_HOME  = 0x24,
    PLATFORM_VK_END   = 0x23,
    PLATFORM_VK_RETURN = 0x0D,
    PLATFORM_VK_ESCAPE = 0x1B,
    PLATFORM_VK_TAB    = 0x09,
    PLATFORM_VK_BACK   = 0x08
};

/* Console I/O */
PlatformHandle platform_console_open_out(bool read_write);
PlatformHandle platform_console_open_in(void);
void platform_handle_close(PlatformHandle h);
bool platform_console_write(PlatformHandle h, const char *s);
bool platform_console_get_info(PlatformHandle h, PlatformConsoleInfo *out);
bool platform_console_set_cursor(PlatformHandle h, int row, int col);
bool platform_console_enable_ansi(PlatformHandle h);
bool platform_console_set_cursor_visible(PlatformHandle h, bool visible, int size);
bool platform_console_get_mode(PlatformHandle h, unsigned long *mode);
bool platform_console_set_mode(PlatformHandle h, unsigned long mode);
bool platform_console_fill_char(PlatformHandle h, int row, int col, int count, char ch);
bool platform_console_fill_attr_default(PlatformHandle h, int row, int col, int count);
bool platform_console_write_at(PlatformHandle h, int row, int col, const char *s);
bool platform_console_read_key_vk(PlatformHandle h, unsigned short *out_vk);

/* Environment and paths */
bool platform_get_env(const char *name, char *buf, size_t buf_size);
bool platform_get_temp_path(char *buf, size_t buf_size);
bool platform_get_current_dir(char *buf, size_t buf_size);
bool platform_get_module_path(char *buf, size_t buf_size);

/* Filesystem operations */
bool platform_delete_file(const char *path);
bool platform_move_file(const char *src, const char *dst);
bool platform_create_dir(const char *path);
bool platform_file_exists(const char *path);
bool platform_dir_exists(const char *path);

/* Mount/drive enumeration helpers */
char platform_get_drive_letter(const char *mount);
int  platform_get_mount_type(const char *mount);
int  platform_enumerate_mounts(char mount_bufs[][MAX_PATH],
                               const char *mount_ptrs[],
                               size_t buf_size,
                               int max_mounts);

/* Database path helpers (customizable per project) */
bool platform_db_default_path(char *buf, size_t buf_size);
bool platform_db_drive_path(char letter, char *buf, size_t buf_size);

/* String utilities */
bool platform_strncpy_s(char *dst, size_t dstsz, const char *src);
bool platform_strncat_s(char *dst, size_t dstsz, const char *src);
char *platform_strtok(char *str, const char *delim, char **saveptr);

/* Drive information (Windows-style API, mapped on Linux) */
unsigned int platform_get_drive_type(const char *root);
unsigned long platform_get_logical_drives_mask(void);
bool platform_get_volume_label(const char *root, char *label, size_t label_size);

/* Process management */
bool platform_spawn_detached(const char *cmd);

/* Timing and synchronization */
void platform_sleep_ms(unsigned long ms);
unsigned long platform_tick_ms(void);
long platform_atomic_inc(volatile long *v);
long platform_atomic_exchange(volatile long *dst, long value);
long platform_atomic_read(volatile long *v);

/* CRC64 checksum (ECMA-182 polynomial) */
unsigned long long platform_crc64(const void *data, size_t len);
unsigned long long platform_crc64_update(unsigned long long crc, const void *data, size_t len);

/* Threading */
PlatformHandle platform_thread_create(unsigned long (*fn)(void *), void *param);
void platform_thread_wait(PlatformHandle h, unsigned long ms);

/* Directory enumeration */
bool platform_find_dirs_begin(const char *pattern, PlatformFindHandle *fh, PlatformFindData *fd);
bool platform_find_next(PlatformFindHandle *fh, PlatformFindData *fd);
void platform_find_close(PlatformFindHandle *fh);
bool platform_find_is_directory(const PlatformFindData *fd);
bool platform_find_is_hidden(const PlatformFindData *fd);
bool platform_find_is_system(const PlatformFindData *fd);
bool platform_find_is_reparse(const PlatformFindData *fd);

#ifdef __cplusplus
}
#endif

#endif /* SHARED_PLATFORM_H */
