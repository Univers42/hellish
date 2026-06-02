/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:03 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:03 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "helpers.h"
#include "env.h"
#include "sh_input.h"
#include "job_control.h"
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void		on(t_shell *state, char **argv, char **envp);
void		run_pending_traps(t_shell *state);
void		run_exit_trap(t_shell *state);
int			exec_string(t_shell *state, char *str);
char		*env_expand(t_shell *state, char *key);
static int	setup_output_buffer(t_shell *state, int *bak);
static void	flush_output_buffer(int buf_fd, int bak);
static void	repl_shell(t_shell *state);
static void	off(t_shell *state);

/* Slurp a whole file into a freshly-allocated NUL-terminated string, or NULL. */
static char	*read_file(const char *path)
{
	char		buf[4096];
	t_string	content;
	int			fd;
	ssize_t		n;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (NULL);
	vec_init(&content);
	content.elem_size = 1;
	n = read(fd, buf, sizeof(buf));
	while (n > 0)
	{
		vec_push_nstr(&content, buf, n);
		n = read(fd, buf, sizeof(buf));
	}
	close(fd);
	vec_push(&content, &(char){0});
	return ((char *)content.ctx);
}

/* On interactive startup, run the user's ~/.hellishrc (aliases, exports,
   functions, set options, prompt tweaks) in the current shell -- our own
   rc-file convention, the .bashrc analogue. Silently skipped if absent or
   non-interactive (-c / scripts / piped input do not source it). */
static void	source_hellishrc(t_shell *state)
{
	char	*home;
	char	*path;
	char	*content;

	if (state->metinp != INP_RL)
		return ;
	home = env_expand(state, "HOME");
	if (!home || !*home)
		return ;
	path = ft_strjoin(home, "/.hellishrc");
	if (!path)
		return ;
	content = read_file(path);
	free(path);
	if (!content)
		return ;
	exec_string(state, content);
	free(content);
}

/**
 * no return needed as we forward with the exit status
 */
int	main(int argc, char **argv, char **envp)
{
	t_shell	state;
	bool	is_login_shell;

	is_login_shell = (argv[0] && argv[0][0] == '-');
	if (is_login_shell)
		argv[0]++;
	(void)argc;
	on(&state, argv, envp);
	source_hellishrc(&state);
	repl_shell(&state);
	off(&state);
}

/* Output buffering (stdout -> temp file dumped at exit) is disabled: it lost
   output when the shell process was replaced (exec) or exited directly (exit),
   and serves no purpose now that prompts are gated on interactivity. */
static int	setup_output_buffer(t_shell *state, int *bak)
{
	(void)state;
	*bak = -1;
	return (-1);
}

static void	flush_output_buffer(int buf_fd, int bak)
{
	char	tmp[4096];
	ssize_t	n;

	if (buf_fd < 0)
		return ;
	dup2(bak, STDOUT_FILENO);
	close(bak);
	lseek(buf_fd, 0, SEEK_SET);
	n = 1;
	while (n > 0)
	{
		n = read(buf_fd, tmp, sizeof(tmp));
		if (n > 0 && write(STDOUT_FILENO, tmp, n) < 0)
			break ;
	}
	close(buf_fd);
}

static void	repl_shell(t_shell *state)
{
	int	buf_fd;
	int	stdout_bak;

	buf_fd = setup_output_buffer(state, &stdout_bak);
	while (!state->should_exit)
	{
		vec_init(&state->input);
		state->input.elem_size = 1;
		get_g_sig()->should_unwind = 0;
		job_notify(state);
		parse_and_execute_input(state);
		run_pending_traps(state);
		free_redirects(&state->redirects);
		free_ast(&state->tree);
		free(state->input.ctx);
		state->input = (t_string){0};
		free(state->hd_src);
		state->hd_src = NULL;
		state->hd_pos = 0;
		free(state->hd_stripped);
		state->hd_stripped = NULL;
	}
	flush_output_buffer(buf_fd, stdout_bak);
}

static void	off(t_shell *state)
{
	run_exit_trap(state);
	free_env(&state->env);
	free_all_state(state);
	forward_exit_status(state->last_cmd_st_exe);
}
