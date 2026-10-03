/* SPDX-License-Identifier: BSD-3-Clause */
/* See acnet_stack.h. */
#include "acnet_stack.h"
#include <exec/types.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <proto/exec.h>

/* Static, not on the stack: between the two StackSwap calls nothing may be
 * read from the old stack's frame. */
static struct StackSwapStruct swap;
static int (*swap_body)(int, char **);
static int swap_argc, swap_rc;
static char **swap_argv;

/* The body's call has a function of its own, so its arguments are pushed and
 * popped on the new stack. Called straight from between the swaps, GCC may
 * leave the pop until after the swap back (it does with -fomit-frame-pointer)
 * and take it off the old stack, and the return then goes astray. */
static void __attribute__((noinline)) call_body(void)
{
    swap_rc = swap_body(swap_argc, swap_argv);
}

static void __attribute__((noinline)) run_swapped(void)
{
    StackSwap(&swap);
    call_body();
    StackSwap(&swap);
}

int acnet_main_with_stack(int (*body)(int, char **), int argc, char **argv, unsigned long bytes)
{
    struct Task *me = FindTask(NULL);
    APTR lower;

    if ((ULONG)me->tc_SPUpper - (ULONG)me->tc_SPLower >= bytes || !(lower = AllocVec(bytes, MEMF_ANY)))
        return body(argc, argv);                 /* big enough already, or run on what there is */
    swap.stk_Lower = lower;
    swap.stk_Upper = (ULONG)lower + bytes;
    swap.stk_Pointer = (APTR)swap.stk_Upper;
    swap_body = body;
    swap_argc = argc;
    swap_argv = argv;
    run_swapped();
    FreeVec(lower);
    return swap_rc;
}
