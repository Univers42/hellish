/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   opt.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 00:27:36 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 00:27:36 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"

/* Recognise one command-line flag and OR its bit into *flags. Note that -c
   also sets OPT_FLAG_VERBOSE -- that's intentional: -c scripts should print
   what they run, matching bash -c behaviour. --debug=<subsystem> is parsed
   with a plain strncmp prefix so we don't need getopt or any state machine;
   unknown flags are silently ignored (they may be filenames for the caller
   to handle). */
static void	process_opt_flag(const char *arg, uint32_t *flags)
{
	const char	*v;

	if (ft_strcmp(arg, "-c") == 0)
		*flags |= OPT_FLAG_VERBOSE;
	else if (ft_strcmp(arg, "--help") == 0 || ft_strcmp(arg, "-h") == 0)
		*flags |= OPT_FLAG_HELP;
	else if (ft_strcmp(arg, "--verbose") == 0)
		*flags |= OPT_FLAG_VERBOSE;
	else if (ft_strcmp(arg, "--posix") == 0)
		*flags |= OPT_FLAG_POSIX;
	else if (ft_strncmp(arg, "--debug=", 8) == 0)
	{
		v = arg + 8;
		if (ft_strcmp(v, "lexer") == 0)
			*flags |= OPT_FLAG_DEBUG_LEXER;
		else if (ft_strcmp(v, "parser") == 0)
			*flags |= OPT_FLAG_DEBUG_PARSER;
		else if (ft_strcmp(v, "ast") == 0)
			*flags |= OPT_FLAG_DEBUG_AST;
	}
}

/* Count leading global option flags (e.g. --posix, --verbose) so the caller
   can shift past them before input selection. The input machinery is
   positional (-c / script live at argv[1]); without this, `hellish --posix -c
   ...` would mistake --posix for the first operand and drop to interactive
   mode. We stop at -c, at -- (end of options), and at the first non-flag. */
int	leading_opt_count(char **argv)
{
	int	i;

	i = 1;
	while (argv[i] && argv[i][0] == '-'
		&& ft_strcmp(argv[i], "-c") != 0 && ft_strcmp(argv[i], "--") != 0)
		i++;
	return (i - 1);
}

/* Walk argv[1..] and collect all recognised flags into a bitmask. We start
   at index 1 (skip argv[0]) and keep going until NULL. Non-flag arguments
   (script file, -c payload) are just left alone here; mode_input() deals
   with them. Returns 0 for the trivial "no flags" case. */
uint32_t	select_mode_from_argv(char **argv)
{
	int			i;
	uint32_t	flags;

	flags = 0;
	if (!argv || !argv[0])
		return (0);
	i = 0;
	while (argv[++i])
	{
		process_opt_flag(argv[i], &flags);
	}
	return (flags);
}
