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
#include "update.h"
#include "shell.h"
#include "pal.h"
#include "helpers.h"
#include "env.h"
#include <string.h>
#include <time.h>
#include "lexer.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

static char	*shell_basename(char *arg0);

/* The two invocations that never reach the REPL, checked in the order the
   old inline pair used: --help wins and dumps the flag reference, exit 0;
   otherwise an unrecognised option (e.g. `hellish -z`, `hellish -c -q`)
   aborts startup with usage status 2 like bash and dash, naming the shell
   by argv[0]'s basename. Both paths free_all_state first so valgrind/ASan
   stay quiet; state is still mostly zero here, which free_all_state
   tolerates. */
static void	cli_early_exit(t_shell *state, char **argv, t_cli *cli)
{
	if (state->option_flags & OPT_FLAG_VERSION)
	{
		print_version();
		free_all_state(state);
		exit(0);
	}
	if (!(state->option_flags & OPT_FLAG_HELP))
	{
		if (!cli->err)
			return ;
		ft_eprintf("%s: invalid option\n", shell_basename(argv[0]));
		free_all_state(state);
		exit(2);
	}
	ft_printf("Usage: %s [options] [file]\n", argv[0]);
	ft_printf("  --help           Show this help\n");
	ft_printf("  --version        Print the version and exit\n");
	ft_printf("  -c <script>      Execute script string\n");
	ft_printf("  --verbose        Verbose mode\n");
	ft_printf("  --posix          POSIX mode (disable non-POSIX extensions)\n");
	ft_printf("  --debug=lexer    Debug lexer only\n");
	ft_printf("  --debug=parser   Debug parser only\n");
	ft_printf("  --debug=ast      Debug AST only\n");
	free_all_state(state);
	exit(0);
}

/* Zero-initialise every table that survives across commands: redirect list,
   process-sub tracking, shell-function list, job table, alias table, and the
   command-hash cache. All use a consistent "zero-length vec" starting point
   so any early-exit path can safely call free/cleanup on them. The line
   editor's default mode rides along: it is the same kind of "state that
   outlives one command", and on() is at its 25-line ceiling. So does
   jc_init, which records the pid/group the job-control code compares
   against for the rest of the session. */
static void	init_tables(t_shell *state)
{
	jc_init(state);
	vec_init(&state->redirects);
	state->redirects.elem_size = sizeof(t_redir);
	vec_init(&state->proc_subs);
	state->proc_subs.elem_size = sizeof(t_procsub_entry);
	vec_init(&state->functions);
	state->functions.elem_size = sizeof(t_shell_func);
	vec_init(&state->dead_funcs);
	state->dead_funcs.elem_size = sizeof(t_ast_node);
	job_table_init(&state->job_table);
	alias_table_init(&state->aliases);
	cmd_hash_init(&state->cmd_cache);
	state->edit_mode = 1;
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

/* Readline-buffer defaults that are non-zero, so shell_init()'s zeroing
   cannot provide them (the on.c contract): a byte-granular input buffer
   and edit_mode 1, the emacs keymap. */
static void	init_rl_buffer(t_shell *state)
{
	buff_readline_init(&state->rl);
	vec_init(&state->rl.buff);
	state->rl.buff.elem_size = 1;
	state->rl.edit_mode = 1;
}

/* Bootstrap the whole shell. Order matters: signals first (Ctrl-C must be
   safe the moment we start reading), then env (commands may read it during
   init), then tables (used by init_history / mode_input), then the input
   source. The PRNG is seeded from pid^time like bash seeds $RANDOM: two
   shells started in the same second must not share a sequence (that also
   de-collides the PRNG-named heredoc tmp files). bg_job_count starts at
   0 -- it is the counter for the unique job IDs we hand out.

   $0 defaults to argv[0] verbatim, exactly as bash does -- not the basename
   used for diagnostics. It has to be seeded here, before cli_dispatch(),
   because only two of the four input modes assign it themselves: a script
   overrides it with its path and `-c cmd NAME` with NAME. Leaving it to
   those two left $0 empty for plain `-c` and for piped stdin (issue #14). */
void	on(t_shell *state, char **argv, char **envp)
{
	t_cli	cli;

	set_unwind_sig();
	*state = shell_init();
	state->shopt = SHOPT_CHECKWINSIZE;
	cli_parse(state, argv, &cli);
	cli_early_exit(state, argv, &cli);
	init_rl_buffer(state);
	state->pid = ft_itoa((int)getpid());
	state->ctx = ft_strdup(shell_basename(argv[0]));
	state->dft_ctx = ft_strdup(shell_basename(argv[0]));
	set_cmd_status(state, res_status(0));
	state->last_cmd_st_exe = res_status(0);
	init_cwd(state);
	state->env = env_to_vec_env(state, envp);
	ensure_essential_env_vars(state);
	env_set(&state->env, env_create(ft_strdup("0"),
			ft_strdup(argv[0]), false));
	init_tables(state);
	cli_dispatch(state, &cli);
	interactive_job_signals(state->metinp == INP_RL);
	prng_initialize_state(&state->prng,
		(uint32_t)(getpid() * 2654435761u ^ time(NULL)));
	state->start_sec = (long long)time(NULL);
	state->bg_job_count = 0;
}
