/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   completion.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <dirent.h>

char	**complete_commands(const char *text, int start, int end);
char	**complete_variables(const char *text, int start, int end);
char	**complete_files(const char *text, int start, int end);

static char	**cmd_completion(const char *text, int start, int end)
{
	(void)end;
	if (start == 0)
		return (complete_commands(text, start, end));
	if (text[0] == '$')
		return (complete_variables(text, start, end));
	return (NULL);
}

void	setup_completion(void)
{
	rl_attempted_completion_function = cmd_completion;
	rl_completion_append_character = ' ';
}
