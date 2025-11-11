/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   demo_kill.c                                        :+:      :+:    :+:   */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "test.h"

static void child_ignore_term(int enable)
{
    if (enable)
        set_signal_handler(SIGTERM, SIG_IGN);
    else
        set_signal_handler(SIGTERM, SIG_DFL);
}

static void parent_demo_kill_sequence(int ignore_term)
{
    pid_t pid;
    int st;

    SECTION(ignore_term
                ? "Kill demo: child ignores SIGTERM -> expect SIGKILL"
                : "Kill demo: child dies on SIGTERM");

    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        FAIL("fork failed");
        return;
    }
    if (pid == 0)
    {
        child_ignore_term(ignore_term);
        pause();
        _exit(0);
    }
    usleep(50000);
    kill(pid, SIGTERM);
    usleep(50000);
    if (waitpid(pid, &st, WNOHANG) == 0)
    {
        if (ignore_term)
        {
            PASS("SIGTERM ignored as expected, using SIGKILL");
            kill(pid, SIGKILL);
        }
        else
            FAIL("child did not exit on SIGTERM");
    }
    waitpid(pid, &st, 0);
    if (ignore_term)
        CHECK_TRUE(WIFSIGNALED(st) && WTERMSIG(st) == SIGKILL,
                   "child killed by SIGKILL", "unexpected child termination");
    else
        CHECK_TRUE(WIFSIGNALED(st) && WTERMSIG(st) == SIGTERM,
                   "child terminated by SIGTERM", "unexpected child termination");
}

int main(void)
{
    printf(C_YEL "=== Demo: kill()/signal handling with custom helpers ===" C_RST "\n");
    parent_demo_kill_sequence(0);
    parent_demo_kill_sequence(1);
    printf(C_YEL "\n=== Done (see PASS/FAIL) ===" C_RST "\n");
    return (0);
}
