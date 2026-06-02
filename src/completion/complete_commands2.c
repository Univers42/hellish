/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_commands2.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "completion_private.h"
#include "libft.h"
#include <readline/readline.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

static char	*cmd_generator(const char *text, int state_gen)
{
	static char	**path_dirs;
	static int	dir_idx;
	size_t		tlen;
	char		*name;

	tlen = ft_strlen(text);
	if (!state_gen)
		cmd_gen_init(&path_dirs, &dir_idx);
	while (g_builtins[g_cmd_idx])
	{
		name = g_builtins[g_cmd_idx++];
		if (ft_strncmp(name, text, tlen) == 0)
			return (ft_strdup(name));
	}
	return (cmd_gen_dirs(&path_dirs, &dir_idx, tlen, text));
}

char	**complete_commands(const char *text, int start, int end)
{
	(void)start;
	(void)end;
	rl_completion_append_character = ' ';
	return (rl_completion_matches(text, cmd_generator));
}
