/*
 * ACNetControl - the ACNet ReAction Commodity for AmigaOS 3.2.3.
 *
 * First-light UI: native five-page control/status application.  Network
 * operation does not depend on this program; closing the window merely hides
 * the Commodity. The pages, the broker and the card's state come from
 * acnetcontrol_core.c, shared with the GadTools front end.
 *
 * BSD-3-Clause.
 */
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
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
#include <proto/button.h>
#include <proto/clicktab.h>
#include <proto/layout.h>
#include <proto/label.h>
#include <proto/window.h>
#include <clib/alib_protos.h>

#include "acnetcontrol_core.h"

struct Library *ButtonBase;
struct Library *ClickTabBase;
struct Library *LabelBase;
struct Library *LayoutBase;
struct Library *WindowBase;

static Object *win_obj;
static struct Window *window;
static struct List tabs;

static BOOL make_tabs(void)
{
    struct Node *node;
    LONG i;

    NewList(&tabs);
    for (i = 0; i < ACNC_PAGES; ++i) {
        node = (struct Node *)AllocClickTabNode(
            TNA_Text, acnc_page[i].name,
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

static Object *button(const ACNCButton *b)
{
    return NewObject(BUTTON_GetClass(), NULL, GA_ID, b->id, GA_Text, (ULONG)b->label,
                     GA_RelVerify, TRUE, GA_Disabled, b->disabled, TAG_DONE);
}

/* One bevelled group: its lines as labels, then its buttons. */
static Object *group_object(const ACNCGroup *g, BOOL outer_space)
{
    Object *grp = NewObject(LAYOUT_GetClass(), NULL, LAYOUT_Orientation, LAYOUT_ORIENT_VERT,
                            LAYOUT_SpaceOuter, outer_space, LAYOUT_BevelStyle, BVS_GROUP,
                            LAYOUT_Label, (ULONG)g->title, TAG_DONE);
    Object *row = NULL;
    int i;

    if (!grp) return NULL;
    for (i = 0; i < ACNC_MAX_LINES && g->line[i]; ++i)
        SetAttrs(grp, LAYOUT_AddImage, (ULONG)NewObject(LABEL_GetClass(), NULL, LABEL_Text, (ULONG)g->line[i], TAG_DONE),
                 CHILD_WeightedHeight, 0, TAG_DONE);
    if (g->buttons_in_a_row && g->button[0].id)
        row = NewObject(LAYOUT_GetClass(), NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ, TAG_DONE);
    for (i = 0; i < ACNC_MAX_BUTTONS && g->button[i].id; ++i) {
        if (row)
            SetAttrs(row, LAYOUT_AddChild, (ULONG)button(&g->button[i]), TAG_DONE);
        else
            SetAttrs(grp, LAYOUT_AddChild, (ULONG)button(&g->button[i]), CHILD_WeightedHeight, 0, TAG_DONE);
    }
    if (row)
        SetAttrs(grp, LAYOUT_AddChild, (ULONG)row, CHILD_WeightedHeight, 0, TAG_DONE);
    return grp;
}

static Object *page_object(const ACNCPage *p)
{
    Object *page;
    if (p->groups == 1)
        return group_object(&p->group[0], TRUE);
    page = NewObject(LAYOUT_GetClass(), NULL, LAYOUT_Orientation, LAYOUT_ORIENT_HORIZ,
                     LAYOUT_SpaceOuter, TRUE, LAYOUT_SpaceInner, TRUE, TAG_DONE);
    if (page)
        SetAttrs(page, LAYOUT_AddChild, (ULONG)group_object(&p->group[0], FALSE),
                 LAYOUT_AddChild, (ULONG)group_object(&p->group[1], FALSE), TAG_DONE);
    return page;
}

static BOOL create_window_object(void)
{
    Object *pages = PageObject, LAYOUT_DeferLayout, TRUE, PageEnd;
    int i;

    if (!pages) return FALSE;
    for (i = 0; i < ACNC_PAGES; ++i)
        SetAttrs(pages, PAGE_Add, (ULONG)page_object(&acnc_page[i]), TAG_DONE);

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
                LABEL_Text, ACNC_TITLE,
            LabelEnd,
            CHILD_WeightedHeight, 0,

            LAYOUT_AddChild, ClickTabObject,
                GA_ID, GID_TABS,
                GA_RelVerify, TRUE,
                CLICKTAB_Labels, &tabs,
                CLICKTAB_Current, 0,
                CLICKTAB_PageGroup, pages,
            ClickTabEnd,

            LAYOUT_AddImage, LabelObject,
                LABEL_Text, acnc_status_line(),
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
    WindowBase   = OpenLibrary("window.class", 0);
    LayoutBase   = OpenLibrary("gadgets/layout.gadget", 0);
    ClickTabBase = OpenLibrary("gadgets/clicktab.gadget", 0);
    LabelBase    = OpenLibrary("images/label.image", 0);
    ButtonBase   = OpenLibrary("gadgets/button.gadget", 0);
    return WindowBase && LayoutBase && ClickTabBase && LabelBase && ButtonBase;
}

static void close_bases(void)
{
    if (ButtonBase)   CloseLibrary(ButtonBase);
    if (LabelBase)    CloseLibrary(LabelBase);
    if (ClickTabBase) CloseLibrary(ClickTabBase);
    if (LayoutBase)   CloseLibrary(LayoutBase);
    if (WindowBase)   CloseLibrary(WindowBase);
}

static int control_main(void)
{
    ULONG winsig = 0, sigs;
    BOOL running = TRUE;
    UWORD code = 0;
    int i;

    if (!open_bases()) {
        PutStr("ACNetControl: required ReAction classes are unavailable; ACNetControlGT runs without them.\n");
        close_bases();
        return 20;
    }

    NewList(&tabs);                         /* free_tabs() is safe on every path */
    acnc_read_state();

    i = acnc_broker_open();
    if (i == ACNC_ALREADY_RUNNING) {        /* that copy has been told to show itself */
        acnc_broker_close();
        close_bases();
        return 5;
    }
    if (i != 1 || !make_tabs() || !create_window_object()) {
        PutStr("ACNetControl: could not initialise.\n");
        acnc_broker_close();
        if (win_obj) DisposeObject(win_obj);
        free_tabs();
        close_bases();
        return 20;
    }

    show_window();

    while (running) {
        winsig = 0;
        if (window) GetAttr(WINDOW_SigMask, win_obj, &winsig);
        sigs = Wait(winsig | acnc_broker_signal() | SIGBREAKF_CTRL_C);

        if (sigs & SIGBREAKF_CTRL_C) running = FALSE;
        if (sigs & acnc_broker_signal()) running = acnc_broker_handle(show_window, hide_window);

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
                if (!window) break;
            }
        }
    }

    hide_window();
    acnc_broker_close();
    if (win_obj) DisposeObject(win_obj);
    free_tabs();
    close_bases();
    return 0;
}

int main(void)
{
    return acnc_main_with_stack(control_main, 32768);
}
