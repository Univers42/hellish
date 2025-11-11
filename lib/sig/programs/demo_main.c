/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   demo_main.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/08 09:20:00 by dlesieur          #+#    #+#             */
/*   Updated: 2025/11/10 14:32:54 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "trap.h"
#include "quit.h"
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>

/* Color helpers */
#define C_RESET "\033[0m"
#define C_RED "\033[31m"
#define C_GRN "\033[32m"
#define C_YEL "\033[33m"
#define C_BLU "\033[34m"
#define C_MAG "\033[35m"
#define C_CYN "\033[36m"
#define C_BOLD "\033[1m"

#ifndef ZOMBIE_PROTECT
#define ZOMBIE_PROTECT 0 /* 0: no reap, 1: handler reap, 2: SA_NOCLDWAIT */
#endif

typedef struct s_child
{
	pid_t pid;
	int pipe_in[2];
	int pipe_out[2];
	char *leak_buf;
	int zombie;
} t_child;

static volatile sig_atomic_t g_stop = 0;
static volatile sig_atomic_t g_got_int = 0;
static volatile sig_atomic_t g_sigchld_received = 0;
static int g_auto_reap = 0; /* if 0 we accumulate zombies */
static int g_leak_mode = 0; /* if 1 we leak parent buffers */
static int g_verbose = 1;	/* verbose signal logs */
static t_child *g_children = NULL;
static int g_child_count = 0;
static int g_protect_mode = ZOMBIE_PROTECT;

static void logmsg(const char *clr, const char *tag, const char *msg)
{
	if (g_verbose)
	{
		write(1, clr, strlen(clr));
		write(1, "[PARENT] ", 9);
		write(1, tag, strlen(tag));
		write(1, msg, strlen(msg));
		write(1, C_RESET, strlen(C_RESET));
	}
}

static void demo_sigint(int sig)
{
	(void)sig;
	g_got_int = 1;
}

static void demo_sigterm(int sig)
{
	(void)sig;
	g_stop = 1;
}

static void demo_sigchld(int sig)
{
	(void)sig;
	g_sigchld_received = 1;
	/* Don't call waitpid here - do it in main loop for safety */
}

static void close_fd(int fd)
{
	if (fd >= 0)
		close(fd);
}

static void reap_zombies(void)
{
	int status;
	pid_t pid;
	int cnt = 0;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		cnt++;
		for (int i = 0; i < g_child_count; i++)
		{
			if (g_children[i].pid == pid)
			{
				g_children[i].zombie = 0;
				g_children[i].pid = -1; /* Mark as dead */
				break;
			}
		}
	}

	if (cnt > 0 && g_verbose)
	{
		char buf[128];
		int n = snprintf(buf, sizeof(buf), "reaped %d children\n", cnt);
		if (n > 0 && n < (int)sizeof(buf))
			logmsg(C_GRN, "[REAP] ", buf);
	}
}

static void free_child_resources(t_child *c)
{
	close_fd(c->pipe_in[0]);
	c->pipe_in[0] = -1;
	close_fd(c->pipe_in[1]);
	c->pipe_in[1] = -1;
	close_fd(c->pipe_out[0]);
	c->pipe_out[0] = -1;
	close_fd(c->pipe_out[1]);
	c->pipe_out[1] = -1;

	/* Free parent buffers only when NOT in leak mode */
	if (g_leak_mode == 0 && c->leak_buf)
	{
		free(c->leak_buf);
		c->leak_buf = NULL;
	}
	/* When leak_mode == 1: we intentionally keep c->leak_buf allocated
	   to demonstrate parent-side leaks visible in valgrind. */
}

