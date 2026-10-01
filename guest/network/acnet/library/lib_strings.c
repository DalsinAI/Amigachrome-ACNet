/* ACNet bsdsocket.library: the text SocketBaseTags gives for an errno or
 * h_errno. Version 1 names the number only ("Error 61"); worded messages
 * come later. BSD-3-Clause. */
#include <exec/types.h>

#include "lib_internal.h"

static char errbuf[24], herrbuf[24];

/* prefix and the number, into buf. */
static const char *numbered(char *buf, const char *prefix, LONG n)
{
    char digits[12];
    int i = 0, k = 0;
    ULONG v = n < 0 ? (ULONG)-n : (ULONG)n;
    do { digits[i++] = (char)('0' + v % 10); v /= 10; } while (v && i < 11);
    while (*prefix) buf[k++] = *prefix++;
    if (n < 0) buf[k++] = '-';
    while (i) buf[k++] = digits[--i];
    buf[k] = 0;
    return buf;
}

const char *errno_string(LONG e) { return e ? numbered(errbuf, "Error ", e) : "No error"; }
const char *herrno_string(LONG e) { return e ? numbered(herrbuf, "Resolver error ", e) : "No error"; }
