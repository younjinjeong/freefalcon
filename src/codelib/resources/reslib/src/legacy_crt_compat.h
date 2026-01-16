/* legacy_crt_compat.h
 *
 * Compatibility shims for legacy C runtime FILE struct access
 *
 * Modern C runtime (Visual Studio 2015+) no longer exposes FILE struct internals.
 * This header provides compatibility macros/functions to bridge the gap.
 *
 * NOTE: This is a TEMPORARY solution. The proper fix is to rewrite resmgr.c
 * to use standard C APIs (fread, fseek, ftell, etc.) instead of direct struct access.
 *
 * TODO: Refactor resmgr.c to eliminate need for this header.
 */

#ifndef LEGACY_CRT_COMPAT_H
#define LEGACY_CRT_COMPAT_H

#include <stdio.h>

// For modern CRT, we need to work around the opaque FILE structure
// We'll use standard C functions instead of direct member access

// Legacy FILE flags that are no longer accessible
#define _IOREAD   0x0001
#define _IOWRT    0x0002
#define _IORW     0x0080
#define _IOEOF    0x0010
#define _IOERR    0x0020
#define _IOMYBUF  0x0008
#define _IOSETVBUF 0x0400
#define _IOCTRLZ  0x4000
#define _IOSTRG   0x0040
#define _IOLOOSE  0x1000  // Custom flag
#define _IOARCHIVE 0x2000 // Custom flag

// Helper functions to replace direct FILE struct access
// Note: These are workarounds and don't provide full functionality

inline int _file_get_cnt(FILE* stream) {
    // Can't get buffer count directly, return 0 to force refill
    return 0;
}

inline void _file_set_cnt(FILE* stream, int value) {
    // No-op in modern CRT
}

inline char* _file_get_ptr(FILE* stream) {
    // Can't get buffer pointer, return NULL
    return NULL;
}

inline void _file_set_ptr(FILE* stream, char* ptr) {
    // No-op in modern CRT
}

inline char* _file_get_base(FILE* stream) {
    // Can't get buffer base, return NULL
    return NULL;
}

inline int _file_get_flag(FILE* stream) {
    // Reconstruct flags from FILE state
    int flags = 0;

    if (feof(stream)) flags |= _IOEOF;
    if (ferror(stream)) flags |= _IOERR;

    // Can't determine other flags reliably
    return flags;
}

inline void _file_set_flag(FILE* stream, int flags) {
    // Limited support - can only clear error/eof
    if ( not (flags bitand _IOEOF)) {
        clearerr(stream);
    }
}

inline int _file_get_file(FILE* stream) {
    return _fileno(stream);
}

// WARNING: This implementation has limitations!
// The original code directly manipulates FILE buffers for performance.
// This compatibility layer falls back to standard I/O which may be slower.
//
// For a production system, resmgr.c should be rewritten to:
// 1. Use standard fread/fwrite instead of buffer manipulation
// 2. Implement custom buffering if needed for performance
// 3. Remove all direct FILE struct member access

#endif // LEGACY_CRT_COMPAT_H
