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

/* Alias expansion at the input level: given a raw input line, check
   whether the first word is an alias and, if so, splice the alias value
   in front of the remaining input.  Only the first word is expanded here
   (no recursive/chained expansion -- that happens in the lexer which
   calls this again on the resulting string up to ALIAS_MAX_DEPTH times).
   Returns NULL if the first word is not an alias (caller keeps input). */

#include "sh_alias.h"
#include "libft.h"
#include <stdlib.h>

/* Advance past blanks so the first word check handles leading spaces. */
static size_t	skip_whitespace(const char *s, size_t i)
{
	while (s[i] && (s[i] == ' ' || s[i] == '\t'))
		i++;
	return (i);
}

/* Measure the first word: stop at whitespace or any shell metachar.
   This mirrors the lexer's idea of where a word ends, so we don't
   accidentally include a redirection operator in the alias name. */
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

/* Concatenate alias-value + the remaining input (rest).  The caller
   passes input+pos+wlen as rest, so the old command word is replaced
   but everything after it (args, redirections) is preserved exactly. */
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
