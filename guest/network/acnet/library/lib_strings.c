/* ACNet bsdsocket.library: errno and h_errno text for SocketBaseTagList.
 * BSD-3-Clause. */
#include <exec/types.h>
#include "lib_internal.h"

const char *errno_string(LONG e)
{
    switch (e) {
    case 0: return "No error";
    case 1: return "Operation not permitted";
    case 2: return "No such file or directory";
    case 3: return "No such process";
    case 4: return "Interrupted system call";
    case 5: return "Input/output error";
    case 6: return "Device not configured";
    case 7: return "Argument list too long";
    case 8: return "Exec format error";
    case 9: return "Bad file descriptor";
    case 10: return "No child processes";
    case 11: return "Resource deadlock avoided";
    case 12: return "Cannot allocate memory";
    case 13: return "Permission denied";
    case 14: return "Bad address";
    case 15: return "Block device required";
    case 16: return "Device or resource busy";
    case 17: return "File exists";
    case 18: return "Cross-device link";
    case 19: return "Operation not supported by device";
    case 20: return "Not a directory";
    case 21: return "Is a directory";
    case 22: return "Invalid argument";
    case 23: return "Too many open files in system";
    case 24: return "Too many open files";
    case 25: return "Inappropriate ioctl for device";
    case 26: return "Text file busy";
    case 27: return "File too large";
    case 28: return "No space left on device";
    case 29: return "Illegal seek";
    case 30: return "Read-only file system";
    case 31: return "Too many links";
    case 32: return "Broken pipe";
    case 33: return "Numerical argument out of domain";
    case 34: return "Result too large";
    case 35: return "Resource temporarily unavailable";
    case 36: return "Operation now in progress";
    case 37: return "Operation already in progress";
    case 38: return "Socket operation on non-socket";
    case 39: return "Destination address required";
    case 40: return "Message too long";
    case 41: return "Protocol wrong type for socket";
    case 42: return "Protocol option not available";
    case 43: return "Protocol not supported";
    case 44: return "Socket type not supported";
    case 45: return "Operation not supported";
    case 46: return "Protocol family not supported";
    case 47: return "Address family not supported";
    case 48: return "Address already in use";
    case 49: return "Cannot assign requested address";
    case 50: return "Network is down";
    case 51: return "Network is unreachable";
    case 52: return "Network dropped connection on reset";
    case 53: return "Software caused connection abort";
    case 54: return "Connection reset by peer";
    case 55: return "No buffer space available";
    case 56: return "Socket is already connected";
    case 57: return "Socket is not connected";
    case 58: return "Cannot send after socket shutdown";
    case 59: return "Too many references";
    case 60: return "Connection timed out";
    case 61: return "Connection refused";
    case 62: return "Too many levels of symbolic links";
    case 63: return "File name too long";
    case 64: return "Host is down";
    case 65: return "No route to host";
    case 66: return "Directory not empty";
    case 68: return "Too many users";
    case 69: return "Disk quota exceeded";
    case 70: return "Stale file handle";
    case 71: return "Too many levels of remote in path";
    case 77: return "No locks available";
    case 78: return "Function not implemented";
    default: return "Unknown network error";
    }
}

const char *herrno_string(LONG e)
{
    switch (e) {
    case 0: return "No resolver error";
    case AH_HOST_NOT_FOUND: return "Unknown host";
    case AH_TRY_AGAIN: return "Temporary resolver failure";
    case AH_NO_RECOVERY: return "Non-recoverable resolver failure";
    case AH_NO_DATA: return "No address data";
    default: return "Unknown resolver error";
    }
}
