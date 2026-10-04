/* ACNet optional modern/Roadshow compatibility helpers.
 * These live in the compatibility tail; the classic core ABI stays unchanged.
 * IPv4 only in ACNet v1. MIT. */
#include <exec/types.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "../../library/lib_internal.h"

struct AINode {
    struct addrinfo ai;
    struct sockaddr_in sin;
    char canon[256];
};

static ULONG m_slen(const char *s, ULONG max)
{
    ULONG n = 0;
    if (!s) return 0;
    while (n < max && s[n]) ++n;
    return n;
}

static LONG m_copy(char *dst, ULONG cap, const char *src)
{
    ULONG n = m_slen(src ? src : "", 65535);
    if (!dst || cap == 0 || n >= cap) return 0;
    if (n) CopyMem((APTR)src, dst, n);
    dst[n] = 0;
    return 1;
}
static UBYTE *align4(UBYTE *p)
{
    ULONG v = (ULONG)p;
    v = (v + 3UL) & ~3UL;
    return (UBYTE *)v;
}

static struct hostent *copy_hostent_r(struct SocketBase *sb, struct hostent *src,
                                      struct hostent *hp, APTR raw, ULONG buflen, LONG *he)
{
    UBYTE *p = (UBYTE *)raw, *end = p + buflen;
    ULONG name_len, count = 0, i;
    char **aliases, **addrs;

    if (he) *he = AH_NO_RECOVERY;
    if (!src || !hp || !raw) {
        set_errno(sb, AE_FAULT);
        return NULL;
    }

    {
        const char *official = src->h_name ? (const char *)src->h_name : "";
        name_len = m_slen(official, 255) + 1;
        while (src->h_addr_list && src->h_addr_list[count] && count < NAME_MAX_ADDRS) ++count;

        if (p + name_len > end) goto small;
        hp->h_name = (STRPTR)p;
        CopyMem((APTR)official, p, name_len);
    }
    p += name_len;

    p = align4(p);
    if (p + sizeof(char *) > end) goto small;
    aliases = (char **)p; aliases[0] = NULL; p += sizeof(char *);

    p = align4(p);
    if (p + (count + 1) * sizeof(char *) > end) goto small;
    addrs = (char **)p; p += (count + 1) * sizeof(char *);

    for (i = 0; i < count; ++i) {
        if (p + 4 > end) goto small;
        addrs[i] = (char *)p;
        CopyMem(src->h_addr_list[i], p, 4);
        p += 4;
    }
    addrs[count] = NULL;
    hp->h_aliases = (STRPTR *)aliases;
    hp->h_addrtype = src->h_addrtype;
    hp->h_length = src->h_length;
    hp->h_addr_list = addrs;
    if (he) *he = 0;
    set_errno(sb, 0);
    return hp;

small:
    set_errno(sb, 34); /* ERANGE */
    if (he) *he = AH_NO_RECOVERY;
    return NULL;
}

struct hostent *bsd_gethostbyname_r(struct SocketBase *sb, STRPTR name,
                                    struct hostent *hp, APTR buf, ULONG buflen, LONG *he)
{
    struct hostent *src = bsd_gethostbyname(sb, name);
    if (!src) { if (he) *he = sb->herrno_val; return NULL; }
    return copy_hostent_r(sb, src, hp, buf, buflen, he);
}

struct hostent *bsd_gethostbyaddr_r(struct SocketBase *sb, STRPTR addr, LONG len, LONG type,
                                    struct hostent *hp, APTR buf, ULONG buflen, LONG *he)
{
    struct hostent *src = bsd_gethostbyaddr(sb, addr, len, type);
    if (!src) { if (he) *he = sb->herrno_val; return NULL; }
    return copy_hostent_r(sb, src, hp, buf, buflen, he);
}

static LONG parse_dec(const char *s, ULONG *value)
{
    ULONG v = 0;
    if (!s || !*s) return 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        v = v * 10 + (ULONG)(*s++ - '0');
        if (v > 65535UL) return 0;
    }
    *value = v;
    return 1;
}
static LONG host_error_to_eai(struct SocketBase *sb)
{
    switch (sb->herrno_val) {
        case AH_HOST_NOT_FOUND: return EAI_NONAME;
        case AH_TRY_AGAIN:      return EAI_AGAIN;
        case AH_NO_DATA:        return EAI_NODATA;
        default:                return EAI_FAIL;
    }
}

static struct AINode *new_ai(void)
{
    return (struct AINode *)AllocMem(sizeof(struct AINode), MEMF_PUBLIC | MEMF_CLEAR);
}

