/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   opt2.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/20 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/20 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"

/* The single-letter options hellish accepts at invocation.  The `set`
   builtin's roster (src/builtins/set_opts4.c) is the authority for which
   letters exist, so command-line and runtime `set` can never disagree; l, c
   and i are invocation-only and handled by the caller. */
bool	cli_known_short(char c)
{
	if (c == '\0')
		return (false);
	if (ft_strchr("lci", c))
		return (true);
	return (setopt_find(NULL, c) != NULL);
}

/* Apply one validated set letter by reusing the `set` builtin's flag-word
   logic: a two-character word "<sign><letter>" is exactly what it expects.
   `-o` never reaches here -- cli_scan peels it off and calls cli_take_o. */
void	cli_apply_short(t_shell *state, char sign, char c)
{
	char	word[3];
	bool	want_o;

	word[0] = sign;
	word[1] = c;
	word[2] = '\0';
	want_o = false;
	(void)apply_flag_letters(state, word, &want_o);
}

/* `-o name` / `+o name`.  The `set` builtin's roster is the single list of
   valid names, so the command line and the builtin can never disagree about
   what exists; an unknown name is a usage error, as in bash. */
int	cli_apply_long(t_shell *state, char sign, const char *name)
{
	return (set_long_option(state, sign, name));
}

/* Consume the argv word at `idx` as the name for an `-o`/`+o` embedded in a
   flag cluster.  A missing or unknown name is a usage error.  Always
   reports one word consumed so the scanner advances even on error. */
int	cli_take_o(t_shell *state, t_cli *cli, char sign, int idx)
{
	char	*name;

	name = cli->argv[idx];
	if (!name || cli_apply_long(state, sign, name))
		cli->err = 2;
	return (1);
}

/* Top-level command-line parse: scan+apply every option into `state`,
   leaving `cli` describing where the operands start and whether this is a
   -c invocation.  The default `set -o` roster is seeded here rather than in
   on() because it has to be in place BEFORE cli_scan applies the user's own
   options -- seeding it later would silently undo `hellish +B` and friends.
   --posix implies opt_posix immediately so later init sees it.  The
   caller checks cli->err / OPT_FLAG_HELP, then calls cli_dispatch
   once the env and tables are ready. */
void	cli_parse(t_shell *state, char **argv, t_cli *cli)
{
	*cli = (t_cli){0};
	cli->argv = argv;
	state->setopt = SETOPT_DEFAULT;
	cli_scan(state, cli);
	if (state->option_flags & OPT_FLAG_POSIX)
		state->opt_posix = true;
}
