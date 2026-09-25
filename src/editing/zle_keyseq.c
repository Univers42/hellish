/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   zle_keyseq.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "zle.h"

/* zsh's spelling of a key sequence, in readline's.
**
** The two share their backslash escapes (\e \C-x \M-x \n \t \NNN ...).
** What only zsh has is the caret -- `^X` a control character, `^[` the
** escape, `^?` DEL -- and \E. Handed to readline as written,
**
**     bindkey '^[[A' history-beginning-search-backward
**
** bound the four characters ^ [ [ A: the arrow never reached it, and
** neither did any other key a zsh config spells that way (#136). */

/* One two-byte token: a backslash escape passes through whole (so the
   `\C` of `\C-x` stays with its backslash), a caret becomes readline's
   control or escape. */
static void	zle_keyseq_pair(t_string *out, char a, char b)
{
	if (a == '\\' && b == 'E')
		vec_push_str(out, "\\e");
	else if (a == '\\')
	{
		vec_push_char(out, a);
		vec_push_char(out, b);
	}
	else if (b == '[')
		vec_push_str(out, "\\e");
	else if (b == '?')
		vec_push_str(out, "\\C-?");
	else
	{
		vec_push_str(out, "\\C-");
		vec_push_char(out, (char)ft_tolower(b));
	}
}

/* Owned; NULL only when out of memory. A caret or backslash with nothing
   after it is itself. */
char	*zle_keyseq(const char *s)
{
	t_string	out;
	size_t		i;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (s[i])
	{
		if (s[i + 1] && (s[i] == '\\' || s[i] == '^'))
		{
			zle_keyseq_pair(&out, s[i], s[i + 1]);
			i += 2;
		}
		else
			vec_push_nstr(&out, s + i++, 1);
	}
	if (!vec_push_nstr(&out, "", 0))
		return (xfree(out.ctx), NULL);
	return ((char *)out.ctx);
}