VOID bsd_freeaddrinfo(struct SocketBase *sb, struct addrinfo *ai)
{
    (void)sb;
    while (ai) {
        struct addrinfo *next = ai->ai_next;
        FreeMem((APTR)ai, sizeof(struct AINode));
        ai = next;
    }
}

static LONG add_ai(struct addrinfo **head, struct addrinfo **tail,
                   ULONG addr, UWORD port, LONG flags, LONG type, LONG proto,
                   const char *canon)
{
    struct AINode *n = new_ai();
    if (!n) return EAI_MEMORY;

    n->sin.sin_len = sizeof(struct sockaddr_in);
    n->sin.sin_family = AF_INET;
    n->sin.sin_port = port;
    n->sin.sin_addr.s_addr = addr;

    n->ai.ai_flags = flags;
    n->ai.ai_family = AF_INET;
    n->ai.ai_socktype = type;
    n->ai.ai_protocol = proto;
    n->ai.ai_addrlen = sizeof(struct sockaddr_in);
    n->ai.ai_addr = (struct sockaddr *)&n->sin;
    if (canon && *canon) {
        m_copy(n->canon, sizeof(n->canon), canon);
        n->ai.ai_canonname = n->canon;
    }
    if (*tail) (*tail)->ai_next = &n->ai;
    else *head = &n->ai;
    *tail = &n->ai;
    return 0;
}

LONG bsd_getaddrinfo(struct SocketBase *sb, STRPTR hostname, STRPTR servname,
                     struct addrinfo *hints, struct addrinfo **res)
{
    LONG flags = 0, family = AF_INET, want_type = 0, want_proto = 0;
    LONG types[2], protos[2], combos = 0, c, rc = 0;
    ULONG addresses[NAME_MAX_ADDRS], addr_count = 0, i;
    ULONG numeric_port = 0;
    struct addrinfo *head = NULL, *tail = NULL;
    const char *canon = NULL;

    if (!res) return EAI_FAIL;
    *res = NULL;
    if (!hostname && !servname) return EAI_NONAME;

    if (hints) {
        flags = hints->ai_flags;
        family = hints->ai_family;
        want_type = hints->ai_socktype;
        want_proto = hints->ai_protocol;
        if (flags & ~AI_MASK) return EAI_BADFLAGS;
        if (family != 0 && family != AF_INET) return EAI_FAMILY;
        if (want_type != 0 && want_type != SOCK_STREAM && want_type != SOCK_DGRAM) return EAI_SOCKTYPE;
        if (want_proto != 0 && want_proto != IPPROTO_TCP && want_proto != IPPROTO_UDP) return EAI_PROTOCOL;
    }

    if (want_type == SOCK_STREAM || want_proto == IPPROTO_TCP) {
        if (want_type == SOCK_DGRAM || want_proto == IPPROTO_UDP) return EAI_BADHINTS;
        types[0] = SOCK_STREAM; protos[0] = IPPROTO_TCP; combos = 1;
    } else if (want_type == SOCK_DGRAM || want_proto == IPPROTO_UDP) {
        types[0] = SOCK_DGRAM; protos[0] = IPPROTO_UDP; combos = 1;
    } else {
        types[0] = SOCK_STREAM; protos[0] = IPPROTO_TCP;
        types[1] = SOCK_DGRAM;  protos[1] = IPPROTO_UDP;
        combos = 2;
    }
    if (!hostname) {
        addresses[0] = (flags & AI_PASSIVE) ? 0UL : 0x7f000001UL;
        addr_count = 1;
    } else {
        struct in_addr a;
        if (bsd_inet_aton(sb, hostname, &a)) {
            addresses[0] = a.s_addr;
            addr_count = 1;
            canon = hostname;
        } else {
            struct hostent *h;
            if (flags & AI_NUMERICHOST) return EAI_NONAME;
            h = bsd_gethostbyname(sb, hostname);
            if (!h) return host_error_to_eai(sb);
            canon = h->h_name ? h->h_name : hostname;
            while (h->h_addr_list && h->h_addr_list[addr_count] && addr_count < NAME_MAX_ADDRS) {
                CopyMem(h->h_addr_list[addr_count], &addresses[addr_count], 4);
                ++addr_count;
            }
            if (!addr_count) return EAI_NODATA;
        }
    }

    if (servname && !parse_dec(servname, &numeric_port) && (flags & AI_NUMERICSERV))
        return EAI_NONAME;

