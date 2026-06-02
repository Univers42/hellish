/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_expand2.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "history.h"
#include "history_private.h"
#include "libft.h"
#include <stdlib.h>

static char	*resolve_bang_neg(t_shell *state, const char *s, size_t *adv)
{
	int	neg;
	int	num;

	neg = 1;
	num = 0;
	while (s[neg] >= '0' && s[neg] <= '9')
		num = num * 10 + (s[neg++] - '0');
	*adv = neg;
	return (hist_entry_at(state, -num));
}

static char	*resolve_bang_pos(t_shell *state, const char *s, size_t *adv)
{
	int	pos;
	int	num;

	num = 0;
	pos = 0;
	while (s[pos] >= '0' && s[pos] <= '9')
		num = num * 10 + (s[pos++] - '0');
	*adv = pos;
	return (hist_entry_at(state, num - 1));
}

char	*resolve_bang(t_shell *state, const char *s, size_t *adv)
{
	size_t	wlen;

	if (s[0] == '!')
		return (*adv = 1, hist_last(state));
	if (s[0] == '?')
	{
		wlen = 1;
		while (s[wlen] && s[wlen] != '?' && s[wlen] != '\n')
			wlen++;
		*adv = wlen + (s[wlen] == '?');
		return (hist_search_contains(state, s + 1, (int)wlen - 1));
	}
	if (s[0] == '-' && s[1] >= '0' && s[1] <= '9')
		return (resolve_bang_neg(state, s, adv));
	if (s[0] >= '0' && s[0] <= '9')
		return (resolve_bang_pos(state, s, adv));
	wlen = 0;
	while (s[wlen] && s[wlen] != ' ' && s[wlen] != '\t' && s[wlen] != '\n')
		wlen++;
	*adv = wlen;
	return (hist_search_prefix(state, s, wlen));
}

int	in_sq(const char *s, size_t pos)
{
	size_t	i;
	int		sq;

	sq = 0;
	i = 0;
	while (i < pos)
	{
		if (s[i] == '\'')
			sq = !sq;
		i++;
	}
	return (sq);
}

/* last command with the first occurrence of `old` replaced by `new`. */
char	*replace_first(const char *s, const char *old, const char *nw)
{
	t_string	r;
	char		*occ;

	occ = ft_strnstr(s, old, ft_strlen(s));
	if (!occ || !*old)
		return (NULL);
	vec_init(&r);
	r.elem_size = 1;
	vec_push_nstr(&r, (char *)s, occ - s);
	vec_push_str(&r, (char *)nw);
	vec_push_str(&r, occ + ft_strlen(old));
	return (vec_push(&r, &(char){0}), (char *)r.ctx);
}
