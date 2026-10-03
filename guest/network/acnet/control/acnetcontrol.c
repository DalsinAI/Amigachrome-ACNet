/*
 * ACNetControl - the ACNet ReAction Commodity for AmigaOS 3.2.3.
 *
 * First-light UI: native five-page control/status application.  Network
 * operation does not depend on this program; closing the window merely hides
 * the Commodity.
 *
 * BSD-3-Clause.
 */
#include <exec/types.h>
#include <exec/io.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <dos/dos.h>
#include <string.h>
#include <intuition/intuition.h>
#include <libraries/commodities.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>
#include <gadgets/button.h>
#include <gadgets/clicktab.h>
#include <gadgets/layout.h>
#include <images/label.h>
#include <classes/window.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/commodities.h>
#include <proto/button.h>
#include <proto/clicktab.h>
#include <proto/layout.h>
#include <proto/label.h>
#include <proto/window.h>
#include <clib/alib_protos.h>

#include "../include/acnet_device.h"

struct Library *CxBase;
struct Library *ButtonBase;
struct Library *ClickTabBase;
struct Library *LabelBase;
struct Library *LayoutBase;
struct Library *WindowBase;

enum {
    GID_TABS = 1,
    GID_GO_OFFLINE,
    GID_WIFI_RESCAN,
    GID_DIAG_DNS,
    GID_DIAG_CONNECT,
    GID_DIAG_INTERNET,
    GID_DIAG_COPY,
    GID_LOG_CLEAR,
    GID_LOG_SAVE
};

#define HOTKEY_ID 0xAC01

static struct MsgPort *cx_port;
static CxObj *broker;
static Object *win_obj;
static struct Window *window;
static struct List tabs;
static struct MsgPort *dev_port;
static struct IOStdReq *dev_req;
static ULONG acnet_state;

static UBYTE status_network[64];
static UBYTE status_card[96];
static UBYTE status_bottom[96];

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

static void read_acnet_state(void)
{
    acnet_state = 0;
    dev_port = CreateMsgPort();
    if (!dev_port) return;
    dev_req = (struct IOStdReq *)CreateIORequest(dev_port, sizeof(*dev_req));
    if (!dev_req) return;

    if (OpenDevice(ACNET_DEVICE_NAME, 0, (struct IORequest *)dev_req, 0) == 0) {
        acnet_state = acn_state_call(dev_req->io_Device);
    }
}

static void close_acnet_state(void)
{
    if (dev_req) {
        if (dev_req->io_Device) CloseDevice((struct IORequest *)dev_req);
        DeleteIORequest((struct IORequest *)dev_req);
        dev_req = NULL;
    }
    if (dev_port) {
        DeleteMsgPort(dev_port);
        dev_port = NULL;
    }
}

