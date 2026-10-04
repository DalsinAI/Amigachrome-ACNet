#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>
#include <sys/socket.h>
#include <netdb.h>

struct Library *SocketBase;

static void mark(const char *s)
{
    LONG n=0;
    while(s[n]) n++;
    Write(Output(), (APTR)s, n);
}

int main(void)
{
    char host[64];
    struct hostent *he;
    LONG s;
    mark("BEFORE_BSD_OPEN\n");
    SocketBase=OpenLibrary("bsdsocket.library",4);
    if(!SocketBase){ mark("BSD_OPEN_FAIL\n"); return 20; }
    mark("AFTER_BSD_OPEN\n");
    (void)getdtablesize();
    mark("AFTER_DTABLE\n");
    if(gethostname(host,sizeof(host))==0) mark("AFTER_HOSTNAME\n");
    else mark("HOSTNAME_FAIL\n");
    mark("BEFORE_DNS\n");
    he=gethostbyname("localhost");
    if(he) mark("AFTER_DNS\n");
    else mark("DNS_FAIL\n");
    mark("BEFORE_SOCKET\n");
    s=socket(AF_INET,SOCK_STREAM,0);
    if(s>=0){
        mark("AFTER_SOCKET\n");
        CloseSocket(s);
        mark("AFTER_CLOSESOCKET\n");
    } else mark("SOCKET_FAIL\n");
    CloseLibrary(SocketBase);
    mark("AFTER_BSD_CLOSE\n");
    return 0;
}
