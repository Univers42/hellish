/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rl_suggest2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/17 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/17 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "rl_private.h"

/* The ghost for the line as it is now. */

/* Forget a POSTDISPLAY a widget set once the line is not the one it was
   set for -- "until the buffer next changes". */
static void	sg_post_drop(t_rl *l)
{
	l->sg_txt = NULL;
	xfree(l->sg_post);
	xfree(l->sg_post_at);
	l->sg_post = NULL;
	l->sg_post_at = NULL;
}

/* The text to paint after the cursor for the line readline just drew, or
   NULL. A POSTDISPLAY a widget set wins over the history suggestion, and
   shows even with HELLISH_AUTOSUGGEST off. l->sg_txt remembers the answer
   (it points into sg_ent / sg_post) for the accept key and for widgets. */
char	*rl_suggest_update(t_shell *state, int cols)
{
	t_rl		*l;
	char		*e;

	l = &state->rl;
	l->sg_txt = NULL;
	if (l->sg_post && ft_strcmp(l->sg_post_at, rl_line_buffer))
		sg_post_drop(l);
	if (!l->sg_post && !l->feat_want[FEAT_ACCEPT])
		return (NULL);
	if (!rl_suggest_gate(l, cols))
		return (NULL);
	if (l->sg_post)
		l->sg_txt = l->sg_post;
	else if (rl_end > 0)
	{
		e = rl_suggest_find(state, l);
		if (e)
			l->sg_txt = e + rl_end;
	}
	if (l->sg_txt && !*l->sg_txt)
		l->sg_txt = NULL;
	return (l->sg_txt);
}

/* What the accept keys may insert right now: the ghost of the last frame,
   if the line still is what it was drawn for. Keys can arrive faster than
   frames (a paste), so the entry is matched against the line again -- and
   an alternative shown by a cycle belongs to the line it was cycled for,
   not to a longer one typed since. */
char	*rl_suggest_live(t_rl *l)
{
	size_t	n;

	n = (size_t)rl_end;
	if (!l->sg_txt || rl_point != rl_end)
		return (NULL);
	if (l->sg_cyc_line && ft_strcmp(l->sg_cyc_line, rl_line_buffer))
		return (NULL);
	if (l->sg_post && !ft_strcmp(l->sg_post_at, rl_line_buffer))
		return (l->sg_post);
	if (!l->sg_post && l->sg_ent && ft_strlen(l->sg_ent) > n
		&& !ft_strncmp(l->sg_ent, rl_line_buffer, n))
		return (l->sg_ent + n);
	return (NULL);
}

/* How many bytes of `s` fit in `avail` columns, cut on a character
   boundary (a half-written UTF-8 sequence would print as garbage), and
   their width in `used`. A byte that is not a character stops it. */
size_t	rl_ghost_fit(const char *s, int avail, int *used)
{
	mbstate_t	st;
	wchar_t		wc;
	size_t		i;
	size_t		n;
	int			w;

	ft_memset(&st, 0, sizeof(st));
	i = 0;
	*used = 0;
	while (s[i])
	{
		n = mbrtowc(&wc, s + i, MB_CUR_MAX, &st);
		if (n == 0 || n >= (size_t)(-2))
			break ;
		w = wcwidth(wc);
		if (w < 0 || *used + w > avail)
			break ;
		*used += w;
		i += n;
	}
	return (i);
}
