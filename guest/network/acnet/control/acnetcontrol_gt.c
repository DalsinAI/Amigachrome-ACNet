/*
 * ACNetControlGT - the ACNet Commodity in GadTools, for AmigaOS 2.04 to 3.2.
 *
 * The same five pages as the ReAction ACNetControl (acnetcontrol_core.c),
 * for machines without ReAction. GadTools has no tabs, so the pages are a
 * radio list down the left, as the classic Prefs editors do it; each page
 * is drawn as ridged groups with its lines and buttons. The window follows
 * the screen's font (Topaz 8 when that would not fit the screen) and can
 * be resized. Closing it hides the Commodity; networking never depends on
 * it.
 *
 * Keys: Tab and Shift-Tab change page, 1 to 5 pick one, Esc hides, and
 * the underlined letters press buttons.
 *
 * BSD-3-Clause.
 */
#include <exec/types.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <libraries/gadtools.h>
#include <devices/inputevent.h>
#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>

#include "acnetcontrol_core.h"

struct Library *GadToolsBase;

#define VERSION_TEXT "OpenSocketControl 1.0 (4.10.2026)"
static const char version[] __attribute__((used)) = "$VER: " VERSION_TEXT;

enum { GID_PAGES = GID_FIRST_FREE, GID_STATUS };
enum { M_ABOUT = 1, M_HIDE, M_QUIT };

#define PAD     6       /* between groups, and between buttons */
#define INSET   10      /* from a group's ridge to its text */
#define MARGIN  8       /* inside the window's borders */

static struct NewMenu menus[] = {
    { NM_TITLE, "Project", NULL, 0, 0, NULL },
    { NM_ITEM, "About...", NULL, 0, 0, (APTR)M_ABOUT },
    { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL },
    { NM_ITEM, "Hide", "H", 0, 0, (APTR)M_HIDE },
    { NM_ITEM, "Quit", "Q", 0, 0, (APTR)M_QUIT },
    { NM_END, NULL, NULL, 0, 0, NULL }
};

static STRPTR page_names[ACNC_PAGES + 1];

static struct Screen *scr;
static APTR vi;
static struct Window *win;
static struct Menu *menu;
static struct Gadget *glist;
static struct TextFont *font;
static struct TextAttr font_attr;
static struct RastPort measure;
static int page;
static int fh, line_h, btn_h, mx_w, title_band;
static int nat_w, nat_h;            /* the page area every page fits in */
static BOOL quit_now;

static struct TextAttr topaz8 = { "topaz.font", 8, FS_NORMAL, FPF_ROMFONT };

/* ---- measuring ---------------------------------------------------------- */

static int text_w(const char *s)
{
    char plain[96];
    int n = 0;
    for (; *s && n < (int)sizeof(plain) - 1; ++s)
        if (*s != '_') plain[n++] = *s;          /* the shortcut mark takes no room */
    return TextLength(&measure, plain, n);
}

static int button_w(const ACNCButton *b)
{
    return text_w(b->label) + 2 * 8;
}

static void group_natural(const ACNCGroup *g, int *w, int *h)
{
    int i, gw = text_w(g->title) + 4 * PAD, gh = title_band + PAD, row = 0;
    /* widths below are the group's: its text plus INSET each side */
    for (i = 0; i < ACNC_MAX_LINES && g->line[i]; ++i) {
        int tw = text_w(g->line[i]);
        if (tw > gw - 2 * INSET) gw = tw + 2 * INSET;
        gh += line_h;
    }
    for (i = 0; i < ACNC_MAX_BUTTONS && g->button[i].id; ++i) {
        if (g->buttons_in_a_row) {
            row += button_w(&g->button[i]) + (i ? PAD : 0);
            if (!i) gh += PAD + btn_h;
        } else {
            if (button_w(&g->button[i]) > gw - 2 * INSET) gw = button_w(&g->button[i]) + 2 * INSET;
            gh += PAD + btn_h;
        }
    }
    if (row > gw - 2 * INSET) gw = row + 2 * INSET;
    *w = gw;
    *h = gh + PAD;
}