    for (i = 0; i < addr_count; ++i) {
        for (c = 0; c < combos; ++c) {
            UWORD port = (UWORD)numeric_port;
            if (servname && !parse_dec(servname, &numeric_port)) {
                struct servent *se = bsd_getservbyname(sb, servname,
                                      protos[c] == IPPROTO_TCP ? (STRPTR)"tcp" : (STRPTR)"udp");
                if (!se) continue;
                port = (UWORD)se->s_port;
            } else {
                port = (UWORD)numeric_port;
            }
            rc = add_ai(&head, &tail, addresses[i], port, flags, types[c], protos[c],
                        ((flags & AI_CANONNAME) && !head) ? canon : NULL);
            if (rc) { bsd_freeaddrinfo(sb, head); return rc; }
        }
    }
    if (!head) return servname ? EAI_SERVICE : EAI_NONAME;
    *res = head;
    set_errno(sb, 0);
    return 0;
}

STRPTR bsd_gai_strerror(struct SocketBase *sb, LONG e)
{
    (void)sb;
    switch (e) {
        case 0: return (STRPTR)"No error";
        case EAI_BADFLAGS: return (STRPTR)"Invalid address-info flags";
        case EAI_NONAME: return (STRPTR)"Name or service not known";
        case EAI_AGAIN: return (STRPTR)"Temporary name resolution failure";
        case EAI_FAIL: return (STRPTR)"Non-recoverable name resolution failure";
        case EAI_NODATA: return (STRPTR)"No address data";
        case EAI_FAMILY: return (STRPTR)"Address family not supported";
        case EAI_SOCKTYPE: return (STRPTR)"Socket type not supported";
        case EAI_SERVICE: return (STRPTR)"Service not supported";
        case EAI_ADDRFAMILY: return (STRPTR)"Address family unavailable for name";
        case EAI_MEMORY: return (STRPTR)"Memory allocation failure";
        case EAI_SYSTEM: return (STRPTR)"System error";
        case EAI_BADHINTS: return (STRPTR)"Invalid address-info hints";
        case EAI_PROTOCOL: return (STRPTR)"Protocol not supported";
        default: return (STRPTR)"Unknown address-info error";
    }
}

static char *u16_text(char *dst, UWORD v)
{
    char tmp[6]; int n = 0;
    do { tmp[n++] = (char)('0' + (v % 10)); v /= 10; } while (v && n < 5);
    while (n) *dst++ = tmp[--n];
    *dst = 0;
    return dst;
}
LONG bsd_getnameinfo(struct SocketBase *sb, struct sockaddr *sa, ULONG salen,
                     STRPTR host, ULONG hostlen, STRPTR serv, ULONG servlen, ULONG flags)
{
    struct sockaddr_in *sin;
    const ULONG allowed = NI_NUMERICHOST | NI_NUMERICSERV | NI_NOFQDN |
                          NI_NAMEREQD | NI_DGRAM | NI_WITHSCOPEID;

    if (!sa || salen < sizeof(struct sockaddr_in)) return EAI_FAIL;
    if (flags & ~allowed) return EAI_BADFLAGS;
    if (sa->sa_family != AF_INET) return EAI_FAMILY;
    sin = (struct sockaddr_in *)sa;

    if (host) {
        const char *name = NULL;
        char shortname[256];
        if (!(flags & NI_NUMERICHOST)) {
            struct hostent *h = bsd_gethostbyaddr(sb, (STRPTR)&sin->sin_addr.s_addr, 4, AF_INET);
            if (h) name = h->h_name;
            else if (flags & NI_NAMEREQD) return EAI_NONAME;
        }
        if (!name) name = bsd_Inet_NtoA(sb, sin->sin_addr.s_addr);
        if (flags & NI_NOFQDN) {
            ULONG i = 0;
            while (name[i] && name[i] != '.' && i < sizeof(shortname)-1) {
                shortname[i] = name[i]; ++i;
            }
            shortname[i] = 0; name = shortname;
        }
        if (!m_copy(host, hostlen, name)) return EAI_MEMORY;
    }

    if (serv) {
        const char *sname = NULL;
        char number[8];
        if (!(flags & NI_NUMERICSERV)) {
            struct servent *se = bsd_getservbyport(sb, (LONG)sin->sin_port,
                                 (flags & NI_DGRAM) ? (STRPTR)"udp" : (STRPTR)"tcp");
            if (se) sname = se->s_name;
        }
        if (!sname) { u16_text(number, sin->sin_port); sname = number; }
        if (!m_copy(serv, servlen, sname)) return EAI_MEMORY;
    }
    return 0;
}
