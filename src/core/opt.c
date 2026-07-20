/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   opt.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 00:27:36 by marvin            #+#    #+#             */
/*   Updated: 2026/07/20 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"

/* A "--name" startup option: --posix, --verbose, --help, --debug=<x>.  Any
   other "--" word is unrecognised and, like bash and dash, aborts startup
   with usage status 2 (recorded in cli->err). */
void	cli_long_word(t_shell *state, t_cli *cli, const char *w)
{
	if (!ft_strcmp(w, "--posix"))
		state->option_flags |= OPT_FLAG_POSIX;
	else if (!ft_strcmp(w, "--verbose"))
		state->option_flags |= OPT_FLAG_VERBOSE;
	else if (!ft_strcmp(w, "--help"))
		state->option_flags |= OPT_FLAG_HELP;
	else if (ft_strncmp(w, "--debug=", 8) == 0 && !ft_strcmp(w + 8, "lexer"))
		state->option_flags |= OPT_FLAG_DEBUG_LEXER;
	else if (ft_strncmp(w, "--debug=", 8) == 0 && !ft_strcmp(w + 8, "parser"))
		state->option_flags |= OPT_FLAG_DEBUG_PARSER;
	else if (ft_strncmp(w, "--debug=", 8) == 0 && !ft_strcmp(w + 8, "ast"))
		state->option_flags |= OPT_FLAG_DEBUG_AST;
	else if (!ft_strcmp(w, "--login"))
		return ;
	else
		cli->err = 2;
}

/* One short-option word (-eux, +o, -oo, -c ...).  Letters map to set
   options; 'c'/'i' are the invocation flags; each 'o' consumes the next
   argv word as an `-o name` long option (so `-oo a b` sets two).  Unknown
   letter => usage error.  Returns the count of EXTRA argv words consumed
   by embedded 'o's so the scanner can skip past them. */
int	cli_opt_word(t_shell *state, t_cli *cli, const char *w)
{
	char	sign;
	int		j;
	int		extra;

	sign = w[0];
	j = 0;
	extra = 0;
	while (w[++j] && !cli->err)
	{
		if (w[j] == 'c')
			cli->cmode = true;
		else if (w[j] == 'i')
			cli->inter = true;
		else if (w[j] == 'o')
			extra += cli_take_o(state, cli, sign, cli->i + 1 + extra);
		else if (cli_known_short(w[j]))
			cli_apply_short(state, sign, w[j]);
		else
			cli->err = 2;
	}
	return (extra);
}

/* A lone "-" ends option processing and (like `set -`) turns off -x/-v. */
void	cli_lone_dash(t_shell *state, t_cli *cli)
{
	state->opt_xtrace = false;
	state->opt_verbose = false;
	cli->i++;
}

/* Walk argv[1..] applying options until an operand is reached.  "--" ends
   options; "-" ends them too (via cli_lone_dash).  On return cli->i indexes
   the first operand (the -c string, the script file, or past the end). */
void	cli_scan(t_shell *state, t_cli *cli)
{
	char	*w;

	cli->i = 1;
	while (cli->argv[cli->i] && !cli->err)
	{
		w = cli->argv[cli->i];
		if (!ft_strcmp(w, "--"))
			return ((void)cli->i++);
		if (!ft_strcmp(w, "-"))
			return (cli_lone_dash(state, cli));
		if (w[0] == '-' && w[1] == '-')
			cli_long_word(state, cli, w);
		else if ((w[0] == '-' || w[0] == '+') && w[1])
			cli->i += cli_opt_word(state, cli, w);
		else
			return ;
		cli->i++;
	}
}

/* Route to the input source now that options are resolved.  The argv
   pointer is biased so the offset-based init_arg/init_file see the command
   string / script exactly where they expect it (argv[2] / argv[1]),
   regardless of how many option words preceded it. */
void	cli_dispatch(t_shell *state, t_cli *cli)
{
	int	op;

	op = cli->i;
	if (cli->inter)
		state->opt_interactive = true;
	if (cli->cmode)
		init_arg(state, cli->argv + (op - 2));
	else if (cli->argv[op])
		init_file(state, cli->argv + (op - 1));
	else if (!isatty(0))
		init_stdin_notty(state);
	else
		init_history(state);
}
