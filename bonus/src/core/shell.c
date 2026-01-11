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
#include <stdlib.h>

# include "shell.h"

// TODO: need to split this function to make it clear the boundaries..

static void repl_shell(t_shell *state);
static void off(t_shell *state);

/**
 * no return needed as we forward with the exit status
 */
int	main(int argc, char **argv, char **envp)
{
	t_shell	state;

	(void)argc;
	on(&state, argv, envp);
	repl_shell(&state);
	off(&state);
	return (0);
}

static void repl_shell(t_shell *state)
{
	while (!state->should_exit)
	{
		vec_init(&state->input);
		state->input.elem_size = 1;
		g_should_unwind = 0;
		parse_and_execute_input(state);
		free_redirects(&state->redirects);
		free_ast(&state->tree);
		free(state->input.ctx);
		state->input = (t_string){0};
	}
}

static void	off(t_shell *state)
{
	/* free environment vector, then other state resources and forward exit */
	free_env(&state->env);
	free_all_state(state);
	forward_exit_status(state->last_cmd_status_res);
}
