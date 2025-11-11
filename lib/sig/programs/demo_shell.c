/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   demo_shell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/* ************************************************************************** */

#include "test.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>

extern char **environ;

static volatile sig_atomic_t g_got_sigint = 0;

/* Track foreground job */
static pid_t g_fg_pid = -1;
static int g_fg_stopped = 0;
static char g_last_line[256];

/* Async-signal-unsafe bits are standard for readline projects in 42 */
static void sigint_interactive(int sig)
{
    (void)sig;
    g_got_sigint = 1;
    write(STDOUT_FILENO, "\n", 1);
    rl_replace_line("", 0);
    rl_on_new_line();
    rl_redisplay();
}

static void install_interactive_handlers(void)
{
    set_signal_handler(SIGINT, sigint_interactive); /* Ctrl+C: clear line */
    set_signal_handler(SIGQUIT, SIG_IGN);           /* Ctrl+\: ignore in shell */
    set_signal_handler(SIGTSTP, SIG_IGN);           /* Ctrl+Z: ignore in shell */
}

static void restore_child_defaults(void)
{
    set_signal_handler(SIGINT, SIG_DFL);
    set_signal_handler(SIGQUIT, SIG_DFL);
    set_signal_handler(SIGTSTP, SIG_DFL);
}

/* Bind Ctrl+L to clear screen */
static int cb_clear_screen(int key, int count)
{
    (void)key;
    (void)count;
    write(STDOUT_FILENO, "\033[2J\033[H", 7);
    rl_on_new_line();
    rl_redisplay();
    return (0);
}

static void setup_readline(const char *prompt)
{
    rl_catch_signals = 0; /* we manage signals ourselves */
    rl_bind_keyseq("\\C-l", cb_clear_screen);
    (void)prompt;
}

/* Very small argv splitter (no quotes/escapes) */
static char **split_argv(const char *line)
{
    size_t i = 0, argc = 0;
    char *copy = NULL;
    char **argv = NULL;

    if (!line)
        return (NULL);
    copy = strdup(line);
    if (!copy)
        return (NULL);
    /* Count words */
    while (copy[i])
    {
        while (copy[i] && (copy[i] == ' ' || copy[i] == '\t'))
            i++;
        if (!copy[i])
            break;
        argc++;
        while (copy[i] && copy[i] != ' ' && copy[i] != '\t')
            i++;
    }
    argv = (char **)calloc(argc + 1, sizeof(char *));
    if (!argv)
        return (free(copy), NULL);
    /* Tokenize */
    i = 0;
    size_t a = 0;
    char *p = copy;
    while (*p)
    {
        while (*p == ' ' || *p == '\t')
            *p++ = '\0';
        if (*p == '\0')
            break;
        argv[a++] = p;
        while (*p && *p != ' ' && *p != '\t')
            p++;
    }
    argv[a] = NULL;
    return (argv); 
}

/* Minimal execvpe: search PATH and execve */
static int execvpe_local(const char *file, char *const argv[], char *const envp[])
{
    char buf[PATH_MAX];
    const char *path;
    const char *colon;
    size_t dirlen;

    if (!file || !*file)
    {
        errno = ENOENT;
        return (-1);
    }
    if (strchr(file, '/'))
    {
        execve(file, argv, envp);
        return (-1);
    }
    path = getenv("PATH");
    if (!path || !*path)
        path = "/bin:/usr/bin";
    while (*path)
    {
        colon = strchr(path, ':');
        dirlen = colon ? (size_t)(colon - path) : strlen(path);
        if (dirlen + 1 + strlen(file) + 1 < sizeof(buf))
        {
            memcpy(buf, path, dirlen);
            buf[dirlen] = '/';
            strcpy(buf + dirlen + 1, file);
            if (access(buf, X_OK) == 0)
                execve(buf, argv, envp);
        }
        if (!colon)
            break;
        path = colon + 1;
    }
    return (-1);
}

static void print_help(void)
{
    puts(C_YEL "Builtins:" C_RST
               " help, exit, fg, jobs, clear, spam [n]\n"
               "Typing tips:\n"
               " - Ctrl+C: cancel current input (shell) or interrupt foreground job\n"
               " - Ctrl+\\: ignored in shell; kills foreground job with SIGQUIT\n"
               " - Ctrl+Z: ignored in shell; stops foreground job (type 'fg' to resume)\n"
               " - Ctrl+S / Ctrl+Q: pause/resume terminal output (flow control)\n"
               " - Ctrl+R: reverse search history (readline)\n"
               " - Ctrl+U: erase entire line; Ctrl+W: erase previous word\n"
               " - Ctrl+D: send EOF at empty prompt to exit");
}

