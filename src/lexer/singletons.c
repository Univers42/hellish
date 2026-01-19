/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   singletons.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/18 23:08:04 by marvin            #+#    #+#             */
/*   Updated: 2026/01/18 23:08:04 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "lexer.h"

t_hash	*get_color_map(void)
{
	static t_hash	map;
	static bool		ready = false;

	if (ready)
		return (&map);
	if (!hash_init(&map, 128))
		return (NULL);
	hash_set(&map, "TT_END", (void *)ASCII_BLUE);
	hash_set(&map, "TT_WORD", (void *)ASCII_GREEN);
	hash_set(&map, "TT_REDIRECT_LEFT", (void *)ASCII_YELLOW);
	hash_set(&map, "TT_REDIRECT_RIGHT", (void *)ASCII_YELLOW);
	hash_set(&map, "TT_APPEND", (void *)ASCII_YELLOW);
	hash_set(&map, "TT_HEREDOC", (void *)ASCII_YELLOW);
	hash_set(&map, "TT_PIPE", (void *)ASCII_CYAN);
	hash_set(&map, "TT_OR", (void *)ASCII_CYAN);
	hash_set(&map, "TT_AND", (void *)ASCII_CYAN);
	hash_set(&map, "TT_SEMICOLON", (void *)ASCII_MAGENTA);
	hash_set(&map, "TT_NEWLINE", (void *)ASCII_MAGENTA);
	hash_set(&map, "TT_QWORD", (void *)ASCII_GREEN);
	hash_set(&map, "TT_SQWORD", (void *)ASCII_GREEN);
	hash_set(&map, "TT_DQWORD", (void *)ASCII_GREEN);
	hash_set(&map, "TT_ENVVAR", (void *)ASCII_GREEN);
	hash_set(&map, "TT_DQENVVAR", (void *)ASCII_GREEN);
	ready = true;
	return (&map);
}

static void	init_tt_names_group1(const char **names)
{
	int	i;

	i = -1;
	while (++i < 256)
		names[i] = "TT_UNKNOWN";
	names[TT_END] = "TT_END";
	names[TT_WORD] = "TT_WORD";
	names[TT_REDIRECT_LEFT] = "TT_REDIRECT_LEFT";
	names[TT_REDIRECT_RIGHT] = "TT_REDIRECT_RIGHT";
	names[TT_APPEND] = "TT_APPEND";
	names[TT_PIPE] = "TT_PIPE";
	names[TT_BRACE_LEFT] = "TT_BRACE_LEFT";
	names[TT_BRACE_RIGHT] = "TT_BRACE_RIGHT";
}

static void	init_tt_names_group2(const char **names)
{
	names[TT_OR] = "TT_OR";
	names[TT_AND] = "TT_AND";
	names[TT_SEMICOLON] = "TT_SEMICOLON";
	names[TT_HEREDOC] = "TT_HEREDOC";
	names[TT_NEWLINE] = "TT_NEWLINE";
	names[TT_SQWORD] = "TT_SQWORD";
	names[TT_DQWORD] = "TT_DQWORD";
	names[TT_ENVVAR] = "TT_ENVVAR";
	names[TT_DQENVVAR] = "TT_DQENVVAR";
}

static void	init_tt_names_group3(const char **names)
{
	names[TT_AMPERSAND] = "TT_AMPERSAND";
	names[TT_ARITH_START] = "TT_ARITH_START";
	names[TT_PROC_SUB_IN] = "TT_PROC_SUB_IN";
	names[TT_PROC_SUB_OUT] = "TT_PROC_SUB_OUT";
}

const char	**get_tt_names(void)
{
	static const char	*names[256];
	static int			inited = 0;

	if (!inited)
	{
		init_tt_names_group1(names);
		init_tt_names_group2(names);
		init_tt_names_group3(names);
		inited = 1;
	}
	return (names);
}
