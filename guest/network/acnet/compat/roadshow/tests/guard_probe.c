#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/bsdsocket.h>
#include <sys/errno.h>
#include <stdio.h>

struct Library *SocketBase;

static LONG raw_scalar_57(void)
{
    register struct Library *a6 __asm("a6") = SocketBase;
    register LONG d0 __asm("d0");
    __asm volatile ("jsr -366(a6)" : "=r"(d0) : "r"(a6) : "d1","a0","a1","cc","memory");
    return d0;
}

static APTR raw_ptr_69(void)
{
    register struct Library *a6 __asm("a6") = SocketBase;
    register APTR d0 __asm("d0");
    __asm volatile ("jsr -438(a6)" : "=r"(d0) : "r"(a6) : "d1","a0","a1","cc","memory");
    return d0;
}

static ULONG raw_reserved_139(void)
{
    register struct Library *a6 __asm("a6") = SocketBase;
    register ULONG d0 __asm("d0");
    __asm volatile ("jsr -858(a6)" : "=r"(d0) : "r"(a6) : "d1","a0","a1","cc","memory");
    return d0;
}

int main(void)
{
    int pass=0, fail=0;
    LONG s; APTR p; ULONG z;
    SocketBase=OpenLibrary("bsdsocket.library",4);
    if(!SocketBase){PutStr("GUARD: OpenLibrary FAIL\n");return 20;}

    s=raw_scalar_57();
    if(s==-1 && Errno()==ENOSYS){PutStr("slot57 scalar guard PASS\n");pass++;}else{printf("slot57 FAIL r=%d errno=%d\n",(int)s,(int)Errno());fail++;}

    p=raw_ptr_69();
    if(p==NULL && Errno()==ENOSYS){PutStr("slot69 pointer guard PASS\n");pass++;}else{printf("slot69 FAIL p=%p errno=%d\n",p,(int)Errno());fail++;}

    z=raw_reserved_139();
    if(z==0 && Errno()==ENOSYS){PutStr("slot139 end guard PASS\n");pass++;}else{printf("slot139 FAIL r=%u errno=%d\n",(unsigned)z,(int)Errno());fail++;}

    printf("GUARD PASS=%d FAIL=%d\n",pass,fail);
    CloseLibrary(SocketBase);
    return fail?20:0;
}
