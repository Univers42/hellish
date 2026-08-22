/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_variables.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Variable ($VAR) completion.  We scan the raw `environ` array directly
   because the completion callback has no t_shell* pointer.  This means
   shell-local (non-exported) variables are invisible here -- a known
   limitation.  The generator strips the leading '$' for prefix matching,
   then var_gen_dollar prepends it back so readline shows "$HOME" etc. */

#include "completion_private.h"
#include "libft.h"
#include <stdio.h>
#include <readline/readline.h>
#include <stdlib.h>

extern char	**environ;

/* File-scope index into environ[]; reset to 0 on each new TAB press. */
static int	g_var_idx;

/* Scan environ[] for the next entry whose name starts with `prefix` (text
   with any leading '$' stripped) and return it as "$NAME", NULL when
   exhausted.  readline inserts and then frees what we return, so the '$'
   is built into the one libc allocation rather than glued on by a second
   pass through ft_strjoin -- that intermediate was ft_malloc memory
   handed to libc free() on SAFE=0 builds (issue #40). */
static char	*var_generator(const char *text, int state_gen)
{
	const char	*prefix;
	size_t		plen;
	char		*entry;
	char		*eq;

	prefix = text + (text[0] == '$');
	plen = ft_strlen(prefix);
	if (!state_gen)
		g_var_idx = 0;
	while (environ[g_var_idx])
	{
		entry = environ[g_var_idx++];
		eq = ft_strchr(entry, '=');
		if (eq && ft_strncmp(entry, prefix, plen) == 0)
			return (rl_dup_dollar(entry, (size_t)(eq - entry)));
	}
	return (NULL);
}

char	**complete_variables(const char *text, int start, int end)
{
	(void)start;
	(void)end;
	rl_completion_append_character = ' ';
	return (rl_completion_matches(text, var_generator));
}
