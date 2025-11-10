/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   demo_signals.c                                     :+:      :+:    :+:   */
/*                                                                            */
/* ************************************************************************** */

#include "test.h"

static int parse_sig(const char *s)
{
    int sig;

    if (!s || !*s)
        return (NO_SIG);
    sig = decode_signal(s, DSIG_SIGPREFIX | DSIG_NOCASE);
    return (sig);
}

static void cmd_help(void)
{
    puts(C_YEL "Commands:" C_RST);
    puts("  help                         - show this help");
    puts("  list                         - list signals (num:name)");
    puts("  name <sig>                   - print canonical name");
    puts("  decode <string>              - decode name/number/RT string");
    puts("  trap <sig>                   - install trap handler (records pending)");
    puts("  untrap <sig>                 - restore default for signal");
    puts("  ignore <sig>                 - set ignore");
    puts("  hardignore <sig>             - mark hard ignored + ignore");
    puts("  default <sig>                - restore default action");
    puts("  send <sig>                   - send signal to self");
    puts("  pending                      - show first/next pending traps");
    puts("  clear                        - clear all pending traps");
    puts("  mask                         - show current mask (bitmask)");
    puts("  block <sig>                  - block signal (ft_*)");
    puts("  unblock <sig>                - unblock to previous mask (ft_*)");
    puts("  orig <sig>                   - fetch and cache original handler");
    puts("  reset_handlers               - reset trapped to originals (no free)");
    puts("  restore_originals            - restore all original handlers");
    puts("  exit                         - quit");
}

static void cmd_list(void)
{
    int i;
    char *nm;

    i = 1;
    while (i < NSIG)
    {
        nm = signal_name(i);
        printf("%2d: %s\n", i, nm ? nm : "(null)");
        i++;
    }
}

static void cmd_name(const char *arg)
{
    int sig;

    sig = parse_sig(arg);
    if (sig <= 0 || sig >= NSIG)
        return ((void)printf("invalid: %s\n", arg));
    printf("%s\n", signal_name(sig));
}

static void cmd_decode(const char *arg)
{
    int sig;

    sig = parse_sig(arg);
    if (sig == NO_SIG)
        printf("decode: %s -> NO_SIG\n", arg);
    else
        printf("decode: %s -> %d (%s)\n", arg, sig, signal_name(sig));
}

static void cmd_trap(int sig)
{
    if (sig <= 0 || sig >= NSIG)
        return;
    /* install trap: use library trap mechanism (trap_handler will record) */
    set_signal(sig, "demo_trap");
    printf("trap set for %s\n", signal_name(sig));
}

static void cmd_untrap(int sig)
{
    if (sig <= 0 || sig >= NSIG)
        return;
    restore_default_signal(sig);
    printf("trap removed/default restored for %s\n", signal_name(sig));
}

static void cmd_ignore(int sig)
{
    if (sig <= 0 || sig >= NSIG)
        return;
    ignore_signal(sig);
    printf("ignored %s\n", signal_name(sig));
}

static void cmd_hardignore(int sig)
{
    if (sig <= 0 || sig >= NSIG)
        return;
    set_signal_hard_ignored(sig);
    set_signal_handler(sig, SIG_IGN);
    printf("hard-ignored %s\n", signal_name(sig));
}

static void cmd_default(int sig)
{
    if (sig <= 0 || sig >= NSIG)
        return;
    restore_default_signal(sig);
    printf("default restored for %s\n", signal_name(sig));
}

static void cmd_send(int sig)
{
    if (sig <= 0 || sig >= NSIG)
        return;
    kill(getpid(), sig);
    printf("sent %s\n", signal_name(sig));
}

static void cmd_pending(void)
{
    int first;
    int next;

    first = first_pending_trap();
    if (first < 0)
        return ((void)puts("no pending traps"));
    printf("first pending: %s (%d)\n", signal_name(first), first);
    next = first;
    while ((next = next_pending_trap(next + 1)) >= 0)
        printf("next pending: %s (%d)\n", signal_name(next), next);
}

static void cmd_clear(void)
{
    clear_pending_traps();
    puts("pending traps cleared");
}

