/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_array_elem_op.c                             :+:      :+:    :+:   */
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

#define AEOP_VAR "__hellish_aeop"

bool	is_valid_ident(char *s, int len);
int		try_unset(t_shell *state, char *key);

/* ${name[sub]OP} for a single element (sub is not the @ or star form),
   OP any parameter operator (default/alt/trim/subst/case). The element
   is resolved, bound to a scratch var (UNSET when the element is unset,
   so :- vs - distinguish correctly), then `scratch OP` runs through the
   scalar engine — so ${c[x]:-0}, ${a[i]#pre}, ${h[k]:+1} all work. */

/* Resolve the value of name[sub]; NULL when the element is unset. */
static char	*elem_value(t_shell *state, const char *s, int nl, int sublen)
{
	char	*val;
	char	*sub;
	char	*r;
	long	idx;

	val = env_expand_n(state, (char *)s, nl);
	sub = expand_param_word(state, (char *)s + nl + 1, sublen, false);
	if (assoc_is(val))
	{
		r = assoc_get(val, sub, (int)ft_strlen(sub));
		xfree(sub);
		return (r);
	}
	r = arith_expand(state, sub, (int)ft_strlen(sub));
	idx = 0;
	if (r)
		idx = ft_atoi(r);
	xfree(r);
	xfree(sub);
	if (arr_is(val))
		return (arr_get_idx(val, idx));
	if (val && idx == 0)
		return (ft_strdup(val));
	return (NULL);
}

/* Split name[sub]OP: *nl name length, *sublen subscript length, *opat
   operator offset. False unless sub is a real (non-aggregate) subscript AND an
   operator follows the ']'. */
static bool	elem_op_split(const char *s, int len, int *nl, int sublen[2])
{
	int	i;
	int	close;

	i = 0;
	while (i < len && s[i] != '[')
		i++;
	if (i < 1 || i + 2 >= len || !is_valid_ident((char *)s, i))
		return (false);
	if (s[i + 1] == '@' || s[i + 1] == '*')
		return (false);
	close = i + 1;
	while (close < len && s[close] != ']')
		close++;
	if (close >= len - 1 || s[close] != ']')
		return (false);
	*nl = i;
	sublen[0] = close - i - 1;
	sublen[1] = close + 1;
	return (true);
}

/* Apply OP to the element via the scratch var + scalar engine.  elem is
   moved into the scratch var; a NULL elem means the element is unset, so
   the scratch var is unset too and :- vs - distinguish correctly. */
static char	*elem_apply(t_shell *state, const char *op, int oplen, char *elem)
{
	char	*body;
	char	*res;
	int		kl;

	if (elem)
		env_set(&state->env, env_create(ft_strdup(AEOP_VAR), elem, false));
	else
		try_unset(state, AEOP_VAR);
	kl = (int)ft_strlen(AEOP_VAR);
	body = xmalloc((size_t)kl + oplen + 1);
	ft_memcpy(body, AEOP_VAR, kl);
	ft_memcpy(body + kl, op, oplen);
	body[kl + oplen] = '\0';
	res = expand_param_format(state, body, kl + oplen, false);
	xfree(body);
	try_unset(state, AEOP_VAR);
	if (!res)
		res = ft_strdup("");
	return (res);
}

bool	expand_array_elem_op(t_shell *state, t_token *tt)
{
	char	*elem;
	char	*res;
	int		nl;
	int		sub[2];

	if ((tt->tt != TT_ENVVAR && tt->tt != TT_DQENVVAR)
		|| !elem_op_split(tt->start, tt->len, &nl, sub))
		return (false);
	if (sub[1] >= tt->len)
		return (false);
	elem = elem_value(state, tt->start, nl, sub[0]);
	res = elem_apply(state, tt->start + sub[1], tt->len - sub[1], elem);
	tt->start = res;
	tt->len = (int)ft_strlen(res);
	tt->allocated = true;
	return (parena_note_attach(), true);
}