static void measure_pages(void)
{
    int p, i;
    fh = font->tf_YSize;
    line_h = fh + 2;
    btn_h = fh + 6;
    title_band = fh;
    mx_w = 0;
    for (p = 0; p < ACNC_PAGES; ++p) {
        int w0, h0, w1 = 0, h1 = 0, w, h;
        int lw = text_w(acnc_page[p].name);
        if (lw > mx_w) mx_w = lw;
        group_natural(&acnc_page[p].group[0], &w0, &h0);
        if (acnc_page[p].groups == 2) group_natural(&acnc_page[p].group[1], &w1, &h1);
        w = w0 + (w1 ? PAD + w1 : 0);
        h = h0 > h1 ? h0 : h1;
        if (w > nat_w) nat_w = w;
        if (h > nat_h) nat_h = h;
    }
    mx_w += 17 + 6;                  /* MXWIDTH and the gap before its label */
    i = (fh > 9 ? fh : 9) + 4;       /* one radio row */
    if (ACNC_PAGES * i > nat_h) nat_h = ACNC_PAGES * i;
}

/* The screen's font, or Topaz 8 when the window would not fit the screen. */
static BOOL choose_font(void)
{
    int pass;
    for (pass = 0; pass < 2; ++pass) {
        font_attr = pass ? topaz8 : *scr->Font;
        font = OpenFont(&font_attr);
        if (!font) continue;
        InitRastPort(&measure);
        SetFont(&measure, font);
        nat_w = nat_h = 0;
        measure_pages();
        if (pass || (MARGIN * 3 + mx_w + nat_w + scr->WBorLeft + scr->WBorRight + 18 <= scr->Width &&
                     MARGIN * 4 + line_h * 2 + nat_h + btn_h + scr->WBorTop + scr->Font->ta_YSize + 12 <= scr->Height))
            return TRUE;
        CloseFont(font);
        font = NULL;
    }
    return font != NULL;
}

/* ---- the window's insides ---------------------------------------------- */

struct Zone { int x, y, w, h; };

static void inner(struct Zone *a)
{
    a->x = win->BorderLeft + MARGIN;
    a->y = win->BorderTop + MARGIN;
    a->w = win->Width - win->BorderLeft - win->BorderRight - 2 * MARGIN;
    a->h = win->Height - win->BorderTop - win->BorderBottom - 2 * MARGIN;
}

/* Where the page's groups sit: one fills the page area, two share it in
 * proportion to what they need. */
static void group_areas(const struct Zone *pa, struct Zone g[2])
{
    const ACNCPage *p = &acnc_page[page];
    g[0] = *pa;
    if (p->groups == 2) {
        int w0, h0, w1, h1, spare;
        group_natural(&p->group[0], &w0, &h0);
        group_natural(&p->group[1], &w1, &h1);
        spare = pa->w - PAD - w0 - w1;
        if (spare < 0) spare = 0;
        g[0].w = w0 + spare / 2;
        g[1] = *pa;
        g[1].x = pa->x + g[0].w + PAD;
        g[1].w = pa->w - g[0].w - PAD;
    }
}

static void page_area(struct Zone *pa)
{
    struct Zone a;
    inner(&a);
    pa->x = a.x + mx_w + MARGIN;
    pa->y = a.y + line_h + PAD;
    pa->w = a.w - mx_w - MARGIN;
    pa->h = a.h - line_h - PAD - (line_h + 4) - PAD;
}

static struct Gadget *make_gadgets(void)
{
    struct NewGadget ng;
    struct Gadget *g;
    struct Zone a, pa, ga[2];
    int i, k;

    glist = NULL;
    g = CreateContext(&glist);
    if (!g) return NULL;
    inner(&a);
    page_area(&pa);
    memset(&ng, 0, sizeof(ng));
    ng.ng_TextAttr = &font_attr;
    ng.ng_VisualInfo = vi;

