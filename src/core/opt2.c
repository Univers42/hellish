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

/* The single-letter options hellish accepts at invocation.  e/u/x/f/C/a/n/v
   map to set options; b/m/h/H are accepted-and-ignored (bash's notify /
   monitor / hashall / histexpand) so real-world `#!/bin/sh -h` shebangs and
   the like don't abort.  c and i are handled by the caller. */
bool	cli_known_short(char c)
{
	return (c != '\0' && ft_strchr("euxfCanvbmhHl", c) != NULL);
}

/* Apply one validated set letter by reusing the `set` builtin's flag-word
   logic: a two-character word "<sign><letter>" is exactly what it expects. */
void	cli_apply_short(t_shell *state, char sign, char c)
{
	char	word[3];

	word[0] = sign;
	word[1] = c;
	word[2] = '\0';
	(void)apply_flag_word(state, word);
}

/* `-o name` / `+o name`.  Only the long option names the `set` builtin
   actually implements are accepted; anything else is a usage error so that
   `-o bogus` fails like bash instead of being silently swallowed. */
int	cli_apply_long(t_shell *state, char sign, const char *name)
{
	static const char	*const	ok[] = {"errexit", "nounset", "xtrace",
		"noglob", "noclobber", "allexport", "noexec", "verbose", "pipefail",
		"vi", "emacs", NULL};
	int							i;

	i = 0;
	while (ok[i])
	{
		if (!ft_strcmp(ok[i], name))
			return (set_long_option(state, sign, name), 0);
		i++;
	}
	return (2);
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
   -c invocation.  --posix implies opt_posix immediately so later init sees
   it.  The caller checks cli->err / OPT_FLAG_HELP, then calls cli_dispatch
   once the env and tables are ready. */
void	cli_parse(t_shell *state, char **argv, t_cli *cli)
{
	*cli = (t_cli){0};
	cli->argv = argv;
	cli_scan(state, cli);
	if (state->option_flags & OPT_FLAG_POSIX)
		state->opt_posix = true;
}
