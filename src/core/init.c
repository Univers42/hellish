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

/* Wire up $0 and $1.. $N from argv[base..]. Called in two places: -c mode
   where base=4 and the optional command-name argv[3] becomes $0, and script
   mode where base=2 and argv[1] (the script path) becomes $0. Both paths
   converge here so positional-arg handling stays in exactly one place. */
static void	set_argv_params(t_shell *state, char **argv, int base, char *zero)
{
	int	n;

	n = 0;
	while (argv[base + n])
		n++;
	set_positional_args(state, argv + base, (size_t)n);
	env_set(&state->env, env_create(ft_strdup("0"), ft_strdup(zero), false));
}

/* Slurp the whole file fd into the readline buffer so the non-interactive
   code-path can reuse the same parse loop that handles interactive input.
   The trailing '\n' push is the classic trick: the last line of many scripts
   has no newline, and without it the parser stalls waiting for more input.
   buff_readline_update() then builds the cursor-relative view the tokenizer
   expects. Caller is responsible for the open(); we take ownership of fd. */
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

/* Point ctx and dft_ctx at the script filename. ctx is what shows up in
   error messages ("script.sh: line 3: ...") while dft_ctx is the fallback
   used before any script name is set. Switching metinp to INP_FILE tells the
   REPL not to prompt and not to source ~/.hellishrc.
     The `.zsh` extension arms the dialect here too. It is the same rule
   `source` and the rc loader already applied (zsh_path), and this was the one
   way into the shell that never asked: `source plugin.zsh` got zsh and
   `hellish plugin.zsh` got bash, silently, for the same bytes. There is no
   third dialect for "the file arrived as an argument". */
void	update_ctx_from_file(t_shell *state, char **argv)
{
	xfree(state->dft_ctx);
	xfree(state->ctx);
	state->dft_ctx = ft_strdup(argv[1]);
	state->ctx = ft_strdup(argv[1]);
	state->metinp = INP_FILE;
	if (zsh_path(argv[1]))
		zsh_mode_swap(state, true);
}

/* -c mode: push argv[2] (the inline script string) into the readline buffer
   and mark it as INP_ARG so the REPL runs exactly once then exits. no_compact
   suppresses the multiline-continuation join because -c input is already a
   complete command. An optional argv[3] becomes $0 (bash compat), and
   argv[4..] become $1..$N. Missing argv[2] is a fatal usage error. */
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
	state->rl.no_compact = true;
	if (argv[3])
		set_argv_params(state, argv, 4, argv[3]);
}

/* Script-file mode: open argv[1] and read it into the readline buffer. We
   separate the open+error step from the buffer-fill step so handle_file_open_
   error() can do the right exit code without the buffer state being half-set.
   no_compact mirrors -c mode (the file is already the full source). */
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
	state->rl.no_compact = true;
	set_argv_params(state, argv, 2, argv[1]);
}