static void set_protect_mode(int mode)
{
	g_protect_mode = mode;
	g_auto_reap = (mode >= 1);

	if (mode == 2)
	{
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sa.sa_flags = SA_NOCLDWAIT | SA_NOCLDSTOP;
		sigemptyset(&sa.sa_mask);
		if (sigaction(SIGCHLD, &sa, NULL) < 0)
			logmsg(C_RED, "[ERR] ", "sigaction failed\n");
		else
			logmsg(C_GRN, "[CFG] ", "SA_NOCLDWAIT enabled (kernel reaps)\n");
	}
	else
	{
		set_signal_handler(SIGCHLD, demo_sigchld);
		char buf[64];
		int n = snprintf(buf, sizeof(buf), "auto_reap=%s\n", g_auto_reap ? "on" : "off");
		if (n > 0 && n < (int)sizeof(buf))
		{
			write(1, C_GRN "[CFG] " C_RESET, strlen(C_GRN "[CFG] " C_RESET));
			write(1, buf, n);
		}
	}
}

static int starts_with(const char *s, const char *p)
{
	return strncmp(s, p, strlen(p)) == 0;
}

static int spawn_child(const char *label)
{
	t_child *new_children = realloc(g_children, sizeof(t_child) * (g_child_count + 1));
	if (!new_children)
		return -1;
	g_children = new_children;

	t_child *c = &g_children[g_child_count];
	memset(c, 0, sizeof(*c));
	c->pid = -1;
	c->zombie = 0;
	c->leak_buf = NULL;
	c->pipe_in[0] = c->pipe_in[1] = -1;
	c->pipe_out[0] = c->pipe_out[1] = -1;

	if (pipe(c->pipe_in) < 0)
		return -1;
	if (pipe(c->pipe_out) < 0)
	{
		close_fd(c->pipe_in[0]);
		close_fd(c->pipe_in[1]);
		return -1;
	}

	/* Allocate parent leak buffer when leak_mode == 1 (leak enabled) */
	if (g_leak_mode == 1)
		c->leak_buf = (char *)malloc(64 * 1024);

	c->pid = fork();
	if (c->pid == 0)
	{
		/* Child process */
		if (dup2(c->pipe_in[0], STDIN_FILENO) < 0)
			_Exit(1);
		if (dup2(c->pipe_out[1], STDOUT_FILENO) < 0)
			_Exit(1);

		close_fd(c->pipe_in[0]);
		close_fd(c->pipe_in[1]);
		close_fd(c->pipe_out[0]);
		close_fd(c->pipe_out[1]);

		set_signal_handler(SIGPIPE, SIG_IGN);
		set_signal_handler(SIGTERM, SIG_DFL); /* Make sure SIGTERM kills us */

		/* Child leak only when leak_mode == 1 (for demonstration) */
		if (g_leak_mode == 1)
		{
			char *child_leak = (char *)malloc(256 * 1024);
			if (child_leak)
				memset(child_leak, 0xAB, 256 * 1024);
		}

		char buf[256];
		pid_t me = getpid(), pp = getppid();
		int n = snprintf(buf, sizeof(buf),
						 C_CYN "[CHILD pid=%d ppid=%d label=%s] started" C_RESET "\n",
						 (int)me, (int)pp, label);
		if (n > 0 && n < (int)sizeof(buf))
			write(STDERR_FILENO, buf, n);

		char iobuf[4096];
		ssize_t r;
		while ((r = read(STDIN_FILENO, iobuf, sizeof(iobuf))) > 0)
			write(STDOUT_FILENO, iobuf, (size_t)r);
		_Exit(0);
	}

	if (c->pid < 0)
	{
		free_child_resources(c);
		return -1;
	}

	/* Close unused ends in parent */
	close_fd(c->pipe_in[0]);
	c->pipe_in[0] = -1;
	close_fd(c->pipe_out[1]);
	c->pipe_out[1] = -1;

	if (fcntl(c->pipe_out[0], F_SETFL, O_NONBLOCK) < 0)
		logmsg(C_RED, "[WARN] ", "fcntl O_NONBLOCK failed\n");

	g_child_count++;
	logmsg(C_BLU, "[SPAWN] ", "child created\n");
	return 0;
}

static void kill_child(int idx, int sig)
{
	if (idx < 0 || idx >= g_child_count)
		return;
	t_child *c = &g_children[idx];
	if (c->pid <= 0)
		return;

	if (kill(c->pid, sig) < 0)
		logmsg(C_RED, "[KILL] ", "kill failed\n");
	else
	{
		if (sig == SIGKILL || sig == SIGTERM)
			c->zombie = 1;
		logmsg(C_RED, "[KILL] ", "signal sent\n");
	}
}

