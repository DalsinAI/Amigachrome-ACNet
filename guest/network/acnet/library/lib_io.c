/* ACNet bsdsocket.library: stream/datagram I/O. BSD-3-Clause. */
#include <exec/types.h>
#include <proto/exec.h>
#include <sys/socket.h>
#include "lib_internal.h"

static LONG io_wait(struct SocketBase *sb, struct FD *f, ULONG ev, int send)
{
    struct timeval *tv=send?&f->sndtimeo:&f->rcvtimeo;
    LONG r=wait_ready(sb,f->handle,ev,tv);
    if(r==0) return fail(sb,AE_TIMEDOUT);
    return r<0?-1:0;
}

LONG bsd_send(struct SocketBase *sb,LONG s,APTR buf,LONG len,LONG flags)
{
    struct FD *f=fd_get(sb,s); if(!f)return -1; if(len<0)return fail(sb,AE_INVAL);
    for(;;){LONG r=prov_call(sb,ACHS_CMD_SEND,f->handle,flags,0,0,buf,len,NULL,0,NULL); if(r>=0)return r;
      if(sb->last_err!=AE_AGAIN||f->nonblock)return fail_provider(sb);
      if(io_wait(sb,f,ACHS_POLLOUT,1)<0)return -1;}
}
LONG bsd_recv(struct SocketBase *sb,LONG s,APTR buf,LONG len,LONG flags)
{
    struct FD *f=fd_get(sb,s); ULONG n=0; if(!f)return -1; if(len<0)return fail(sb,AE_INVAL);
    for(;;){LONG r=prov_call(sb,ACHS_CMD_RECV,f->handle,len,flags,0,NULL,0,buf,len,&n); if(r>=0)return r;
      if(sb->last_err!=AE_AGAIN||f->nonblock)return fail_provider(sb);
      if(io_wait(sb,f,ACHS_POLLIN,0)<0)return -1;}
}
LONG bsd_sendto(struct SocketBase *sb,LONG s,APTR buf,LONG len,LONG flags,struct sockaddr *to,socklen_t tolen)
{
    struct FD *f=fd_get(sb,s); UBYTE *x=scratch(sb); if(!f||!x)return -1; if(len<0||!to||tolen>16)return fail(sb,AE_INVAL);
    CopyMem(to,x,tolen); CopyMem(buf,x+16,len); for(;;){LONG r=prov_call(sb,ACHS_CMD_SENDTO,f->handle,flags,0,0,x,16+len,NULL,0,NULL); if(r>=0)return r;
      if(sb->last_err!=AE_AGAIN||f->nonblock)return fail_provider(sb);
      if(io_wait(sb,f,ACHS_POLLOUT,1)<0)return -1;}
}
LONG bsd_recvfrom(struct SocketBase *sb,LONG s,APTR buf,LONG len,LONG flags,struct sockaddr *addr,socklen_t *addrlen)
{
    struct FD *f=fd_get(sb,s); UBYTE *x=scratch(sb); ULONG n=0; if(!f||!x)return -1;
    for(;;){LONG r=prov_call(sb,ACHS_CMD_RECVFROM,f->handle,len,flags,0,NULL,0,x,SCRATCH_SIZE,&n); if(r>=0){
      if(n>=16){ULONG d=n-16; if(d>(ULONG)len)d=len; CopyMem(x+16,buf,d); if(addr&&addrlen){ULONG m=*addrlen<16?*addrlen:16;CopyMem(x,addr,m);*addrlen=16;} return r;} return r;}
      if(sb->last_err!=AE_AGAIN||f->nonblock)return fail_provider(sb);
      if(io_wait(sb,f,ACHS_POLLIN,0)<0)return -1;}
}
