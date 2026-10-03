/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef ACNET_STACK_H
#define ACNET_STACK_H
/* Runs a program's body on a stack of at least `bytes`, swapped in with
 * exec's StackSwap when the one it was given is smaller.
 *
 * The libnix that the os32 stove (bebbo's m68k-amigaos-gcc 6.5) links has no
 * stack swapping in its startup (ncrt0.o), so `__stack` alone does nothing: a
 * program started with Run from a boot-time shell (S:User-Startup) gets 4 KB
 * on AmigaOS 3.2.3, whatever it asks for. ACNet's calls run on the caller's
 * stack too: WaitSelect alone takes about 1.3 KB of it.
 *
 *   static int body(int argc, char **argv) { ... }
 *   int main(int argc, char **argv) { return acnet_main_with_stack(body, argc, argv, 32768); }
 *
 * The body must return, not call exit(): exit() would leave the task on the
 * swapped stack. Workbench start-ups pass through as they came (argc 0). */
int acnet_main_with_stack(int (*body)(int, char **), int argc, char **argv, unsigned long bytes);
#endif
