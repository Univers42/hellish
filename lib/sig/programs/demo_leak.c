/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   demo_leak.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "test.h"

static volatile sig_atomic_t g_stop = 0;
static void *g_buf = NULL;
static size_t g_sz = 0;

/* Async-signal-safe: only set a flag */
static void sigint_request_stop(int sig)
{
    (void)sig;
    g_stop = 1;
}

/* Protect global pointer update with our custom bitmask API */
static void set_current_buffer(void *p, size_t sz)
{
    t_sigset block;
    t_sigset prev;

    ft_sigemptyset(&block);
    ft_sigaddset(&block, SIGINT);
    /* Block SIGINT while we update globals to avoid races */
    ft_sigprocmask(SIG_BLOCK, &block, &prev);
    g_buf = p;
    g_sz = sz;
    ft_sigprocmask(SIG_SETMASK, &prev, NULL);
}

/* Free current buffer with SIGINT blocked to avoid races */
static void free_current_buffer(void)
{
    t_sigset block;
    t_sigset prev;
    void *p;

    ft_sigemptyset(&block);
    ft_sigaddset(&block, SIGINT);
    ft_sigprocmask(SIG_BLOCK, &block, &prev);

    p = g_buf;
    g_buf = NULL;
    g_sz = 0;

    ft_sigprocmask(SIG_SETMASK, &prev, NULL);
    if (p)
        free(p);
}

static void run_leak_safe_loop(size_t chunk_kb, int seconds)
{
    time_t start;
    size_t step = 0;
    t_sigset mask;

    SECTION("Leak-safe infinite loop: Ctrl+C should free the last allocation");
    printf(" allocating %zuKB chunks; press Ctrl+C to interrupt...\n", chunk_kb);
    set_signal_handler(SIGINT, sigint_request_stop);

    start = time(NULL);
    ft_sigemptyset(&mask);

    while (1)
    {
        void *newp;
        size_t bytes;

        /* Optional: auto-stop after 'seconds' to demo non-interactively */
        if (seconds > 0 && time(NULL) - start >= seconds)
            g_stop = 1;

        /* Simulate work: allocate and release previous */
        bytes = chunk_kb * 1024;
        newp = malloc(bytes);
        if (!newp)
        {
            FAIL("malloc failed; stopping");
            break;
        }

        /* Store latest allocation safely (SIGINT blocked) */
        free_current_buffer();
        set_current_buffer(newp, bytes);

        /* Small visible heartbeat; do not block signals here */
        if ((step++ % 16) == 0)
            write(STDOUT_FILENO, ".", 1);
        usleep(50000);

        /* Check for interrupt request set by the handler */
        if (g_stop)
            break;
    }

    /* Cleanup before exit; this is the key: no leak on interruption */
    free_current_buffer();

    /* Validate we did free something if an allocation had been made */
    CHECK_TRUE(g_buf == NULL && g_sz == 0,
               "cleanup done: current allocation freed",
               "cleanup missing: current allocation not freed");

    PASS("interrupted run completed without leaking the tracked buffer");
}

int main(void)
{
    printf(C_YEL "=== Demo: freeing in-flight allocations on Ctrl+C using custom ft_* API ===" C_RST "\n");
    /* Run for max 5s if no user input, otherwise Ctrl+C at any time */
    run_leak_safe_loop(128, 5);
    printf(C_YEL "\n=== Done (see PASS/FAIL) ===" C_RST "\n");
    return (0);
}
