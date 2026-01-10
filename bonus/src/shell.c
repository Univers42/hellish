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

#include "libft/dsa/dyn_str.h"

int	main(int argc, char **argv, char **envp)
{
	t_state	state;

	(void)argc;
	init_setup(&state, argv, envp);
	state.redirects.elem_size = sizeof(t_redir);
	while (!state.should_exit)
	{
		vec_init(&state.input);
		g_should_unwind = 0;
		parse_and_execute_input(&state);
		free_redirects(&state.redirects);
		free_ast(&state.tree);
		free(state.input.ctx);
		state.input = (t_string){};
	}
	free_env(&state.env);
	free_all_state(&state);
	forward_exit_status(state.last_cmd_status_res);
}
