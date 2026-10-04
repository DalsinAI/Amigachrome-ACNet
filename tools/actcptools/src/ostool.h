/* ostool: what OpenSocket's newer command-line tools share (host, whois,
 * finger, telnet, ftp, tftp, sntp, httpget, nc). They use only the published
 * bsdsocket.library interface, so they run on any Amiga TCP/IP stack.
 * MIT. */
#ifndef OSTOOL_H
#define OSTOOL_H

#include <exec/types.h>

extern struct Library *SocketBase;

/* Opens bsdsocket.library 4, or says why not. prog is the tool's name. */
int ost_open(const char *prog);
void ost_close(void);

/* A name or a dotted address -> addr (network order). 0, with a message, when not found. */
int ost_resolve(const char *prog, const char *host, ULONG *addr);

/* A TCP connection to host:port, given up after `seconds`. The socket, or
 * -1 with a message. Ctrl-C stops the wait. */
LONG ost_connect(const char *prog, const char *host, unsigned port, int seconds);

/* Ctrl-C since the last look (and clears it). */
int ost_break(void);

/* Waits up to ms for the socket to be readable: 1 readable, 0 not yet,
 * -1 Ctrl-C or an error. */
int ost_wait_read(LONG s, long ms);

/* Sends all of buf: 0, or -1. */
int ost_send_all(LONG s, const void *buf, long len);
int ost_send_str(LONG s, const char *text);

/* Lines from a socket, LF-terminated (a CR before it is dropped). */
struct ost_lines {
    LONG s;
    long have, at;
    char buf[1024];
};
void ost_lines_init(struct ost_lines *l, LONG s);
/* The next line into out (at most max - 1 characters): its length (0 for an
 * empty line), -2 at the end of the stream, -1 on an error, Ctrl-C or
 * `seconds` without one. */
long ost_line(struct ost_lines *l, char *out, long max, int seconds);
/* Bytes already read past the last line: for a body after headers. */
long ost_lines_take(struct ost_lines *l, char *out, long max);

/* Runs body on a 16 KB stack: libnix ignores __stack. */
int ost_main(int (*body)(int, char **), int argc, char **argv);

#endif
