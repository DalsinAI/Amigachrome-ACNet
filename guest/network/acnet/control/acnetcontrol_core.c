/*
 * ACNetControl's shared core (acnetcontrol_core.h).
 *
 * BSD-3-Clause.
 */
#include <exec/types.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <dos/dos.h>
#include <libraries/commodities.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/commodities.h>
#include <clib/alib_protos.h>

#include "../include/acnet_device.h"
#include "acnetcontrol_core.h"

struct Library *CxBase;

#define HOTKEY_ID 0xAC01

static ULONG acnet_state;
static char status_network[64] = "Network       No ACNet card";
static char status_card[96] = "Card          Not present";
static char status_bottom[96] = "No ACNet card - enable Network in Cradle and reboot";

/* What each page says. Lines are aligned for the Workbench's fixed-width
 * font, as the first-light design was. */
const ACNCPage acnc_page[ACNC_PAGES] = {
    { "Status", 2, {
        { "Network Status",
          { status_network, status_card,
            "Library       bsdsocket.library 4.x (ACNet)",
            "This Amiga    instance / acnet0",
            "IP Address    telemetry pending",
            "Subnet Mask   telemetry pending",
            "Gateway       telemetry pending",
            "DNS Servers   telemetry pending",
            "Uptime        telemetry pending", NULL },
          { { GID_GO_OFFLINE, "_Go offline...", TRUE }, { 0, NULL, FALSE } }, FALSE },
        { "Traffic / Host (PC)",
          { "Download      telemetry pending",
            "Upload        telemetry pending",
            "Total In      telemetry pending",
            "Total Out     telemetry pending",
            "Open Sockets  telemetry pending",
            "",
            "PC Address    telemetry pending",
            "Interface     telemetry pending",
            "Gateway       telemetry pending",
            "Public Addr   Diagnostics only", NULL },
          { { 0, NULL, FALSE } }, FALSE } } },
    { "Wi-Fi", 1, {
        { "Wi-Fi",
          { "Host Wi-Fi control permission is reported by Cradle.",
            "Version 1 joins only networks already known by the PC.",
            "Passphrases never pass through the Amiga.",
            "",
            "Current link  telemetry pending",
            "SSID          telemetry pending",
            "Signal        telemetry pending",
            "Security      telemetry pending",
            "Link rate     telemetry pending", NULL },
          { { GID_WIFI_RESCAN, "_Rescan", TRUE }, { 0, NULL, FALSE } }, FALSE } } },
    { "Connections", 1, {
        { "Connections",
          { "Open sockets on this Amiga",
            "Program       Protocol   Local          Remote         State",
            "-------------------------------------------------------------",
            "Connection inventory pending ACNet private tags.",
            "Version 1 is read-only.", NULL },
          { { 0, NULL, FALSE } }, FALSE } } },
    { "Diagnostics", 1, {
        { "Diagnostics",
          { "Diagnostics contact outside services only when requested.", NULL },
          { { GID_DIAG_DNS, "Look _up a name...", FALSE },
            { GID_DIAG_CONNECT, "_Test a connection...", FALSE },
            { GID_DIAG_INTERNET, "Check the _internet...", FALSE },
            { GID_DIAG_COPY, "Cop_y report", FALSE } }, FALSE } } },
    { "Log", 1, {
        { "ACNet Event Log",
          { "Event ring support is the next ACNet device/HostSocket backend.",
            "Online/offline, DNS failures and policy refusals will appear here.", NULL },
          { { GID_LOG_CLEAR, "_Clear", TRUE }, { GID_LOG_SAVE, "_Save as...", TRUE }, { 0, NULL, FALSE } }, TRUE } } },
};

static ULONG acn_state_call(struct Device *dev)
{
    register struct Device *a6 __asm("a6") = dev;
    register ULONG d0 __asm("d0");
    __asm volatile ("jsr -60(a6)"
                    : "=r"(d0)
                    : "r"(a6)
                    : "d1", "a0", "a1", "cc", "memory");
    return d0;
}