static void make_status_strings(void)
{
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

static BOOL make_tabs(void)
{
    static STRPTR names[] = {
        "Status", "Wi-Fi", "Connections", "Diagnostics", "Log", NULL
    };
    struct Node *node;
    LONG i;

    NewList(&tabs);
    for (i = 0; names[i]; ++i) {
        node = (struct Node *)AllocClickTabNode(
            TNA_Text, names[i],
            TNA_Number, i,
            TNA_Enabled, TRUE,
            TNA_Spacing, 6,
            TAG_DONE);
        if (!node) return FALSE;
        AddTail(&tabs, node);
    }
    return TRUE;
}

static void free_tabs(void)
{
    struct Node *node;
    while ((node = RemHead(&tabs)) != NULL) FreeClickTabNode(node);
}

#define LINE(txt)     LAYOUT_AddImage, LabelObject, LABEL_Text, (ULONG)(txt), LabelEnd,     CHILD_WeightedHeight, 0

static Object *status_page(void)
{
    return HGroupObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_SpaceInner, TRUE,

        LAYOUT_AddChild, VGroupObject,
            LAYOUT_BevelStyle, BVS_GROUP,
            LAYOUT_Label, "Network Status",
            LINE(status_network),
            LINE(status_card),
            LINE("Library       bsdsocket.library 4.x (ACNet)"),
            LINE("This Amiga    instance / acnet0"),
            LINE("IP Address    telemetry pending"),
            LINE("Subnet Mask   telemetry pending"),
            LINE("Gateway       telemetry pending"),
            LINE("DNS Servers   telemetry pending"),
            LINE("Uptime        telemetry pending"),
            LAYOUT_AddChild, ButtonObject,
                GA_ID, GID_GO_OFFLINE,
                GA_Text, "Go offline...",
                GA_RelVerify, TRUE,
                GA_Disabled, TRUE,
            ButtonEnd,
            CHILD_WeightedHeight, 0,
        LayoutEnd,

        LAYOUT_AddChild, VGroupObject,
            LAYOUT_BevelStyle, BVS_GROUP,
            LAYOUT_Label, "Traffic / Host (PC)",
            LINE("Download      telemetry pending"),
            LINE("Upload        telemetry pending"),
            LINE("Total In      telemetry pending"),
            LINE("Total Out     telemetry pending"),
            LINE("Open Sockets  telemetry pending"),
            LINE(""),
            LINE("PC Address    telemetry pending"),
            LINE("Interface     telemetry pending"),
            LINE("Gateway       telemetry pending"),
            LINE("Public Addr   Diagnostics only"),
        LayoutEnd,
    LayoutEnd;
}

static Object *wifi_page(void)
{
    return VGroupObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_BevelStyle, BVS_GROUP,
        LAYOUT_Label, "Wi-Fi",
        LINE("Host Wi-Fi control permission is reported by Cradle."),
        LINE("Version 1 joins only networks already known by the PC."),
        LINE("Passphrases never pass through the Amiga."),
        LINE(""),
        LINE("Current link  telemetry pending"),
        LINE("SSID          telemetry pending"),
        LINE("Signal        telemetry pending"),
        LINE("Security      telemetry pending"),
        LINE("Link rate     telemetry pending"),
        LAYOUT_AddChild, ButtonObject,
            GA_ID, GID_WIFI_RESCAN,
            GA_Text, "Rescan",
            GA_RelVerify, TRUE,
            GA_Disabled, TRUE,
        ButtonEnd,
        CHILD_WeightedHeight, 0,
    LayoutEnd;
}

static Object *connections_page(void)
{
    return VGroupObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_BevelStyle, BVS_GROUP,
        LAYOUT_Label, "Connections",
        LINE("Open sockets on this Amiga"),
        LINE("Program       Protocol   Local          Remote         State"),
        LINE("-------------------------------------------------------------"),
        LINE("Connection inventory pending ACNet private tags."),
        LINE("Version 1 is read-only."),
    LayoutEnd;
}

static Object *diagnostics_page(void)
{
    return VGroupObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_BevelStyle, BVS_GROUP,
        LAYOUT_Label, "Diagnostics",
        LINE("Diagnostics contact outside services only when requested."),
        LAYOUT_AddChild, ButtonObject,
            GA_ID, GID_DIAG_DNS,
            GA_Text, "Look up a name...",
            GA_RelVerify, TRUE,
        ButtonEnd,
        LAYOUT_AddChild, ButtonObject,
            GA_ID, GID_DIAG_CONNECT,
            GA_Text, "Test a connection...",
            GA_RelVerify, TRUE,
        ButtonEnd,
        LAYOUT_AddChild, ButtonObject,
            GA_ID, GID_DIAG_INTERNET,
            GA_Text, "Check the internet...",
            GA_RelVerify, TRUE,
        ButtonEnd,
        LAYOUT_AddChild, ButtonObject,
            GA_ID, GID_DIAG_COPY,
            GA_Text, "Copy report",
            GA_RelVerify, TRUE,
        ButtonEnd,
    LayoutEnd;
}

static Object *log_page(void)
{
    return VGroupObject,
        LAYOUT_SpaceOuter, TRUE,
        LAYOUT_BevelStyle, BVS_GROUP,
        LAYOUT_Label, "ACNet Event Log",
        LINE("Event ring support is the next ACNet device/HostSocket backend."),
        LINE("Online/offline, DNS failures and policy refusals will appear here."),
        LAYOUT_AddChild, HGroupObject,
            LAYOUT_AddChild, ButtonObject,
                GA_ID, GID_LOG_CLEAR,
                GA_Text, "Clear",
                GA_RelVerify, TRUE,
                GA_Disabled, TRUE,
            ButtonEnd,
            LAYOUT_AddChild, ButtonObject,
                GA_ID, GID_LOG_SAVE,
                GA_Text, "Save as...",
                GA_RelVerify, TRUE,
                GA_Disabled, TRUE,
            ButtonEnd,
        LayoutEnd,
        CHILD_WeightedHeight, 0,
    LayoutEnd;
}

static BOOL create_window_object(void)
{
    Object *pages = NULL;

    win_obj = WindowObject,
        WA_Title, "ACNetControl",
        WA_ScreenTitle, "ACNet - AmigaChrome networking",
        WA_Activate, TRUE,
        WA_DepthGadget, TRUE,
        WA_DragBar, TRUE,
        WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE,
        WA_SmartRefresh, TRUE,
        WINDOW_Position, WPOS_CENTERSCREEN,
        WINDOW_ParentGroup, VGroupObject,
            LAYOUT_SpaceOuter, TRUE,
            LAYOUT_DeferLayout, TRUE,

            LAYOUT_AddImage, LabelObject,
                LABEL_Text, "ACNetControl - Network Configuration for AmigaChrome",
            LabelEnd,
            CHILD_WeightedHeight, 0,

            LAYOUT_AddChild, ClickTabObject,
                GA_ID, GID_TABS,
                GA_RelVerify, TRUE,
                CLICKTAB_Labels, &tabs,
                CLICKTAB_Current, 0,
                CLICKTAB_PageGroup, pages = PageObject,
                    LAYOUT_DeferLayout, TRUE,
                    PAGE_Add, status_page(),
                    PAGE_Add, wifi_page(),
                    PAGE_Add, connections_page(),
                    PAGE_Add, diagnostics_page(),
                    PAGE_Add, log_page(),
                PageEnd,
            ClickTabEnd,

            LAYOUT_AddImage, LabelObject,
                LABEL_Text, status_bottom,
            LabelEnd,
            CHILD_WeightedHeight, 0,
        LayoutEnd,
    EndWindow;

    return win_obj != NULL;
}

static void hide_window(void)
{
    if (window && win_obj) {
        DoMethod(win_obj, WM_CLOSE);
        window = NULL;
    }
}

static void show_window(void)
{
    if (!window && win_obj) window = (struct Window *)RA_OpenWindow(win_obj);
    if (window) {
        WindowToFront(window);
        ActivateWindow(window);
    }
}

static BOOL open_bases(void)
{
    CxBase       = OpenLibrary("commodities.library", 37);
    WindowBase   = OpenLibrary("window.class", 0);
    LayoutBase   = OpenLibrary("gadgets/layout.gadget", 0);
    ClickTabBase = OpenLibrary("gadgets/clicktab.gadget", 0);
    LabelBase    = OpenLibrary("images/label.image", 0);
    ButtonBase   = OpenLibrary("gadgets/button.gadget", 0);
    return CxBase && WindowBase && LayoutBase && ClickTabBase && LabelBase && ButtonBase;
}

static void close_bases(void)
{
    if (ButtonBase)   CloseLibrary(ButtonBase);
    if (LabelBase)    CloseLibrary(LabelBase);
    if (ClickTabBase) CloseLibrary(ClickTabBase);
    if (LayoutBase)   CloseLibrary(LayoutBase);
    if (WindowBase)   CloseLibrary(WindowBase);
    if (CxBase)       CloseLibrary(CxBase);
}

static BOOL create_broker(void)
{
    struct NewBroker nb;
    CxObj *hotkey;
    LONG err = 0;

    cx_port = CreateMsgPort();
    if (!cx_port) return FALSE;

    nb.nb_Version = NB_VERSION;
    nb.nb_Name = "ACNetControl";
    nb.nb_Title = "ACNet Network Control";
    nb.nb_Descr = "Status and controls for AmigaChrome ACNet";
    nb.nb_Unique = NBU_UNIQUE | NBU_NOTIFY;
    nb.nb_Flags = COF_SHOW_HIDE;
    nb.nb_Pri = 0;
    nb.nb_Port = cx_port;
    nb.nb_ReservedChannel = 0;

    broker = CxBroker(&nb, &err);
    if (!broker) return FALSE;

    hotkey = HotKey("ctrl alt n", cx_port, HOTKEY_ID);
    if (hotkey) AttachCxObj(broker, hotkey);
    ActivateCxObj(broker, 1);
    return TRUE;
}

static void destroy_broker(void)
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
}

