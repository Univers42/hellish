/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sq_quote.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 10:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 10:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"

/* Wrap a string in single quotes so the shell reads it back as itself.
**
** An embedded quote cannot be escaped inside single quotes, so the only
** spelling that works is to leave them: close, an escaped quote, reopen --
** the '\'' idiom bash's own printf %q emits. `it's` becomes 'it'\''s'.
**
** This was ${x@Q}'s private static until the completion dispatcher needed
** the same thing to hand a user's half-typed word to a shell function
** without it being reparsed. Two callers, one implementation: the failure
** of a second copy would be a word that re-parses to something ELSE, which
** is unquoted-eval territory and never announces itself.
*/
char	*sq_quote(const char *val)
{
	t_string	out;
	int			i;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, '\'');
	i = 0;
	while (val && val[i])
	{
		if (val[i] == '\'')
			vec_push_str(&out, "'\\''");
		else
			vec_push_char(&out, val[i]);
		i++;
	}
	vec_push_char(&out, '\'');
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}