    ng.ng_LeftEdge = a.x;
    ng.ng_TopEdge = pa.y + 2;
    ng.ng_Width = 17;
    ng.ng_Height = 9;
    ng.ng_GadgetID = GID_PAGES;
    ng.ng_Flags = PLACETEXT_RIGHT;
    g = CreateGadget(MX_KIND, g, &ng, GTMX_Labels, (ULONG)page_names, GTMX_Active, page,
                     GTMX_Spacing, (fh > 9 ? fh - 9 : 0) + 4, TAG_DONE);

    group_areas(&pa, ga);
    for (k = 0; k < acnc_page[page].groups; ++k) {
        const ACNCGroup *grp = &acnc_page[page].group[k];
        int lines = 0, x = ga[k].x + INSET, y;
        while (lines < ACNC_MAX_LINES && grp->line[lines]) ++lines;
        y = ga[k].y + title_band + PAD + lines * line_h;
        for (i = 0; i < ACNC_MAX_BUTTONS && grp->button[i].id; ++i) {
            ng.ng_LeftEdge = x;
            ng.ng_TopEdge = y + PAD;
            ng.ng_Width = grp->buttons_in_a_row ? button_w(&grp->button[i]) : ga[k].w - 2 * INSET;
            ng.ng_Height = btn_h;
            ng.ng_GadgetText = (UBYTE *)grp->button[i].label;
            ng.ng_GadgetID = grp->button[i].id;
            ng.ng_Flags = PLACETEXT_IN;
            g = CreateGadget(BUTTON_KIND, g, &ng, GT_Underscore, '_', GA_Disabled, grp->button[i].disabled, TAG_DONE);
            if (grp->buttons_in_a_row) x += ng.ng_Width + PAD;
            else y += PAD + btn_h;
        }
    }

    ng.ng_LeftEdge = a.x;
    ng.ng_TopEdge = a.y + a.h - (line_h + 4);
    ng.ng_Width = a.w;
    ng.ng_Height = line_h + 4;
    ng.ng_GadgetText = NULL;
    ng.ng_GadgetID = GID_STATUS;
    ng.ng_Flags = 0;
    g = CreateGadget(TEXT_KIND, g, &ng, GTTX_Text, (ULONG)acnc_status_line(), GTTX_Border, TRUE,
                     GTTX_CopyText, TRUE, TAG_DONE);
    return g;
}

static void ridge(struct RastPort *rp, const struct Zone *r)
{
    if (GadToolsBase->lib_Version >= 39) {
        DrawBevelBox(rp, r->x, r->y, r->w, r->h, GT_VisualInfo, (ULONG)vi, GTBB_FrameType, BBFT_RIDGE, TAG_DONE);
    } else {                        /* 2.04: a ridge from two boxes */
        DrawBevelBox(rp, r->x, r->y, r->w, r->h, GT_VisualInfo, (ULONG)vi, GTBB_Recessed, TRUE, TAG_DONE);
        DrawBevelBox(rp, r->x + 1, r->y + 1, r->w - 2, r->h - 2, GT_VisualInfo, (ULONG)vi, TAG_DONE);
    }
}

static void say(struct RastPort *rp, int x, int y, const char *s, UWORD pen)
{
    SetAPen(rp, pen);
    Move(rp, x, y + font->tf_Baseline);
    Text(rp, (STRPTR)s, strlen(s));
}

/* What is not a gadget: the heading and the page's groups. */
static void draw_static(void)
{
    struct RastPort *rp = win->RPort;
    struct DrawInfo *dri = GetScreenDrawInfo(scr);
    UWORD text = dri ? dri->dri_Pens[TEXTPEN] : 1, back = dri ? dri->dri_Pens[BACKGROUNDPEN] : 0;
    UWORD hi = dri ? dri->dri_Pens[HIGHLIGHTTEXTPEN] : 2;
    struct Zone a, pa, ga[2];
    int k, i;

    inner(&a);
    page_area(&pa);
    group_areas(&pa, ga);
    SetFont(rp, font);
    SetDrMd(rp, JAM1);
    say(rp, a.x, a.y, ACNC_TITLE, hi);

    for (k = 0; k < acnc_page[page].groups; ++k) {
        const ACNCGroup *grp = &acnc_page[page].group[k];
        struct Zone frame = ga[k];
        int tw = text_w(grp->title);
        frame.y += title_band / 2;
        frame.h -= title_band / 2;
        ridge(rp, &frame);
        SetAPen(rp, back);                       /* the title sits in a gap in the ridge */
        RectFill(rp, ga[k].x + (ga[k].w - tw) / 2 - 4, ga[k].y, ga[k].x + (ga[k].w + tw) / 2 + 3, ga[k].y + fh - 1);
        say(rp, ga[k].x + (ga[k].w - tw) / 2, ga[k].y, grp->title, text);
        for (i = 0; i < ACNC_MAX_LINES && grp->line[i]; ++i)
            say(rp, ga[k].x + INSET, ga[k].y + title_band + PAD + i * line_h, grp->line[i], text);
    }
    if (dri) FreeScreenDrawInfo(scr, dri);
}

