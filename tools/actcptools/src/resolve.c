/* ACTCPTools resolve for AmigaOS 3.2.3.
 * BSD-3-Clause. Uses only the published bsdsocket.library interface. */
#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <proto/bsdsocket.h>
#include <stdio.h>
#include <string.h>

struct Library *SocketBase;

static int usage(void)
{
    fprintf(stderr, "Usage: resolve hostname\n");
    return 20;
}

int main(int argc, char **argv)
{
    struct hostent *he;
    char **p;
    if (argc != 2) return usage();
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (!SocketBase) {
        fprintf(stderr, "resolve: cannot open bsdsocket.library V4\n");
        return 20;
    }
    he = gethostbyname(argv[1]);
    if (!he || he->h_addrtype != AF_INET || he->h_length != 4) {
        fprintf(stderr, "resolve: lookup failed for %s\n", argv[1]);
        CloseLibrary(SocketBase);
        return 10;
    }
    printf("Name: %s\n", he->h_name ? (char *)he->h_name : argv[1]);
    for (p = he->h_addr_list; p && *p; ++p) {
        struct in_addr addr;
        memcpy(&addr, *p, 4);
        printf("Address: %s\n", (char *)Inet_NtoA((LONG)addr.s_addr));
    }
    CloseLibrary(SocketBase);
    return 0;
}