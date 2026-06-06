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

/* Tab completion entry point.  We install one function
   (rl_attempted_completion_function) and let readline call it for every
   TAB.  The function dispatches: position 0 = command, '$' prefix =
   variable, anything else = file.  NULL return falls back to readline's
   default filename completion (which we want for non-dollar arguments). */

#include "libft.h"
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <dirent.h>

char	**complete_commands(const char *text, int start, int end);
char	**complete_variables(const char *text, int start, int end);
char	**complete_files(const char *text, int start, int end);

/* Completion dispatcher registered with readline.  start==0 means we
   are at the command position (no preceding words yet).  text[0]=='$'
   triggers variable completion regardless of position. */
static char	**cmd_completion(const char *text, int start, int end)
{
	(void)end;
	if (start == 0)
		return (complete_commands(text, start, end));
	if (text[0] == '$')
		return (complete_variables(text, start, end));
	return (NULL);
}

/* Register our completion function and set the default append char to
   space.  Must be called once after readline is initialised; subsequent
   calls are harmless (just overwrites the same pointers). */
void	setup_completion(void)
{
	rl_attempted_completion_function = cmd_completion;
	rl_completion_append_character = ' ';
}
