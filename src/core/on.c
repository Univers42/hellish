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
#include <time.h>
#include "lexer.h"
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

static char	*shell_basename(char *arg0);

/* --help: dump the flag reference, free everything, and exit cleanly.
   Calling free_all_state first means valgrind/ASan stay quiet even when
   --help is the only thing the user asked for. */
static void	print_opts(char **argv, t_shell *state)
{
	ft_printf("Usage: %s [options] [file]\n", argv[0]);
	ft_printf("  --help           Show this help\n");
	ft_printf("  -c <script>      Execute script string\n");
	ft_printf("  --verbose        Verbose mode\n");
	ft_printf("  --posix          POSIX mode (disable non-POSIX extensions)\n");
	ft_printf("  --debug=lexer    Debug lexer only\n");
	ft_printf("  --debug=parser   Debug parser only\n");
	ft_printf("  --debug=ast      Debug AST only\n");
	free_all_state(state);
	exit(0);
}

/* An unrecognised invocation option (e.g. `hellish -z`, `hellish -c -q`):
   bash and dash both abort startup with usage status 2.  argv[0]'s basename
   names the shell in the diagnostic; state is still mostly zero here, which
   free_all_state tolerates. */
static void	cli_usage_error(t_shell *state, char **argv)
{
	ft_eprintf("%s: invalid option\n", shell_basename(argv[0]));
	free_all_state(state);
	exit(2);
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
   source. The PRNG is seeded from pid^time like bash seeds $RANDOM: two
   shells started in the same second must not share a sequence (that also
   de-collides the PRNG-named heredoc tmp files). bg_job_count starts at
   0 -- it is the counter for the unique job IDs we hand out. */
void	on(t_shell *state, char **argv, char **envp)
{
	t_cli	cli;

	set_unwind_sig();
	*state = shell_init();
	cli_parse(state, argv, &cli);
	if (state->option_flags & OPT_FLAG_HELP)
		print_opts(argv, state);
	if (cli.err)
		cli_usage_error(state, argv);
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
	cli_dispatch(state, &cli);
	prng_initialize_state(&state->prng,
		(uint32_t)(getpid() * 2654435761u ^ time(NULL)));
	state->start_sec = (long long)time(NULL);
	state->bg_job_count = 0;
}