static void drain_child_output(t_child *c)
{
	char buf[1024];
	ssize_t r;
	const char prefix[] = C_MAG "[out] " C_RESET;

	while ((r = read(c->pipe_out[0], buf, sizeof(buf))) > 0)
	{
		write(1, prefix, strlen(prefix));
		write(1, buf, (size_t)r);
	}
}

static void status_report(void)
{
	char line[256];
	int alive = 0, zomb = 0;

	for (int i = 0; i < g_child_count; i++)
		if (g_children[i].pid > 0)
		{
			alive++;
			if (g_children[i].zombie)
				zomb++;
		}

	int n = snprintf(line, sizeof(line),
					 "children=%d alive=%d zombies=%d leak_mode=%d protect_mode=%d auto_reap=%d\n",
					 g_child_count, alive, zomb, g_leak_mode, g_protect_mode, g_auto_reap);

	if (n > 0 && n < (int)sizeof(line))
	{
		const char prefix[] = C_BOLD C_YEL "[STATUS] " C_RESET;
		write(1, prefix, strlen(prefix));
		write(1, line, n);
	}

	for (int i = 0; i < g_child_count; i++)
		if (g_children[i].pid > 0)
		{
			char b[128];
			int bn = snprintf(b, sizeof(b), "  idx=%d pid=%d zombie=%d\n",
							  i, (int)g_children[i].pid, g_children[i].zombie);
			if (bn > 0 && bn < (int)sizeof(b))
				write(1, b, bn);
		}
}

static void cleanup_all(void)
{
	/* First, kill all children */
	for (int i = 0; i < g_child_count; i++)
		if (g_children[i].pid > 0)
		{
			kill(g_children[i].pid, SIGTERM);
		}

	/* Give them a moment to die gracefully */
	usleep(50000); /* 50ms */

	/* Force kill any survivors */
	for (int i = 0; i < g_child_count; i++)
		if (g_children[i].pid > 0)
		{
			kill(g_children[i].pid, SIGKILL);
		}

	/* Reap all zombies */
	if (g_auto_reap || g_protect_mode >= 1)
		reap_zombies();

	/* Close all pipes and free resources */
	for (int i = 0; i < g_child_count; i++)
		free_child_resources(&g_children[i]);

	/* Free the children array itself */
	free(g_children);
	g_children = NULL;
	g_child_count = 0;

	logmsg(C_GRN, "[CLEANUP] ", "done\n");
}