/* Builds the gadgets for the current page and size, and draws it all. */
static void redo(void)
{
    if (glist) {
        RemoveGList(win, glist, -1);
        FreeGadgets(glist);
        glist = NULL;
    }
    EraseRect(win->RPort, win->BorderLeft, win->BorderTop,
              win->Width - win->BorderRight - 1, win->Height - win->BorderBottom - 1);
    if (!make_gadgets()) return;
    AddGList(win, glist, ~0, -1, NULL);
    RefreshGList(glist, win, NULL, -1);
    GT_RefreshWindow(win, NULL);
    draw_static();
}

/* ---- the Commodity's window --------------------------------------------- */

static void hide_window(void)
{
    if (win) {
        ClearMenuStrip(win);
        CloseWindow(win);
        win = NULL;
    }
    if (glist) { FreeGadgets(glist); glist = NULL; }
    if (menu) { FreeMenus(menu); menu = NULL; }
    if (vi) { FreeVisualInfo(vi); vi = NULL; }
    if (font) { CloseFont(font); font = NULL; }
    if (scr) { UnlockPubScreen(NULL, scr); scr = NULL; }
}

static void show_window(void)
{
    int w, h;
    if (win) {
        WindowToFront(win);
        ActivateWindow(win);
        return;
    }
    scr = LockPubScreen(NULL);
    if (!scr) return;
    vi = GetVisualInfo(scr, TAG_DONE);
    if (!vi || !choose_font()) { hide_window(); return; }
    menu = CreateMenus(menus, TAG_DONE);
    if (menu) LayoutMenus(menu, vi, GTMN_NewLookMenus, TRUE, TAG_DONE);

    w = 2 * MARGIN + mx_w + MARGIN + nat_w;
    h = 2 * MARGIN + line_h + PAD + nat_h + PAD + line_h + 4;
    win = OpenWindowTags(NULL,
        WA_Title, (ULONG)"OpenSocketControl",
        WA_ScreenTitle, (ULONG)"OpenSocket - networking for the Amiga",
        WA_PubScreen, (ULONG)scr,
        WA_InnerWidth, w, WA_InnerHeight, h,
        WA_Left, (scr->Width - w) / 2, WA_Top, (scr->Height - h) / 2,
        WA_Activate, TRUE, WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE,
        WA_SizeGadget, TRUE, WA_SizeBBottom, TRUE, WA_SmartRefresh, TRUE, WA_NewLookMenus, TRUE,
        WA_IDCMP, IDCMP_CLOSEWINDOW | IDCMP_GADGETUP | IDCMP_GADGETDOWN | IDCMP_MENUPICK | IDCMP_VANILLAKEY |
                  IDCMP_REFRESHWINDOW | IDCMP_NEWSIZE | MXIDCMP | BUTTONIDCMP | TEXTIDCMP,
        TAG_DONE);
    if (!win) { hide_window(); return; }
    WindowLimits(win, win->Width, win->Height, ~0, ~0);   /* never smaller than the pages need */
    if (menu) SetMenuStrip(win, menu);
    redo();
}

static void set_page(int p)
{
    page = (p + ACNC_PAGES) % ACNC_PAGES;
    if (win) redo();
}

