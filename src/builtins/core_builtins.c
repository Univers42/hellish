/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   core_builtins.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:15:14 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:36:24 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* export [-p] [name[=value] ...]: mark variables for export to the environment
   of subsequent commands. Must run in the parent shell — forking would make
   the assignment invisible to the rest of the session.

   `export` alone (or `export -p`) prints the current export list in a form
   that can be fed back to the shell. When arguments are given, -p is treated
   as a signal to skip the first arg and start processing from argv[2]. Each
   argument goes through process_arg(), which handles NAME, NAME=val, and the
   `export NAME value` two-word form. Accumulates errors but keeps going — a
   single bad identifier should not stop the valid ones. */
/* Consume export's leading option words (-p/-n/-f, combos, and the bare
   "--" terminator).  Returns the index of the first operand; on an
   unrecognised option prints the error and sets *err = 2 so the builtin can
   return bash's usage status without touching the environment. */
size_t	export_skip_opts(t_shell *st, t_vec av, int *err)
{
	size_t	i;
	char	*a;

	*err = 0;
	i = 1;
	while (i < av.len)
	{
		a = ((char **)av.ctx)[i];
		if (a[0] != '-' || a[1] == '\0')
			break ;
		if (!ft_strcmp(a, "--"))
			return (i + 1);
		if (bad_opt_word(a, "pnf"))
			return (ft_eprintf("%s: export: %s: invalid option\n",
					st->ctx, a), *err = 2, i);
		i++;
	}
	return (i);
}

int	builtin_export(t_shell *st, t_vec av)
{
	size_t	i;
	int		status;
	int		idx;

	i = export_skip_opts(st, av, &status);
	if (status)
		return (2);
	if (i >= av.len)
		return (collect_and_print_exported(st), 0);
	status = 0;
	while (i < av.len)
	{
		idx = (int)i;
		status = process_arg(st, av, &idx) || status;
		i = (size_t)idx + 1;
	}
	if (status)
		return (1);
	return (0);
}

/* exit [n]: terminate the shell with status n (0–255), or with $? when n is
   omitted. In an interactive session the word "exit" is printed to stderr
   before leaving. A valid long long status is masked to 8 bits (so
   `exit 9223372036854775807` exits 255). Error handling is mode-dependent,
   matching bash exactly: under -c (INP_ARG) a bad operand EXITS the shell
   (too many => 1, non-numeric/overflow => 2); reading a script/pipe/tty it
   only prints the error and returns 2 so the shell keeps running. */
int	builtin_exit(t_shell *state, t_vec argv)
{
	long long	code;
	size_t		i;

	print_exit_if_readline(state);
	if (handle_no_args(state, argv))
		return (0);
	i = handle_double_dash(state, argv, 1);
	if (handle_non_numeric(state, argv, i, &code))
	{
		if (state->metinp == INP_ARG)
			exit_clean(state, 2);
		return (2);
	}
	if (i + 1 < argv.len)
	{
		ft_eprintf("%s: %s: too many arguments\n", state->ctx,
			((char **)argv.ctx)[0]);
		if (state->metinp == INP_ARG)
			exit_clean(state, 1);
		return (2);
	}
	return (exit_clean(state, (int)(code & 0xFF)), 0);
}

/* The ':' null utility: expand args (already done before we run) and
   succeed. Used for side effects like `: ${x:=default}` and as a true-ish. */
int	builtin_colon(t_shell *state, t_vec argv)
{
	(void)state;
	(void)argv;
	return (0);
}

/* echo [-n|-e|-E] [args ...]: print the arguments separated by spaces. The
   option parsing is intentionally lenient — any leading word that starts with
   '-' and contains only the letters n/e/E is treated as a flag word (so
   `-ne` works, but `-z` stops flag scanning and is printed literally). -n
   suppresses the trailing newline; -e enables backslash escape processing
   handled by e_parser(); -E explicitly disables it. The whole line is built
   in one buffer and emitted with a single write(2) — the previous per-arg
   ft_printf cost one syscall per argument, separator and newline. */
int	builtin_echo(t_shell *state, t_vec argv)
{
	int		n;
	int		e;
	size_t	first_arg_print;
	t_vec	out;

	n = 0;
	e = 0;
	vec_init(&out);
	out.elem_size = 1;
	first_arg_print = parse_flags(argv, &n, &e);
	if (!print_args(&out, e, argv, first_arg_print) && !n)
		vec_push_char(&out, '\n');
	if (out.len > 0 && write(STDOUT_FILENO, out.ctx, out.len) < 0)
	{
		ft_eprintf("%s: echo: write error\n", state->dft_ctx);
		xfree(out.ctx);
		return (1);
	}
	xfree(out.ctx);
	return (0);
}

