/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:51 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:51 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"
#include "env.h"
#include "ft_builtins.h"

/* $0 + positional params $1.. from argv[base..] ($0 = `zero`). Lets a script
   (or -c command_name args) see its arguments, per POSIX. */
static void	set_argv_params(t_shell *state, char **argv, int base, char *zero)
{
	int	n;

	n = 0;
	while (argv[base + n])
		n++;
	set_positional_args(state, argv + base, (size_t)n);
	env_set(&state->env, env_create(ft_strdup("0"), ft_strdup(zero), false));
}

/* Helper for reading file into buffer */
void	read_file_to_buffer(int fd, t_shell *state)
{
	char	ch;

	vec_append_fd(fd, &state->rl.buff);
	close(fd);
	{
		ch = '\n';
		vec_push(&state->rl.buff, &ch);
	}
	buff_readline_update(&state->rl);
	state->rl.should_update_ctx = true;
}

/* Helper for updating ctx after file read */
void	update_ctx_from_file(t_shell *state, char **argv)
{
	free(state->dft_ctx);
	free(state->ctx);
	state->dft_ctx = ft_strdup(argv[1]);
	state->ctx = ft_strdup(argv[1]);
	state->metinp = INP_FILE;
}

void	init_arg(t_shell *state, char **argv)
{
	if (!argv[2])
	{
		ft_eprintf("%s: -c: option requires an argument\n",
			state->dft_ctx);
		free_all_state(state);
		exit(SYNTAX_ERR);
	}
	vec_push_str(&state->rl.buff, argv[2]);
	buff_readline_update(&state->rl);
	state->rl.should_update_ctx = true;
	state->metinp = INP_ARG;
	if (argv[3])
		set_argv_params(state, argv, 4, argv[3]);
}

void	init_file(t_shell *state, char **argv)
{
	int	fd;

	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
	{
		handle_file_open_error(state, argv);
		return ;
	}
	read_file_to_buffer(fd, state);
	update_ctx_from_file(state, argv);
	set_argv_params(state, argv, 2, argv[1]);
}

void	init_stdin_notty(t_shell *state)
{
	state->metinp = INP_NOTTY;
	state->rl.should_update_ctx = true;
}