static void about(void)
{
    struct EasyStruct es = { sizeof(struct EasyStruct), 0, "OpenSocketControl",
        VERSION_TEXT "\n\nThe OpenSocket Commodity.\nNetworking runs without it.", "OK" };
    EasyRequestArgs(win, &es, NULL, NULL);
}

static void menu_pick(UWORD number)
{
    while (number != MENUNULL && win) {
        struct MenuItem *item = ItemAddress(menu, number);
        if (!item) break;
        switch ((ULONG)GTMENUITEM_USERDATA(item)) {
            case M_ABOUT: about(); break;
            case M_HIDE: hide_window(); return;
            case M_QUIT: quit_now = TRUE; return;
        }
        number = item->NextSelect;
    }
}

/* A button's underlined letter, on the page that shows it. */
static void shortcut(UWORD key)
{
    int k, i;
    for (k = 0; k < acnc_page[page].groups; ++k)
        for (i = 0; i < ACNC_MAX_BUTTONS && acnc_page[page].group[k].button[i].id; ++i) {
            const ACNCButton *b = &acnc_page[page].group[k].button[i];
            const char *u = strchr(b->label, '_');
            if (u && !b->disabled && (u[1] | 0x20) == (key | 0x20))
                return;     /* first light: the buttons have no backend yet */
        }
}

static void key(UWORD code, UWORD qualifier)
{
    if (code == 27) hide_window();
    else if (code == 9) set_page(page + ((qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)) ? -1 : 1));
    else if (code >= '1' && code < '1' + ACNC_PAGES) set_page(code - '1');
    else shortcut(code);
}

static void window_events(void)
{
    struct IntuiMessage *im;
    while (win && (im = GT_GetIMsg(win->UserPort)) != NULL) {
        ULONG class = im->Class;
        UWORD code = im->Code, qualifier = im->Qualifier;
        struct Gadget *gad = (struct Gadget *)im->IAddress;
        GT_ReplyIMsg(im);
        switch (class) {
            case IDCMP_CLOSEWINDOW: hide_window(); break;
            case IDCMP_REFRESHWINDOW:
                GT_BeginRefresh(win);
                draw_static();
                GT_EndRefresh(win, TRUE);
                break;
            case IDCMP_NEWSIZE: redo(); break;
            case IDCMP_GADGETDOWN:
                if (gad->GadgetID == GID_PAGES && code != page) set_page(code);
                break;
            case IDCMP_GADGETUP:
                /* The first-light UI has real controls only where their
                 * backends already exist. */
                break;
            case IDCMP_MENUPICK: menu_pick(code); break;
            case IDCMP_VANILLAKEY: key(code, qualifier); break;
        }
    }
}

static int control_main(void)
{
    int i;
    BOOL running = TRUE;

    GadToolsBase = OpenLibrary("gadtools.library", 37);
    if (!GadToolsBase) {
        PutStr("OpenSocketControl: needs gadtools.library 37 (AmigaOS 2.04 or later).\n");
        return 20;
    }
    for (i = 0; i < ACNC_PAGES; ++i) page_names[i] = (STRPTR)acnc_page[i].name;
    acnc_read_state();
    i = acnc_broker_open();
    if (i != 1) {
        /* ACNC_ALREADY_RUNNING: Exchange has told that copy to show itself. */
        if (i != ACNC_ALREADY_RUNNING) PutStr("OpenSocketControl: could not start the Commodity.\n");
        acnc_broker_close();
        CloseLibrary(GadToolsBase);
        return i == ACNC_ALREADY_RUNNING ? 5 : 20;
    }
    show_window();

    while (running && !quit_now) {
        ULONG winsig = win ? 1UL << win->UserPort->mp_SigBit : 0;
        ULONG sigs = Wait(winsig | acnc_broker_signal() | SIGBREAKF_CTRL_C);
        if (sigs & SIGBREAKF_CTRL_C) running = FALSE;
        if (sigs & acnc_broker_signal()) running = acnc_broker_handle(show_window, hide_window) && running;
        if (sigs & winsig) window_events();
    }

    hide_window();
    acnc_broker_close();
    CloseLibrary(GadToolsBase);
    return 0;
}

int main(void)
{
    return acnc_main_with_stack(control_main, 32768);
}