int main(void)
{
	set_signal_handler(SIGINT, demo_sigint);
	set_signal_handler(SIGTERM, demo_sigterm);
	set_protect_mode(ZOMBIE_PROTECT);
	ignore_signal(SIGPIPE);

	const char *msg1 = C_BOLD "Signal Demo Mini-Shell" C_RESET "\n";
	const char *msg2 = "Commands: spawn [n], kill [idx|all], zombie [idx], protect 0|1|2, leak on|off, status, crash, exit\n";
	const char *msg3 = C_YEL "Test scenario:" C_RESET " Set 'leak on' or 'leak off' FIRST, then 'spawn 3', then 'crash'.\n";
	const char *msg4 = "  leak on (parent leak) + crash: parent leaks ~64KB per child buffer.\n";
	const char *msg5 = "  leak off (clean) + crash: parent properly frees buffers on cleanup.\n";
	const char *msg6 = "  protect 0: zombies remain; check with 'ps aux | grep worker'.\n";
	const char *msg7 = C_CYN "Initial state: leak_mode=off (clean), protect_mode=" C_RESET;
	const char *msg8 = "Note: Toggle 'leak on' or 'leak off' affects future allocations.\n";

	write(1, msg1, strlen(msg1));
	write(1, msg2, strlen(msg2));
	write(1, msg3, strlen(msg3));
	write(1, msg4, strlen(msg4));
	write(1, msg5, strlen(msg5));
	write(1, msg6, strlen(msg6));
	write(1, msg7, strlen(msg7));
	char protect_buf[8];
	int pn = snprintf(protect_buf, sizeof(protect_buf), "%d\n", ZOMBIE_PROTECT);
	if (pn > 0 && pn < (int)sizeof(protect_buf))
		write(1, protect_buf, pn);
	write(1, msg8, strlen(msg8));

	char line[512];
	const char *prompt = C_CYN "mini> " C_RESET;

	while (!g_stop)
	{
		/* Check for SIGCHLD and reap if needed */
		if (g_sigchld_received && g_auto_reap)
		{
			g_sigchld_received = 0;
			reap_zombies();
		}

		write(1, prompt, strlen(prompt));
		ssize_t r = read(STDIN_FILENO, line, sizeof(line) - 1);
		if (r <= 0)
			break;
		if (g_got_int)
		{
			g_got_int = 0;
			logmsg(C_YEL, "[SIGINT] ", "interrupt received\n");
			continue;
		}
		line[r] = '\0';

		/* Remove trailing newline */
		if (r > 0 && line[r - 1] == '\n')
			line[r - 1] = '\0';

		if (starts_with(line, "exit"))
			break;
		else if (starts_with(line, "crash"))
		{
			const char *crash_msg = C_RED "[CRASH] simulated crash, no cleanup\n" C_RESET;
			write(1, crash_msg, strlen(crash_msg));

			/* Free the children array to avoid "still reachable" leak */
			free(g_children);
			g_children = NULL;

			_exit(42);
		}
		else if (starts_with(line, "spawn"))
		{
			int n = 1;
			sscanf(line + 5, "%d", &n);
			for (int i = 0; i < n; i++)
			{
				char lbl[32];
				snprintf(lbl, sizeof(lbl), "worker%02d", g_child_count);
				spawn_child(lbl);
			}
		}
		else if (starts_with(line, "kill all"))
		{
			for (int i = 0; i < g_child_count; i++)
				kill_child(i, SIGKILL);
		}
		else if (starts_with(line, "kill"))
		{
			int idx = -1;
			sscanf(line + 4, "%d", &idx);
			kill_child(idx, SIGKILL);
		}
		else if (starts_with(line, "zombie"))
		{
			int idx = -1;
			sscanf(line + 6, "%d", &idx);
			kill_child(idx, SIGTERM);
		}
		else if (starts_with(line, "auto_reap on"))
		{
			set_protect_mode(1);
		}
		else if (starts_with(line, "auto_reap off"))
		{
			set_protect_mode(0);
		}
		else if (starts_with(line, "protect"))
		{
			int m = -1;
			sscanf(line + 7, "%d", &m);
			if (m >= 0 && m <= 2)
				set_protect_mode(m);
		}
		else if (starts_with(line, "leak on"))
		{
			g_leak_mode = 1;
			logmsg(C_RED, "[CFG] ", "leak_mode=parent-leak (buffers will leak on crash)\n");
		}
		else if (starts_with(line, "leak off"))
		{
			/* Free existing leak buffers when disabling leak mode */
			for (int i = 0; i < g_child_count; i++)
			{
				if (g_children[i].leak_buf)
				{
					free(g_children[i].leak_buf);
					g_children[i].leak_buf = NULL;
				}
			}
			g_leak_mode = 0;
			logmsg(C_GRN, "[CFG] ", "leak_mode=clean (buffers freed properly)\n");
		}
		else if (starts_with(line, "status"))
			status_report();
		else if (starts_with(line, "ps"))
			status_report();
		else
		{
			/* Send input to all children */
			for (int i = 0; i < g_child_count; i++)
				if (g_children[i].pid > 0 && g_children[i].pipe_in[1] >= 0)
					write(g_children[i].pipe_in[1], line, strlen(line));

			/* Drain output from all children */
			for (int i = 0; i < g_child_count; i++)
				if (g_children[i].pid > 0)
					drain_child_output(&g_children[i]);
		}

		/* Always check for zombies after each command */
		if (g_sigchld_received && g_auto_reap)
		{
			g_sigchld_received = 0;
			reap_zombies();
		}
	}

	cleanup_all();

	const char *exit_msg = C_BOLD "[demo] clean exit" C_RESET "\n";
	write(1, exit_msg, strlen(exit_msg));

	return 0;
}