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

static void	process_opt_flag(const char *arg, uint32_t *flags)
{
	const char	*v;

	if (ft_strcmp(arg, "-c") == 0)
		*flags |= OPT_FLAG_VERBOSE;
	else if (ft_strcmp(arg, "--help") == 0 || ft_strcmp(arg, "-h") == 0)
		*flags |= OPT_FLAG_HELP;
	else if (ft_strcmp(arg, "--verbose") == 0)
		*flags |= OPT_FLAG_VERBOSE;
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
