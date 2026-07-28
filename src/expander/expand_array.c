/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "arith.h"
#include "parena.h"

bool	is_valid_ident(char *s, int len);

/* ${name[subscript]} expansion. [@] and [*] mirror the positional $@/$*
   machinery: in a split context the token DEFERS via a registry marker
   (pointer-identity, like pos_mark, but carrying the array name) and the
   splitter emits one field per element; in a no-split context they join
   eagerly. A numeric subscript is an arithmetic expression. A scalar
   variable behaves as a one-element array at index 0, like bash. */

/* Split "name[body]": returns the name length and the body slice, or 0
   when the token is not exactly a valid NAME[ ... ] form. */
static int	subscript_of(const char *s, int len, const char **body, int *blen)
{
	int	i;

	i = 0;
	while (i < len && s[i] != '[')
		i++;
	if (i == 0 || i >= len - 1 || s[len - 1] != ']')
		return (0);
	if (!is_valid_ident((char *)s, i))
		return (0);
	*body = s + i + 1;
	*blen = len - i - 2;
	if (*blen <= 0)
		return (0);
	return (i);
}

/* The [@] / [*] forms. Deferred cases hand the splitter a marker whose
   POINTER identity proves it is ours; "${arr[*]}" (quoted star) joins
   with IFS[0] eagerly like "$*", no-split contexts join with a space. */
static bool	arr_at_star(t_shell *state, t_token *tt, char *val, bool split)
{
	char	sub;
	char	*sep;

	sub = tt->start[tt->len - 2];
	if (split && (tt->tt == TT_ENVVAR
			|| (tt->tt == TT_DQENVVAR && sub == '@')))
	{
		tt->start = arr_mark_push(state, tt->start,
				(int)(ft_strnchr(tt->start, '[', tt->len) - tt->start));
		tt->allocated = false;
		return (true);
	}
	sep = " ";
	if (sub == '*')
	{
		sep = env_get_ifs(&state->env);
		if (!sep[0])
			sep = "";
	}
	tt->start = arr_join(val, sep[0]);
	tt->len = (int)ft_strlen(tt->start);
	tt->allocated = true;
	return (parena_note_attach(), true);
}

/* Store an owned element string into the token, or empty when NULL. */
static void	arr_elem_emit(t_token *tt, char *elem)
{
	if (elem)
	{
		tt->start = elem;
		tt->len = (int)ft_strlen(elem);
		tt->allocated = true;
		return ((void)parena_note_attach());
	}
	tt->start = "";
	tt->len = 0;
	tt->allocated = false;
}

/* Subscript element read. For an associative value the subscript is a
   LITERAL string key; otherwise it is an arithmetic index (negative
   wraps from the end). A scalar answers only index 0. */
static void	arr_element(t_shell *state, t_token *tt, char *val, int nl)
{
	char	*res;
	char	*elem;
	long	idx;

	if (assoc_is(val))
	{
		res = expand_param_word(state, tt->start + nl + 1,
				tt->len - nl - 2, false);
		arr_elem_emit(tt, assoc_get(val, res, (int)ft_strlen(res)));
		return ((void)xfree(res));
	}
	res = arith_expand(state, tt->start + nl + 1, tt->len - nl - 2);
	idx = 0;
	if (res)
		idx = ft_atoi(res);
	xfree(res);
	if (idx < 0 && arr_is(val))
		idx += arr_count(val);
	elem = NULL;
	if (arr_is(val))
		elem = arr_get_idx(val, idx);
	else if (val && idx == 0)
		elem = ft_strdup(val);
	arr_elem_emit(tt, elem);
}

/* Entry, tried before the ${p-w} operator family: recognise NAME[...]
   tokens and expand them; leave everything else untouched. */
bool	expand_array_token(t_shell *state, t_token *tt, bool split_ctx)
{
	const char	*body;
	int			blen;
	int			nl;
	char		*val;

	if (tt->tt != TT_ENVVAR && tt->tt != TT_DQENVVAR)
		return (false);
	nl = subscript_of(tt->start, tt->len, &body, &blen);
	if (nl == 0)
		return (false);
	val = env_expand_n(state, (char *)tt->start, nl);
	if (blen == 1 && (body[0] == '@' || body[0] == '*'))
		return (arr_at_star(state, tt, val, split_ctx));
	arr_element(state, tt, val, nl);
	return (true);
}