void acnc_read_state(void)
{
    struct MsgPort *port = CreateMsgPort();
    struct IOStdReq *req = port ? (struct IOStdReq *)CreateIORequest(port, sizeof(*req)) : NULL;

    acnet_state = 0;
    if (req && OpenDevice(ACNET_DEVICE_NAME, 0, (struct IORequest *)req, 0) == 0) {
        acnet_state = acn_state_call(req->io_Device);
        CloseDevice((struct IORequest *)req);
    }
    if (req) DeleteIORequest((struct IORequest *)req);
    if (port) DeleteMsgPort(port);

    if (!(acnet_state & ACN_STATE_CARD)) {
        strcpy(status_network, "Network       No ACNet card");
        strcpy(status_card,    "Card          Not present");
        strcpy(status_bottom,  "No ACNet card - enable Network in Cradle and reboot");
    } else if (!(acnet_state & ACN_STATE_ONLINE)) {
        strcpy(status_network, "Network       Off in Cradle");
        strcpy(status_card,    "Card          ACNet - Dalsin product 6 - HostSocket");
        strcpy(status_bottom,  "Network is disabled in Cradle");
    } else {
        strcpy(status_network, "Network       Online");
        strcpy(status_card,    "Card          ACNet - Dalsin product 6 - HostSocket");
        strcpy(status_bottom,  "Online - ACNet card present");
    }
}

const char *acnc_status_line(void)
{
    return status_bottom;
}

static struct MsgPort *cx_port;
static CxObj *broker;

int acnc_broker_open(void)
{
    struct NewBroker nb;
    CxObj *hotkey;
    LONG err = 0;

    CxBase = OpenLibrary("commodities.library", 37);
    if (!CxBase) return 0;
    cx_port = CreateMsgPort();
    if (!cx_port) return 0;

    memset(&nb, 0, sizeof(nb));
    nb.nb_Version = NB_VERSION;
    nb.nb_Name = "ACNetControl";            /* both front ends: only one runs */
    nb.nb_Title = "ACNet Network Control";
    nb.nb_Descr = "Status and controls for AmigaChrome ACNet";
    nb.nb_Unique = NBU_UNIQUE | NBU_NOTIFY;
    nb.nb_Flags = COF_SHOW_HIDE;
    nb.nb_Pri = 0;
    nb.nb_Port = cx_port;

    broker = CxBroker(&nb, &err);
    if (!broker) return err == CBERR_DUP ? ACNC_ALREADY_RUNNING : 0;

    hotkey = HotKey(ACNC_HOTKEY, cx_port, HOTKEY_ID);
    if (hotkey) AttachCxObj(broker, hotkey);
    ActivateCxObj(broker, 1);
    return 1;
}

void acnc_broker_close(void)
{
    if (broker) {
        DeleteCxObjAll(broker);
        broker = NULL;
    }
    if (cx_port) {
        struct Message *m;
        while ((m = GetMsg(cx_port)) != NULL) ReplyMsg(m);
        DeleteMsgPort(cx_port);
        cx_port = NULL;
    }
    if (CxBase) {
        CloseLibrary(CxBase);
        CxBase = NULL;
    }
}

ULONG acnc_broker_signal(void)
{
    return broker ? 1UL << cx_port->mp_SigBit : 0;
}

BOOL acnc_broker_handle(void (*show)(void), void (*hide)(void))
{
    CxMsg *msg;
    BOOL running = TRUE;

    while ((msg = (CxMsg *)GetMsg(cx_port)) != NULL) {
        ULONG type = CxMsgType(msg);
        ULONG id = CxMsgID(msg);

        if (type == CXM_COMMAND) {
            switch (id) {
                case CXCMD_APPEAR:
                case CXCMD_UNIQUE:
                    show();
                    break;
                case CXCMD_DISAPPEAR:
                    hide();
                    break;
                case CXCMD_ENABLE:
                    ActivateCxObj(broker, 1);
                    break;
                case CXCMD_DISABLE:
                    ActivateCxObj(broker, 0);
                    break;
                case CXCMD_KILL:
                    running = FALSE;
                    break;
            }
        } else if (id == HOTKEY_ID) {
            show();
        }
        ReplyMsg((struct Message *)msg);
    }
    return running;
}

/* Static, not on the stack: nothing may be read from the old stack's frame
 * between the two StackSwap calls. */
static struct StackSwapStruct swap;
static int (*volatile swap_body)(void);
static volatile int swap_rc;

int acnc_main_with_stack(int (*body)(void), unsigned long bytes)
{
    struct Task *me = FindTask(NULL);
    unsigned long have = (unsigned long)me->tc_SPUpper - (unsigned long)me->tc_SPLower;
    APTR lower;

    if (have >= bytes || !(lower = AllocVec(bytes, MEMF_ANY)))
        return body();
    swap.stk_Lower = lower;
    swap.stk_Upper = (ULONG)lower + bytes;
    swap.stk_Pointer = (APTR)swap.stk_Upper;
    swap_body = body;
    StackSwap(&swap);
    swap_rc = swap_body();
    StackSwap(&swap);
    FreeVec(lower);
    return swap_rc;
}
