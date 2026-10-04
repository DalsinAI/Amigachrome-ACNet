/* ACNet SANA-II live probe for AmigaOS 3.2.3. BSD-3-Clause. */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <proto/exec.h>
#include <devices/sana2.h>
#include <utility/tagitem.h>
#include <stdio.h>
#include <string.h>

#include "../include/acnet_device.h"

#define REG(r, decl) register decl __asm(#r)

static BOOL copy_to(REG(a0, APTR to), REG(a1, APTR from), REG(d0, ULONG n))
{
    CopyMem(from, to, n);
    return TRUE;
}

static BOOL copy_from(REG(a0, APTR to), REG(a1, APTR from), REG(d0, ULONG n))
{
    CopyMem(from, to, n);
    return TRUE;
}

static int failures;
static void check(int ok, const char *name)
{
    printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
    if (!ok) failures++;
}
int main(void)
{
    struct MsgPort *port;
    struct IOSana2Req *io, *readreq;
    struct TagItem tags[3];
    struct Sana2DeviceQuery query;
    UBYTE rx[1500], frame[64];
    LONG err;

    port = CreateMsgPort();
    if (!port) return 20;
    io = (struct IOSana2Req *)CreateIORequest(port, sizeof(*io));
    readreq = (struct IOSana2Req *)CreateIORequest(port, sizeof(*readreq));
    if (!io || !readreq) return 20;

    tags[0].ti_Tag = S2_CopyToBuff; tags[0].ti_Data = (ULONG)copy_to;
    tags[1].ti_Tag = S2_CopyFromBuff; tags[1].ti_Data = (ULONG)copy_from;
    tags[2].ti_Tag = TAG_DONE; tags[2].ti_Data = 0;
    io->ios2_BufferManagement = tags;

    err = OPENSOCKET_OPEN_DEVICE(io);
    check(err == 0, "OpenDevice");
    if (err) return 20;

    memset(&query, 0, sizeof(query));
    io->ios2_Req.io_Command = S2_DEVICEQUERY;
    io->ios2_StatData = &query;
    check(DoIO((struct IORequest *)io) == 0 &&
          query.HardwareType == S2WireType_Ethernet &&
          query.MTU == 1500 && query.RawMTU == 1514,
          "S2_DEVICEQUERY");

    io->ios2_Req.io_Command = S2_GETSTATIONADDRESS;
    check(DoIO((struct IORequest *)io) == 0 &&
          io->ios2_SrcAddr[0] == 0x02 && io->ios2_SrcAddr[1] == 0xda,
          "S2_GETSTATIONADDRESS");
    io->ios2_Req.io_Command = S2_ONLINE;
    err = DoIO((struct IORequest *)io);
    check(err == 0 || io->ios2_WireError == S2WERR_UNIT_ONLINE, "S2_ONLINE");

    io->ios2_Req.io_Command = S2_TRACKTYPE;
    io->ios2_PacketType = 0x0806;
    err = DoIO((struct IORequest *)io);
    check(err == 0 || io->ios2_WireError == S2WERR_ALREADY_TRACKED,
          "S2_TRACKTYPE ARP");

    CopyMem(io, readreq, sizeof(*readreq));
    readreq->ios2_Req.io_Message.mn_ReplyPort = port;
    readreq->ios2_Req.io_Message.mn_Length = sizeof(*readreq);
    readreq->ios2_Req.io_Command = CMD_READ;
    readreq->ios2_Req.io_Flags = 0;
    readreq->ios2_PacketType = 0x0806;
    readreq->ios2_Data = rx;
    readreq->ios2_DataLength = sizeof(rx);
    readreq->ios2_BufferManagement = tags;
    SendIO((struct IORequest *)readreq);

    memset(frame, 0, sizeof(frame));
    memset(frame, 0xff, 6);
    frame[6]=0x02; frame[7]=0xda; frame[8]=0x15; frame[9]=0; frame[10]=0; frame[11]=0x06;
    frame[12]=0x08; frame[13]=0x06;
    frame[14]=0; frame[15]=1; frame[16]=0x08; frame[17]=0;
    frame[18]=6; frame[19]=4; frame[20]=0; frame[21]=1;
    CopyMem(frame+6, frame+22, 6);
    frame[28]=10; frame[29]=0; frame[30]=2; frame[31]=15;
    frame[38]=10; frame[39]=0; frame[40]=2; frame[41]=2;

    io->ios2_Req.io_Command = CMD_WRITE;
    io->ios2_Req.io_Flags = SANA2IOF_RAW;
    io->ios2_Data = frame;
    io->ios2_DataLength = 64;
    io->ios2_PacketType = 0x0806;
    check(DoIO((struct IORequest *)io) == 0, "CMD_WRITE raw ARP");
    WaitIO((struct IORequest *)readreq);
    check(readreq->ios2_Req.io_Error == 0 &&
          readreq->ios2_PacketType == 0x0806 &&
          readreq->ios2_DataLength >= 28 &&
          rx[6] == 0 && rx[7] == 2,
          "CMD_READ async ARP reply");

    io->ios2_Req.io_Command = S2_OFFLINE;
    err = DoIO((struct IORequest *)io);
    check(err == 0 || io->ios2_WireError == S2WERR_UNIT_OFFLINE, "S2_OFFLINE");

    CloseDevice((struct IORequest *)io);
    DeleteIORequest((struct IORequest *)readreq);
    DeleteIORequest((struct IORequest *)io);
    DeleteMsgPort(port);

    printf("RESULT: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 5 : 0;
}