/* Foreground job control */
static void wait_foreground(pid_t pid, const char *desc)
{
    int st;

    g_fg_pid = pid;
    g_fg_stopped = 0;
    if (desc)
        strncpy(g_last_line, desc, sizeof(g_last_line) - 1);
    else
        g_last_line[0] = '\0';
    /* Wait for exit or stop (WUNTRACED to catch Ctrl+Z) */
    if (waitpid(pid, &st, WUNTRACED) < 0)
    {
        perror("waitpid");
        g_fg_pid = -1;
        return;
    }
    if (WIFSTOPPED(st))
    {
        g_fg_stopped = 1;
        printf(C_YEL "[stopped]" C_RST " %s (pid %d, signal %d)\n",
               g_last_line[0] ? g_last_line : "(unknown)", (int)pid, WSTOPSIG(st));
    }
    else
    {
        if (WIFSIGNALED(st))
            printf(C_RED "[terminated]" C_RST " by signal %d%s\n",
                   WTERMSIG(st), WCOREDUMP(st) ? " (core dumped)" : "");
        g_fg_pid = -1;
    }
}

static void do_fg(void)
{
    if (g_fg_pid <= 0 || g_fg_stopped == 0)
    {
        puts(C_YEL "fg:" C_RST " no stopped job");
        return;
    }
    if (kill(g_fg_pid, SIGCONT) < 0)
    {
        perror("kill(SIGCONT)");
        return;
    }
    g_fg_stopped = 0;
    wait_foreground(g_fg_pid, g_last_line);
}

/* Demo long output for Ctrl+S/Ctrl+Q */
static void do_spam(int lines)
{
    if (lines <= 0)
        lines = 500;
    for (int i = 0; i < lines; ++i)
    {
        printf("spam line %d: press Ctrl+S to freeze output, Ctrl+Q to resume...\n", i + 1);
        usleep(2000);
    }
}

/* Execute a line: builtin or external */
static void run_command_line(const char *line)
{
    char **argv;
    pid_t pid;

    if (!line || !*line)
        return;
    if (strcmp(line, "exit") == 0)
    {
        printf("exit\n");
        exit(0);
    }
    if (strcmp(line, "help") == 0)
        return (print_help());
    if (strcmp(line, "jobs") == 0)
    {
        if (g_fg_pid > 0 && g_fg_stopped)
            printf("[1]+  Stopped  %s (pid %d)\n", g_last_line, (int)g_fg_pid);
        else
            puts("no jobs");
        return;
    }
    if (strcmp(line, "fg") == 0)
        return (do_fg());
    if (strncmp(line, "spam", 4) == 0)
    {
        int n = 1000;
        (void)sscanf(line + 4, "%d", &n);
        return (do_spam(n));
    }
    if (strcmp(line, "clear") == 0)
    {
        cb_clear_screen(0, 0);
        return;
    }
    argv = split_argv(line);
    if (!argv || !argv[0])
    {
        free(argv ? argv[0] : NULL);
        free(argv);
        return;
    }
    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        free(argv[0]);
        free(argv);
        return;
    }
    if (pid == 0)
    {
        restore_child_defaults();
        /* child: run command via PATH search */
        execvpe_local(argv[0], argv, environ);
        perror(argv[0]);
        _exit(127);
    }
    /* Parent: ignore job signals so only the child is affected */
    set_signal_handler(SIGINT, SIG_IGN);
    set_signal_handler(SIGQUIT, SIG_IGN);
    wait_foreground(pid, line);
    /* Restore interactive handlers after job */
    install_interactive_handlers();
    /* Free argv storage (single contiguous block at argv[0]) */
    free(argv[0]);
    free(argv);
}

int main(void)
{
    const char *prompt = C_CYN "minishell> " C_RST;
    char *line;

    printf(C_YEL "=== Demo: readline-based mini-shell (custom signals + jobs) ===" C_RST "\n");
    setup_readline(prompt);
    install_interactive_handlers();
    print_help();
    while (1)
    {
        g_got_sigint = 0;
        line = readline(prompt);
        if (line == NULL)
        {
            printf("exit\n");
            break;
        }
        if (g_got_sigint)
        {
            free(line);
            continue;
        }
        if (*line)
            add_history(line);
        run_command_line(line);
        free(line);
    }
    return (0);
}
