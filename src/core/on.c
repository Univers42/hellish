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

/* --help: dump the flag reference, free everything, and exit cleanly.
   Calling free_all_state first means valgrind/ASan stay quiet even when
   --help is the only thing the user asked for. */
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

/* Decide how we read commands. Priority: explicit -c string > script file >
   non-tty stdin (pipe) > interactive readline. The argv[1][0] != '-' check
   is intentionally broad: any first argument that is not a flag is taken as
   a file name; unknown flags fall through to interactive mode too. */
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

/* Zero-initialise every table that survives across commands: redirect list,
   process-sub tracking, shell-function list, job table, alias table, and the
   command-hash cache. All use a consistent "zero-length vec" starting point
   so any early-exit path can safely call free/cleanup on them. */
static void	init_tables(t_shell *state)
{
	vec_init(&state->redirects);
	state->redirects.elem_size = sizeof(t_redir);
	vec_init(&state->proc_subs);
	state->proc_subs.elem_size = sizeof(t_procsub_entry);
	vec_init(&state->functions);
	state->functions.elem_size = sizeof(t_shell_func);
	job_table_init(&state->job_table);
	alias_table_init(&state->aliases);
	cmd_hash_init(&state->cmd_cache);
}

/* The name the shell reports in diagnostics: the basename of argv[0]
   (bash uses "bash", not the full path it was launched from). A script
   overrides this later with the script name. */
static char	*shell_basename(char *arg0)
{
	char	*slash;

	slash = ft_strrchr(arg0, '/');
	if (slash && slash[1])
		return (slash + 1);
	return (arg0);
}

/* Bootstrap the whole shell. Order matters: signals first (Ctrl-C must be
   safe the moment we start reading), then env (commands may read it during
   init), then tables (used by init_history / mode_input), then the input
   source. The PRNG seed (Knuth constant) is fixed so $RANDOM is repeatable
   across machines for test scripts. bg_job_count starts at 0 -- it is the
   counter for the unique job IDs we hand out. */
void	on(t_shell *state, char **argv, char **envp)
{
	set_unwind_sig();
	*state = shell_init();
	state->option_flags = select_mode_from_argv(argv);
	if (state->option_flags & OPT_FLAG_POSIX)
		state->opt_posix = true;
	if (state->option_flags & OPT_FLAG_HELP)
		print_opts(argv, state);
	buff_readline_init(&state->rl);
	vec_init(&state->rl.buff);
	state->rl.buff.elem_size = 1;
	state->rl.edit_mode = 1;
	state->pid = xgetpid();
	state->ctx = ft_strdup(shell_basename(argv[0]));
	state->dft_ctx = ft_strdup(shell_basename(argv[0]));
	set_cmd_status(state, res_status(0));
	state->last_cmd_st_exe = res_status(0);
	init_cwd(state);
	state->env = env_to_vec_env(state, envp);
	ensure_essential_env_vars(state);
	init_tables(state);
	state->edit_mode = 1;
	mode_input(argv + leading_opt_count(argv), state);
	prng_initialize_state(&state->prng, 19650218UL);
	state->bg_job_count = 0;
}
