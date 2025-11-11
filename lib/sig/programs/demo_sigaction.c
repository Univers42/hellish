/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   demo_sigaction.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "test.h"

static volatile int g_count = 0;

static void handler_a(int sig)
{
    (void)sig;
    g_count++;
    printf("  handler_a run (count=%d)\n", g_count);
}

static void handler_b(int sig)
{
    (void)sig;
    g_count += 10;
    printf("  handler_b run (count=%d)\n", g_count);
}

static void demo_native_sigaction_with_custom_mask(void)
{
    struct sigaction act;
    struct sigaction old;
    t_sigset mask;
    t_sigset prev;

    SECTION("1) native sigaction + custom ft_sigprocmask");
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler_a;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    /* Install with native sigaction */
    if (sigaction(SIGUSR1, &act, &old) < 0)
    {
        perror("sigaction");
        FAIL("sigaction failed");
        return;
    }
    PASS("sigaction installed handler_a");

    /* Block with our custom bitmask API */
    ft_sigemptyset(&mask);
    ft_sigaddset(&mask, SIGUSR1);
    CHECK_TRUE(ft_sigprocmask(SIG_BLOCK, &mask, &prev) == 0,
               "ft_sigprocmask blocked SIGUSR1", "ft_sigprocmask block failed");

    g_count = 0;
    kill(getpid(), SIGUSR1);
    CHECK_EQ_INT(g_count, 0, "SIGUSR1 pending while blocked",
                 "SIGUSR1 delivered while blocked");

    /* Unblock and ensure pending delivery */
    CHECK_TRUE(ft_sigprocmask(SIG_SETMASK, &prev, NULL) == 0,
               "ft_sigprocmask restored previous mask", "ft_sigprocmask restore failed");
    CHECK_EQ_INT(g_count, 1, "Pending SIGUSR1 delivered after unblock",
                 "Pending SIGUSR1 not delivered on unblock");

    /* Restore previous handler */
    sigaction(SIGUSR1, &old, NULL);
}

static void demo_switch_handler_between_native_and_custom(void)
{
    struct sigaction act;
    struct sigaction old;

    SECTION("2) switch handler: native -> custom -> native");
    /* Install native handler_a */
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler_a;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGUSR1, &act, &old) < 0)
    {
        perror("sigaction");
        FAIL("sigaction failed");
        return;
    }
    PASS("native handler_a installed");

    /* Switch to handler_b using our custom set_signal_handler */
    set_signal_handler(SIGUSR1, handler_b);
    PASS("custom set_signal_handler -> handler_b");

    g_count = 0;
    kill(getpid(), SIGUSR1);
    CHECK_EQ_INT(g_count, 10, "handler_b executed (count=10)",
                 "handler_b not executed");

    /* Switch back to handler_a using native sigaction */
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler_a;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGUSR1, &act, NULL) == 0)
        PASS("back to native handler_a");
    else
        FAIL("sigaction restore failed");

    g_count = 0;
    kill(getpid(), SIGUSR1);
    CHECK_EQ_INT(g_count, 1, "handler_a executed after restore",
                 "handler_a not executed after restore");

    /* Cleanup: restore previous */
    sigaction(SIGUSR1, &old, NULL);
}

int main(void)
{
    printf(C_YEL "=== Demo: Interop native sigaction <-> custom ft_* API ===" C_RST "\n");
    demo_native_sigaction_with_custom_mask();
    demo_switch_handler_between_native_and_custom();
    printf(C_YEL "\n=== Done (see PASS/FAIL) ===" C_RST "\n");
    return (0);
}
