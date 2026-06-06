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

/* Determine if the brace body contains a top-level comma (depth 0), which
   distinguishes comma-alternation from a bare {word} that should NOT expand.
   Inner braces are tracked so {a,{b,c}} is correctly seen as having one
   top-level comma between 'a' and the inner group. */
static bool	has_top_comma(const char *s, int open, int close)
{
	int	depth;
	int	i;

	depth = 0;
	i = open + 1;
	while (i < close)
	{
		if (s[i] == '{')
			depth++;
		else if (s[i] == '}')
			depth--;
		else if (s[i] == ',' && depth == 0)
			return (true);
		i++;
	}
	return (false);
}

/* Find the leftmost '{' that forms an expandable group (contains a top-level
   comma or is a valid sequence). Sets *close to its matching '}'. */
int	brace_find_expandable(const char *s, int *close)
{
	int		i;
	int		c;
	char	*body;
	bool	seq;

	i = 0;
	while (s[i])
	{
		if (s[i] == '{')
		{
			c = brace_match(s, i);
			if (c > i + 1)
			{
				body = ft_substr(s, i + 1, c - i - 1);
				seq = brace_gen_sequence(body, NULL);
				xfree(body);
				if (seq || has_top_comma(s, i, c))
					return (*close = c, i);
			}
		}
		i++;
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
		i++;
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
