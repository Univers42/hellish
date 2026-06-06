/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   alias_expand.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "sh_alias.h"
#include "libft.h"
#include <stdlib.h>

static size_t	skip_whitespace(const char *s, size_t i)
{
	while (s[i] && (s[i] == ' ' || s[i] == '\t'))
		i++;
	return (i);
}

static size_t	word_len(const char *s, size_t i)
{
	size_t	start;

	start = i;
	while (s[i] && s[i] != ' ' && s[i] != '\t'
		&& s[i] != '\n' && s[i] != ';' && s[i] != '|'
		&& s[i] != '&' && s[i] != '<' && s[i] != '>')
		i++;
	return (i - start);
}

static char	*build_expanded(const char *val, const char *rest)
{
	char	*result;
	size_t	vlen;
	size_t	rlen;

	vlen = ft_strlen(val);
	rlen = ft_strlen(rest);
	result = xmalloc(vlen + rlen + 1);
	if (!result)
		return (NULL);
	ft_memcpy(result, val, vlen);
	ft_memcpy(result + vlen, rest, rlen);
	result[vlen + rlen] = '\0';
	return (result);
}

char	*alias_expand_input(t_hash *aliases, const char *input)
{
	size_t	pos;
	size_t	wlen;
	char	*word;
	char	*val;
	char	*result;

	if (!input || !aliases || !aliases->ctx)
		return (NULL);
	pos = skip_whitespace(input, 0);
	wlen = word_len(input, pos);
	if (wlen == 0)
		return (NULL);
	word = ft_substr(input, pos, wlen);
	if (!word)
		return (NULL);
	val = alias_get(aliases, word);
	xfree(word);
	if (!val)
		return (NULL);
	result = build_expanded(val, input + pos + wlen);
	return (result);
}
