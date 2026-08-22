/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_commands2.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 13:49:38 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:49:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* The readline generator and public entry point for command completion.
   readline calls cmd_generator in a loop: state_gen==0 on first call,
   then 1, 2... until we return NULL.  The static path_dirs persists
   across calls within one TAB press, which is why cmd_gen_init resets
   everything on state_gen==0. */

#include "completion_private.h"
#include "libft.h"
#include <stdio.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

/* readline generator: yields one matching command name per call.
   First exhausts the built-ins list, then scans each PATH directory.
   Matches are libc-allocated (rl_dup) because readline frees them --
   handing it ft_malloc memory aborted the shell on SAFE=0 builds. */
static char	*cmd_generator(const char *text, int state_gen)
{
	static t_cmd_gen	g;
	size_t				tlen;
	char				*name;

	tlen = ft_strlen(text);
	if (!state_gen)
		cmd_gen_init(&g);
	while (g_builtins[g.bidx])
	{
		name = g_builtins[g.bidx++];
		if (ft_strncmp(name, text, tlen) == 0)
			return (rl_dup(name));
	}
	return (cmd_gen_dirs(&g, tlen, text));
}

char	**complete_commands(const char *text, int start, int end)
{
	(void)start;
	(void)end;
	rl_completion_append_character = ' ';
	return (rl_completion_matches(text, cmd_generator));
}
