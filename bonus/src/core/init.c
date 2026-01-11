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

#include "shell.h"
# include "helpers.h"
# include "input.h"
#include "env.h"

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
		ft_eprintf("%s: %s: %s\n", state->base_context,
			argv[1], strerror(errno));
		free_all_state(state);
		if (errno == EISDIR)
			exit(127);
		exit(EXE_BASE_ERROR + errno);
	}
	vec_append_fd(fd, &state->readline_buff.buff);
	close(fd);
	{ char ch = '\n'; vec_push(&state->readline_buff.buff, &ch); }
	buff_readline_update(&state->readline_buff);
	state->readline_buff.should_update_context = true;
	free(state->base_context);
	free(state->context);
	state->base_context = ft_strdup(argv[1]);
	state->context = ft_strdup(argv[1]);
	state->input_method = INP_FILE;
}

void	init_stdin_notty(t_shell *state)
{
	state->input_method = INP_STDIN_NOTTY;
	state->readline_buff.should_update_context = true;
}

void	init_cwd(t_shell *state)
{
	char	*cwd;

	vec_init(&state->cwd);
	cwd = getcwd(NULL, 0);
	if (cwd)
	{
		vec_push_str(&state->cwd, cwd);
	}
	else
	{
		ft_eprintf("shell-init: error retrieving current directory:"
			" getcwd: cannot access parent directories:"
			" No such file or directory\nsh: 0: "
			"getcwd() failed: No such file or directory\n");
	}
	free(cwd);
}

void	on(t_shell *state, char **argv, char **envp)
{
	set_unwind_sig();
	*state = (t_shell){0};
	state->pid = getpid_hack();
	state->context = ft_strdup(argv[0]);
	state->base_context = ft_strdup(argv[0]);
	set_cmd_status(state, res_status(0));
	state->last_cmd_status_res = res_status(0);
	init_cwd(state);
	state->env = env_to_vec_env(state, envp);
	/* initialize redirects vector so later vec_push/vec_idx are safe */
	vec_init(&state->redirects);
	state->redirects.elem_size = sizeof(t_redir);
	if (argv[1] && ft_strcmp(argv[1], "-c") == 0)
		init_arg(state, argv);
	else if (argv[1])
		init_file(state, argv);
	else if (!isatty(0))
		init_stdin_notty(state);
	else
		init_history(state);
	prng_initialize_state(&state->prng, 19650218UL);
	state->redirects.elem_size = sizeof(t_redir);
}
