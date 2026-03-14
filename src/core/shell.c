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
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void		on(t_shell *state, char **argv, char **envp);
static int	setup_output_buffer(t_shell *state, int *bak);
static void	flush_output_buffer(int buf_fd, int bak);
static void	repl_shell(t_shell *state);
static void	off(t_shell *state);

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
	repl_shell(&state);
	off(&state);
}

static int	setup_output_buffer(t_shell *state, int *bak)
{
	int		fd;
	char	path[32];

	*bak = -1;
	if (state->metinp == INP_RL)
		return (-1);
	ft_strlcpy(path, "/tmp/.hellish_XXXXXX", 32);
	fd = mkstemp(path);
	if (fd < 0)
		return (-1);
	unlink(path);
	*bak = dup(STDOUT_FILENO);
	dup2(fd, STDOUT_FILENO);
	return (fd);
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
		if (n > 0)
			write(STDOUT_FILENO, tmp, n);
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
		parse_and_execute_input(state);
		free_redirects(&state->redirects);
		free_ast(&state->tree);
		free(state->input.ctx);
		state->input = (t_string){0};
	}
	flush_output_buffer(buf_fd, stdout_bak);
}

static void	off(t_shell *state)
{
	free_env(&state->env);
	free_all_state(state);
	forward_exit_status(state->last_cmd_st_exe);
}
