/* ACNet bsdsocket.library: IPv4 address conversion helpers. BSD-3-Clause. */
#include <exec/types.h>
#include <proto/exec.h>
#include <netinet/in.h>
#include <acnetwork.h>
#include "lib_internal.h"

static int digit(char c){return c>='0'&&c<='9';}
static LONG parse4(const char*s,ULONG*out){ULONG v=0,a=0;int part=0,d=0;if(!s)return 0;while(*s){if(digit(*s)){v=v*10+(*s-'0');if(v>255)return 0;d=1;}else if(*s=='.'&&d&&part<3){a=(a<<8)|v;part++;v=0;d=0;}else return 0;s++;}if(!d||part!=3)return 0;*out=(a<<8)|v;return 1;}
static char *putnum(char*p,ULONG v){if(v>=100)*p++='0'+v/100;if(v>=10)*p++='0'+(v/10)%10;*p++='0'+v%10;return p;}
LONG bsd_inet_aton(struct SocketBase *sb,STRPTR cp,struct in_addr *addr){ULONG v;(void)sb;if(!addr||!parse4(cp,&v))return 0;addr->s_addr=v;return 1;}
in_addr_t bsd_inet_addr(struct SocketBase *sb,STRPTR cp){ULONG v;(void)sb;return parse4(cp,&v)?v:(in_addr_t)0xffffffffUL;}
STRPTR bsd_Inet_NtoA(struct SocketBase *sb,in_addr_t ip){char*p=sb->ntoa;p=putnum(p,(ip>>24)&255);*p++='.';p=putnum(p,(ip>>16)&255);*p++='.';p=putnum(p,(ip>>8)&255);*p++='.';p=putnum(p,ip&255);*p=0;return sb->ntoa;}
LONG bsd_inet_pton(struct SocketBase *sb,LONG af,STRPTR src,APTR dst){ULONG v;(void)sb;if(af!=AF_INET)return -1;if(!parse4(src,&v))return 0;*(ULONG*)dst=v;return 1;}
STRPTR bsd_inet_ntop(struct SocketBase *sb,LONG af,APTR src,STRPTR dst,LONG size){if(af!=AF_INET||!src||!dst||size<16){set_errno(sb,AE_INVAL);return NULL;}STRPTR s=bsd_Inet_NtoA(sb,*(ULONG*)src);LONG i=0;while(s[i]&&i<size-1){dst[i]=s[i];i++;}dst[i]=0;return dst;}
in_addr_t bsd_Inet_NetOf(struct SocketBase *sb,in_addr_t in){(void)sb;ULONG h=(in>>24)&255;return h<128?(in&0xff000000UL):h<192?(in&0xffff0000UL):(in&0xffffff00UL);}
in_addr_t bsd_Inet_LnaOf(struct SocketBase *sb,in_addr_t in){in_addr_t n=bsd_Inet_NetOf(sb,in);return in&~n;}
in_addr_t bsd_Inet_MakeAddr(struct SocketBase *sb,in_addr_t net,in_addr_t host){(void)sb;if(net<128)return(net<<24)|(host&0xffffff);if(net<65536)return(net<<16)|(host&0xffff);return(net<<8)|(host&0xff);}
in_addr_t bsd_inet_network(struct SocketBase *sb,STRPTR cp){ULONG v;(void)sb;return parse4(cp,&v)?v:(in_addr_t)0xffffffffUL;}
LONG bsd_In_LocalAddr(struct SocketBase *sb,in_addr_t a){struct ACNetworkInterface rows[16];ULONG n=0;LONG r=prov_call(sb,ACNETWORK_CMD_INTERFACES,16,0,0,0,NULL,0,rows,sizeof(rows),&n);LONG i,count;if(r<0)return 0;count=r;if((ULONG)count>n/sizeof(rows[0]))count=n/sizeof(rows[0]);for(i=0;i<count;i++){ULONG ip=0,mask=0;CopyMem(rows[i].ipv4,&ip,4);CopyMem(rows[i].netmask,&mask,4);if(a==ip||((a&mask)==(ip&mask)))return 1;}return 0;}
LONG bsd_In_CanForward(struct SocketBase *sb,in_addr_t a){(void)sb;ULONG h=(a>>24)&255;if(a==0xffffffffUL)return 0;return h!=0&&h!=127&&h<224;}
