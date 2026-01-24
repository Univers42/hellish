/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   on.c                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/17 23:38:13 by marvin            #+#    #+#             */
/*   Updated: 2026/01/17 23:38:13 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"
#include "shell.h"
#include "helpers.h"
#include "env.h"
#include <string.h>
#include "lexer.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

static void	print_opts(char **argv, t_shell *state)
{
	ft_printf("Usage: %s [options] [file]\n", argv[0]);
	ft_printf("  --help           Show this help\n");
	ft_printf("  -c <script>      Execute script string\n");
	ft_printf("  --verbose        Verbose mode\n");
	ft_printf("  --debug=lexer    Debug lexer only\n");
	ft_printf("  --debug=parser   Debug parser only\n");
	ft_printf("  --debug=ast      Debug AST only\n");
	free_all_state(state);
	exit(0);
}

static void	mode_input(char **argv, t_shell *state)
{
	if (argv[1] && ft_strcmp(argv[1], "-c") == 0)
		init_arg(state, argv);
	else if (argv[1] && argv[1][0] != '-')
		init_file(state, argv);
	else if (!isatty(0))
		init_stdin_notty(state);
	else
		init_history(state);
}

void	on(t_shell *state, char **argv, char **envp)
{
	set_unwind_sig();
	*state = shell_init();
	state->option_flags = select_mode_from_argv(argv);
	if (state->option_flags & OPT_FLAG_HELP)
		print_opts(argv, state);
	buff_readline_init(&state->readline_buff);
	vec_init(&state->readline_buff.buff);
	state->readline_buff.buff.elem_size = 1;
	state->pid = xgetpid();
	state->context = ft_strdup(argv[0]);
	state->base_context = ft_strdup(argv[0]);
	set_cmd_status(state, res_status(0));
	state->last_cmd_status_res = res_status(0);
	init_cwd(state);
	state->env = env_to_vec_env(state, envp);
	ensure_essential_env_vars(state);
	vec_init(&state->redirects);
	state->redirects.elem_size = sizeof(t_redir);
	vec_init(&state->proc_subs);
	state->proc_subs.elem_size = sizeof(t_procsub_entry);
	mode_input(argv, state);
	prng_initialize_state(&state->prng, 19650218UL);
	state->bg_job_count = 0;
}
