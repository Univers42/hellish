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
#include <string.h>
 #include "lexer.h"
 #include <fcntl.h>
 #include <errno.h>
 #include <unistd.h>
 
 /* Parse argv early to set option flags without performing heavy initialization. */
 static uint32_t select_mode_from_argv(char **argv)
 {
     uint32_t flags = 0;
     if (!argv || !argv[0])
         return 0;
     for (int i = 1; argv[i]; ++i)
     {
         if (ft_strcmp(argv[i], "-c") == 0)
             flags |= OPT_FLAG_VERBOSE; /* treat -c as request to run provided command */
         else if (ft_strcmp(argv[i], "--help") == 0 || ft_strcmp(argv[i], "-h") == 0)
             flags |= OPT_FLAG_HELP;
         else if (ft_strcmp(argv[i], "--verbose") == 0)
             flags |= OPT_FLAG_VERBOSE;
         else if (ft_strncmp(argv[i], "--debug=", 8) == 0)
         {
             char *v = argv[i] + 8;
             if (ft_strcmp(v, "lexer") == 0)
                 flags |= OPT_FLAG_DEBUG_LEXER;
             else if (ft_strcmp(v, "parser") == 0)
                 flags |= OPT_FLAG_DEBUG_PARSER;
             else if (ft_strcmp(v, "ast") == 0)
                 flags |= OPT_FLAG_DEBUG_AST;
         }
     }
     return flags;
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
    /* parse argv to detect requested runtime mode before heavy init */
    state->option_flags = select_mode_from_argv(argv);
     /* If only help requested, print help and exit early (no further init) */
    if (state->option_flags & OPT_FLAG_HELP)
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
     /* initialize readline buffer structure and its inner string buffer */
	buff_readline_init(&state->readline_buff);
	vec_init(&state->readline_buff.buff);
	state->readline_buff.buff.elem_size = 1;
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
	else if (argv[1] && argv[1][0] != '-')
         init_file(state, argv);
     else if (!isatty(0))
         init_stdin_notty(state);
     else
         init_history(state);
     prng_initialize_state(&state->prng, 19650218UL);
     state->redirects.elem_size = sizeof(t_redir);
}