static void cmd_mask_show(void)
{
    t_sigset cur;

    if (ft_sigprocmask(SIG_SETMASK, NULL, &cur) == 0)
        printf("current mask: 0x%x\n", cur);
    else
        perror("ft_sigprocmask");
}

static t_sigset g_saved_mask = 0;

static void cmd_block(int sig)
{
    t_sigset set;
    t_sigset old;

    if (sig <= 0 || sig >= NSIG)
        return;
    ft_sigemptyset(&set);
    ft_sigaddset(&set, sig);
    if (ft_sigprocmask(SIG_BLOCK, &set, &old) == 0)
    {
        g_saved_mask = old;
        printf("blocked %s, saved old mask=0x%x\n", signal_name(sig), old);
    }
    else
        perror("ft_sigprocmask");
}

static void cmd_unblock(void)
{
    if (ft_sigprocmask(SIG_SETMASK, &g_saved_mask, NULL) == 0)
        printf("restored previous mask 0x%x\n", g_saved_mask);
    else
        perror("ft_sigprocmask");
}

static void cmd_orig(int sig)
{
    if (sig <= 0 || sig >= NSIG)
        return;
    get_original_signal(sig);
    printf("original for %s cached\n", signal_name(sig));
}

static void cmd_reset_handlers(void)
{
    reset_signal_handlers();
    puts("reset trapped signals to originals (no free of trap strings)");
}

static void cmd_restore_originals(void)
{
    restore_original_signals();
    puts("restored all original signal handlers");
}

static int read_line(char *buf, size_t cap)
{
    ssize_t r;
    static const char prompt[] = C_CYN "signals> " C_RST;

    write(1, prompt, sizeof(prompt) - 1);
    r = read(STDIN_FILENO, buf, cap - 1);
    if (r <= 0)
        return (0);
    buf[r] = '\0';
    if (r && buf[r - 1] == '\n')
        buf[r - 1] = '\0';
    return (1);
}

int main(void)
{
    char buf[256];
    char cmd[64];
    char arg[128];

    initialize_signames();
    initialize_traps();
    puts(C_YEL "=== Demo: signal library explorer ===" C_RST);
    cmd_help();
    while (read_line(buf, sizeof(buf)))
    {
        cmd[0] = '\0';
        arg[0] = '\0';
        if (sscanf(buf, "%63s %127[^\n]", cmd, arg) < 1)
            continue;
        if (strcmp(cmd, "help") == 0)
            cmd_help();
        else if (strcmp(cmd, "list") == 0)
            cmd_list();
        else if (strcmp(cmd, "name") == 0)
            cmd_name(arg);
        else if (strcmp(cmd, "decode") == 0)
            cmd_decode(arg);
        else if (strcmp(cmd, "trap") == 0)
            cmd_trap(parse_sig(arg));
        else if (strcmp(cmd, "untrap") == 0)
            cmd_untrap(parse_sig(arg));
        else if (strcmp(cmd, "ignore") == 0)
            cmd_ignore(parse_sig(arg));
        else if (strcmp(cmd, "hardignore") == 0)
            cmd_hardignore(parse_sig(arg));
        else if (strcmp(cmd, "default") == 0)
            cmd_default(parse_sig(arg));
        else if (strcmp(cmd, "send") == 0)
            cmd_send(parse_sig(arg));
        else if (strcmp(cmd, "pending") == 0)
            cmd_pending();
        else if (strcmp(cmd, "clear") == 0)
            cmd_clear();
        else if (strcmp(cmd, "mask") == 0)
            cmd_mask_show();
        else if (strcmp(cmd, "block") == 0)
            cmd_block(parse_sig(arg));
        else if (strcmp(cmd, "unblock") == 0)
            cmd_unblock();
        else if (strcmp(cmd, "orig") == 0)
            cmd_orig(parse_sig(arg));
        else if (strcmp(cmd, "reset_handlers") == 0)
            cmd_reset_handlers();
        else if (strcmp(cmd, "restore_originals") == 0)
            cmd_restore_originals();
        else if (strcmp(cmd, "exit") == 0)
            break;
    }
    puts(C_YEL "=== Bye ===" C_RST);
    return (0);
}