static BOOL handle_cx_messages(void)
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
                    show_window();
                    break;
                case CXCMD_DISAPPEAR:
                    hide_window();
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
            show_window();
        }
        ReplyMsg((struct Message *)msg);
    }
    return running;
}

int main(void)
{
    ULONG winsig = 0, sigs;
    BOOL running = TRUE;
    UWORD code = 0;

    if (!open_bases()) {
        PutStr("ACNetControl: required ReAction/Commodity classes are unavailable.\n");
        close_bases();
        return 20;
    }

    read_acnet_state();
    make_status_strings();

    if (!make_tabs() || !create_window_object() || !create_broker()) {
        PutStr("ACNetControl: could not initialise.\n");
        destroy_broker();
        if (win_obj) DisposeObject(win_obj);
        free_tabs();
        close_acnet_state();
        close_bases();
        return 20;
    }

    show_window();

    while (running) {
        winsig = 0;
        if (window) GetAttr(WINDOW_SigMask, win_obj, &winsig);
        sigs = Wait(winsig | (1UL << cx_port->mp_SigBit) | SIGBREAKF_CTRL_C);

        if (sigs & SIGBREAKF_CTRL_C) running = FALSE;
        if (sigs & (1UL << cx_port->mp_SigBit)) running = handle_cx_messages();

        if (running && window && (sigs & winsig)) {
            ULONG result;
            while ((result = RA_HandleInput(win_obj, &code)) != WMHI_LASTMSG) {
                switch (result & WMHI_CLASSMASK) {
                    case WMHI_CLOSEWINDOW:
                        hide_window();
                        break;
                    case WMHI_GADGETUP:
                        /* The first-light UI has real controls only where
                         * their backends already exist. */
                        break;
                }
            }
        }
    }

    hide_window();
    destroy_broker();
    if (win_obj) DisposeObject(win_obj);
    free_tabs();
    close_acnet_state();
    close_bases();
    return 0;
}
