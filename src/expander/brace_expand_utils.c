/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   brace_expand_utils.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "brace_expand.h"

/* Find the leftmost '{' that forms an expandable group. Sets *close to its
   matching '}'.  Stepping with brace_next is what keeps the scan out of
   command substitutions and quoted spans: `$(cmd "{a,b}")` has no brace
   group of its own, so this returns -1 and the word is left alone. */
int	brace_find_expandable(const char *s, int *close)
{
	int	i;

	i = 0;
	while (s[i])
	{
		if (brace_group_opens_at(s, i, close))
			return (i);
		i = brace_next(s, i);
	}
	return (-1);
}

/* Recursively expand one comma alternative of a brace group and push all
   resulting strings into `out`.  Recursion handles nested braces inside each
   alternative: {a{1,2},b} → "a1", "a2", "b" via two push_piece calls. */
static void	push_piece(const char *s, int start, int len, t_vec *out)
{
	t_vec	sub;
	char	*piece;
	size_t	j;

	piece = ft_substr(s, start, len);
	sub = brace_expand_str(piece);
	xfree(piece);
	j = 0;
	while (j < sub.len)
		vec_push(out, &((char **)sub.ctx)[j++]);
	xfree(sub.ctx);
}

static void	split_commas(const char *s, int open, int close, t_vec *out)
{
	int	depth;
	int	i;
	int	start;

	depth = 0;
	start = open + 1;
	i = start;
	while (i <= close)
	{
		if (i == close || (s[i] == ',' && depth == 0))
		{
			push_piece(s, start, i - start, out);
			start = i + 1;
		}
		depth += (s[i] == '{') - (s[i] == '}');
		i = brace_next(s, i);
	}
}

/* Return the list of alternative strings inside the brace group at
   s[open..close].  A sequence spec (A..B[..S]) is detected first and
   generates elements via run_seq; otherwise split_commas tokenises on
   top-level commas and brace_expand_str recursively expands each piece.
   The returned t_vec owns its strings; caller must destroy it. */
t_vec	brace_alternatives(const char *s, int open, int close)
{
	t_vec	out;
	char	*body;

	vec_init(&out);
	out.elem_size = sizeof(char *);
	body = ft_substr(s, open + 1, close - open - 1);
	if (brace_gen_sequence(body, &out))
		return (xfree(body), out);
	xfree(body);
	split_commas(s, open, close, &out);
	return (out);
}
