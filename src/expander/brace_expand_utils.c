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

/* Is there a top-level (depth-0) comma between s[open+1] and s[close-1]? */
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

/* Split the body (between open+1 and close-1) on top-level commas, recursively
   expanding each piece, accumulating fully expanded strings into `out`. */
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

/* Return the alternatives of the group at [open..close]: a generated sequence
   if the body is "X..Y[..step]", otherwise the recursively expanded comma
   pieces. */
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
