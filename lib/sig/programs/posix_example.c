/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   posix_example.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/10 18:55:00 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 18:51:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "test.h"

/* Colors */
static volatile int g_signal_count = 0;
static volatile int g_suspend_ready = 0;


static void example_handler(int sig)
{
    g_signal_count++;
    printf("  handler: signal %d (count=%d)\n", sig, g_signal_count);
    (void)sig;
}

static void alarm_handler(int sig)
{
    (void)sig;
    g_suspend_ready = 1;
    printf("  handler: SIGALRM received, waking ft_sigsuspend()\n");
}

static void section(const char *title)
{
    printf(C_CYN "\n=== %s ===\n" C_RST, title);
}

/* 1) Bitmask API operations */
static void demo_sigset_operations(void)
{
    t_sigset set;
    t_sigset full;

    section("1. Bitmask API: ft_sig* operations");
    ft_sigemptyset(&set);
    printf(" start empty mask=0x%x\n", set);

    ft_sigaddset(&set, SIGUSR1);
    ft_sigaddset(&set, SIGUSR2);
    printf(" after add SIGUSR1,SIGUSR2 mask=0x%x\n", set);
    if (!ft_sigismember(&set, SIGUSR1))
        FAIL("SIGUSR1 missing after add");
    else
        PASS("SIGUSR1 present after add");
    if (!ft_sigismember(&set, SIGUSR2))
        FAIL("SIGUSR2 missing after add");
    else
        PASS("SIGUSR2 present after add");

    ft_sigdelset(&set, SIGUSR1);
    printf(" after del SIGUSR1 mask=0x%x\n", set);
    if (ft_sigismember(&set, SIGUSR1))
        FAIL("SIGUSR1 still present after delete");
    else
        PASS("SIGUSR1 removed");

    ft_sigfillset(&full);
    printf(" filled mask=0x%x (bitmask wide)\n", full);
}

/* 2) Handler install/trigger */
static void demo_signal_handler(void)
{
    section("2. set_signal_handler demo (SIGUSR1)");
    g_signal_count = 0;
    set_signal_handler(SIGUSR1, example_handler);
    printf(" sending SIGUSR1...\n");
    kill(getpid(), SIGUSR1);
    if (g_signal_count == 1)
        PASS("SIGUSR1 delivered and handled");
    else
        FAIL("SIGUSR1 handler did not run exactly once");
}

/* 3) Block/unblock with ft_sigprocmask */
static void demo_sigprocmask(void)
{
    t_sigset block;
    t_sigset prev;

    section("3. ft_sigprocmask block/unblock (SIGUSR1)");
    g_signal_count = 0;
    ft_sigemptyset(&block);
    ft_sigaddset(&block, SIGUSR1);
    printf(" blocking mask=0x%x\n", block);
    ft_sigprocmask(SIG_BLOCK, &block, &prev);
    printf(" prev mask saved=0x%x\n", prev);

    kill(getpid(), SIGUSR1);
    if (g_signal_count == 0)
        PASS("SIGUSR1 pending while blocked");
    else
        FAIL("SIGUSR1 should be pending, not delivered");

    printf(" restoring previous mask...\n");
    ft_sigprocmask(SIG_SETMASK, &prev, NULL);
    if (g_signal_count == 1)
        PASS("pending SIGUSR1 delivered on unblock");
    else
        FAIL("pending SIGUSR1 not delivered on unblock");

    g_signal_count = 0;
    kill(getpid(), SIGUSR1);
    if (g_signal_count == 1)
        PASS("SIGUSR1 delivered normally after unblock");
    else
        FAIL("SIGUSR1 delivery failed after unblock");
}

/* 4) Wrapper block/unblock */
static void demo_block_unblock(void)
{
    t_sigset set;
    t_sigset oset;

    section("4. block_signal / unblock_signal (SIGINT)");
    g_signal_count = 0;
    set_signal_handler(SIGINT, example_handler);
    block_signal(SIGINT, &set, &oset);
    printf(" block_signal: set=0x%x saved=0x%x\n", set, oset);
    kill(getpid(), SIGINT);
    if (g_signal_count == 0)
        PASS("SIGINT blocked (no delivery)");
    else
        FAIL("SIGINT delivered while blocked");

    unblock_signal(&oset);
    if (g_signal_count == 1)
        PASS("pending SIGINT delivered on unblock");
    else
        FAIL("pending SIGINT not delivered on unblock");

    g_signal_count = 0;
    kill(getpid(), SIGINT);
    if (g_signal_count == 1)
        PASS("SIGINT delivered normally after unblock");
    else
        FAIL("SIGINT delivery failed after unblock");
}

/* 5) Suspend/until SIGALRM arrives */
static void demo_sigsuspend(void)
{
    t_sigset mask;
    int r;

    section("5. ft_sigsuspend waiting for SIGALRM");
    set_signal_handler(SIGALRM, alarm_handler);
    ft_sigemptyset(&mask);
    printf(" scheduling alarm for 1 second...\n");
    alarm(1);
    g_suspend_ready = 0;
    r = ft_sigsuspend(&mask);
    (void)r;
    if (g_suspend_ready)
        PASS("ft_sigsuspend woke on SIGALRM");
    else
        FAIL("ft_sigsuspend did not wake on SIGALRM");
}

/* 6) Real-life sequence around a critical section */
static void demo_real_life_sequence(void)
{
    t_sigset block;
    t_sigset prev;

    section("6. Real-life: protect critical section (SIGUSR2)");
    set_signal_handler(SIGUSR2, example_handler);
    g_signal_count = 0;
    ft_sigemptyset(&block);
    ft_sigaddset(&block, SIGUSR2);
    ft_sigprocmask(SIG_BLOCK, &block, &prev);

    printf(" entering critical section (SIGUSR2 blocked)\n");
    kill(getpid(), SIGUSR2);
    printf(" inside section: count=%d (expect 0)\n", g_signal_count);
    if (g_signal_count == 0)
        PASS("no delivery while blocked");
    else
        FAIL("signal should be pending, not delivered");

    ft_sigprocmask(SIG_SETMASK, &prev, NULL);
    printf(" after section: count=%d (expect 1)\n", g_signal_count);
    if (g_signal_count == 1)
        PASS("pending SIGUSR2 delivered after unblock");
    else
        FAIL("pending SIGUSR2 not delivered after unblock");
}

int main(void)
{
    printf(C_YEL "=== POSIX Custom Signal API Enhanced Demo ===" C_RST "\n");
    set_signal_handler(SIGINT, example_handler);

    demo_sigset_operations();
    demo_signal_handler();
    demo_sigprocmask();
    demo_block_unblock();
    demo_sigsuspend();
    demo_real_life_sequence();

    printf(C_YEL "\n=== Summary: inspect PASS/FAIL lines above ===\n" C_RST);
    return (0);
}
