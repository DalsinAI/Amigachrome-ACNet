/* ACTCPTools hostname for AmigaOS 3.2.3.
 * MIT. Uses only the published bsdsocket.library interface. */
#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <stdio.h>

struct Library *SocketBase;

int main(void)
{
    char name[256];
    SocketBase = OpenLibrary("bsdsocket.library", 4);
    if (!SocketBase) {
        fprintf(stderr, "hostname: cannot open bsdsocket.library V4\n");
        return 20;
    }
    if (gethostname(name, sizeof(name)) != 0) {
        fprintf(stderr, "hostname: gethostname failed\n");
        CloseLibrary(SocketBase);
        return 10;
    }
    name[sizeof(name) - 1] = '\0';
    puts(name);
    CloseLibrary(SocketBase);
    return 0;
}