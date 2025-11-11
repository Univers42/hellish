/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   demo_zombie.c                                      :+:      :+:    :+:   */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "test.h"

static void sa_ignore_sigchld(void)
{
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    act.sa_flags = SA_NOCLDWAIT | SA_NOCLDSTOP;
    sigemptyset(&act.sa_mask);
    sigaction(SIGCHLD, &act, NULL);
}

static void spawn_and_show_zombie_then_reap(void)
{
    pid_t pid;
    int st;

    SECTION("Zombie demo: child exits, parent reaps later");
    pid = fork();
    if (pid == 0)
        _exit(0);
    usleep(50000);
    if (waitpid(pid, &st, WNOHANG) == pid)
        PASS("child ready to be reaped (zombie until wait)");
    else
        FAIL("child not ready?");
    /* Now reap */
    waitpid(pid, &st, 0);
    PASS("reaped child (no longer zombie)");
}

static void spawn_with_nocldwait(void)
{
    pid_t pid;

    SECTION("No zombies with SA_NOCLDWAIT");
    sa_ignore_sigchld();
    pid = fork();
    if (pid == 0)
        _exit(0);
    usleep(50000);
    /* Should already be reaped by kernel; waitpid should fail or return 0 */
    if (waitpid(pid, NULL, WNOHANG) <= 0)
        PASS("no zombie (kernel reaped)");
    else
        FAIL("unexpected waitpid return");
}

int main(void)
{
    printf(C_YEL "=== Demo: zombie behaviors (reap vs NOCLDWAIT) ===" C_RST "\n");
    spawn_and_show_zombie_then_reap();
    spawn_with_nocldwait();
    printf(C_YEL "\n=== Done (see PASS/FAIL) ===" C_RST "\n");
    return (0);
}
