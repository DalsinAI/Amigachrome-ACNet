/*
 * ACNetControl's shared core: what both front ends show and do.
 *
 * ACNetControl comes as two programs with one behaviour:
 *   ACNetControl    ReAction, for AmigaOS 3.2.3 (acnetcontrol.c);
 *   ACNetControlGT  GadTools, for AmigaOS 2.04 to 3.2 (acnetcontrol_gt.c).
 * Each draws the pages below in its own toolkit; neither has page text of
 * its own, so the two cannot drift apart. They register the same broker
 * name, so only one runs at a time.
 *
 * BSD-3-Clause.
 */
#ifndef ACNETCONTROL_CORE_H
#define ACNETCONTROL_CORE_H

#include <exec/types.h>
#include <exec/ports.h>

enum {
    GID_TABS = 1,
    GID_GO_OFFLINE,
    GID_WIFI_RESCAN,
    GID_DIAG_DNS,
    GID_DIAG_CONNECT,
    GID_DIAG_INTERNET,
    GID_DIAG_COPY,
    GID_LOG_CLEAR,
    GID_LOG_SAVE,
    GID_FIRST_FREE              /* a front end's own gadgets start here */
};

#define ACNC_PAGES      5
#define ACNC_MAX_LINES  12
#define ACNC_MAX_BUTTONS 4

typedef struct ACNCButton {
    ULONG id;                   /* 0 ends the list */
    const char *label;          /* '_' marks the shortcut key */
    BOOL disabled;              /* no backend for it yet */
} ACNCButton;

typedef struct ACNCGroup {
    const char *title;
    const char *line[ACNC_MAX_LINES];   /* NULL ends the list; "" is a blank line */
    ACNCButton button[ACNC_MAX_BUTTONS];
    BOOL buttons_in_a_row;      /* else one under another */
} ACNCGroup;

typedef struct ACNCPage {
    const char *name;           /* the tab */
    int groups;                 /* 1, or 2 side by side */
    ACNCGroup group[2];
} ACNCPage;

extern const ACNCPage acnc_page[ACNC_PAGES];

#define ACNC_TITLE   "OpenSocketControl - Network Configuration"
#define ACNC_HOTKEY  "ctrl alt n"

/* The card's state, read once at start-up, and the lines that describe it. */
void acnc_read_state(void);
const char *acnc_status_line(void);     /* the status bar */

/* The commodity. show() and hide() are the front end's window calls.
 * acnc_broker_signal() is 0 until acnc_broker_open() succeeds.
 * acnc_broker_open(): 1 ready, 0 failed, ACNC_ALREADY_RUNNING when either
 * front end already runs (Exchange has told that copy to show itself). */
#define ACNC_ALREADY_RUNNING (-1)
int acnc_broker_open(void);
void acnc_broker_close(void);
ULONG acnc_broker_signal(void);
/* Handles Exchange's commands and the hotkey; FALSE when told to quit. */
BOOL acnc_broker_handle(void (*show)(void), void (*hide)(void));

/* Runs body on a stack of at least `bytes`. Our libnix ignores __stack, so
 * a program started with Run from a boot-time shell gets 4 KB. */
int acnc_main_with_stack(int (*body)(void), unsigned long bytes);

#endif
