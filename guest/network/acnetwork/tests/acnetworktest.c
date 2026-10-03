#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/acnetwork.h>

struct Library *ACNetworkBase;

static void mark(const char *s)
{
    LONG n=0;
    while(s[n]) n++;
    Write(Output(), (APTR)s, n);
}

int main(void)
{
    ULONG state, mask;
    mark("BEFORE_OPEN\n");
    ACNetworkBase=OpenLibrary(ACNETWORK_LIBRARY_NAME, ACNETWORK_LIBRARY_VERSION);
    if(!ACNetworkBase){ mark("OPEN_FAIL\n"); return 20; }
    mark("AFTER_OPEN\n");
    state=ACNetwork_State();
    (void)state;
    mark("AFTER_STATE\n");
    mask=ACNetwork_SignalMask();
    (void)mask;
    mark("AFTER_SIGNALMASK\n");
    CloseLibrary(ACNetworkBase);
    mark("AFTER_CLOSE\n");
    return 0;
}
