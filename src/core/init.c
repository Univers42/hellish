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

/* Helper for reading file into buffer */
void	read_file_to_buffer(int fd, t_shell *state)
{
	char	ch;

	vec_append_fd(fd, &state->readline_buff.buff);
	close(fd);
	{
		ch = '\n';
		vec_push(&state->readline_buff.buff, &ch);
	}
	buff_readline_update(&state->readline_buff);
	state->readline_buff.should_update_context = true;
}

/* Helper for updating context after file read */
void	update_context_from_file(t_shell *state, char **argv)
{
	free(state->base_context);
	free(state->context);
	state->base_context = ft_strdup(argv[1]);
	state->context = ft_strdup(argv[1]);
	state->input_method = INP_FILE;
}

void	init_arg(t_shell *state, char **argv)
{
	if (!argv[2])
	{
		ft_eprintf("%s: -c: option requires an argument\n",
			state->base_context);
		free_all_state(state);
		exit(SYNTAX_ERR);
	}
	vec_push_str(&state->readline_buff.buff, argv[2]);
	buff_readline_update(&state->readline_buff);
	state->readline_buff.should_update_context = true;
	state->input_method = INP_ARG;
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
	update_context_from_file(state, argv);
}

void	init_stdin_notty(t_shell *state)
{
	state->input_method = INP_STDIN_NOTTY;
	state->readline_buff.should_update_context = true;
}
