/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   brace_expand.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "brace_expand.h"

/* Find the matching '}' for the '{' at index `open`, tracking nesting.
   Returns the index of the matching '}', or -1 if unmatched.  The cursor
   steps with brace_next so a '}' sitting inside a substitution or a quoted
   span cannot close the group. */
int	brace_match(const char *s, int open)
{
	int	depth;
	int	i;

	depth = 0;
	i = open;
	while (s[i])
	{
		if (s[i] == '{')
			depth++;
		else if (s[i] == '}' && --depth == 0)
			return (i);
		i = brace_next(s, i);
	}
	return (-1);
}

/* Cartesian product: for every (mid, post) pair build pre+mid+post and push
   it into `out`.  Empty strings are silently dropped (bash does not generate
   "" from {,} or {a,} unless there is actual content). */
static void	combine(t_vec *out, const char *pre, t_vec *mids, t_vec *posts)
{
	size_t	a;
	size_t	b;
	char	*tmp;
	char	*res;

	a = 0;
	while (a < mids->len)
	{
		b = 0;
		while (b < posts->len)
		{
			tmp = ft_strjoin(pre, ((char **)mids->ctx)[a]);
			res = ft_strjoin(tmp, ((char **)posts->ctx)[b]);
			xfree(tmp);
			if (res[0])
				vec_push(out, &res);
			else
				xfree(res);
			b++;
		}
		a++;
	}
}

/* Expand the brace group at s[open..close] into a list of strings, then do
   a cartesian combine with the pre-brace text and the recursively expanded
   post-brace tail.  Recursion handles nested braces naturally: each `mid`
   from brace_alternatives is already fully expanded before combine. */
static t_vec	expand_at(const char *s, int open, int close)
{
	t_vec	mids;
	t_vec	posts;
	t_vec	out;
	char	*pre;

	mids = brace_alternatives(s, open, close);
	posts = brace_expand_str(s + close + 1);
	pre = ft_substr(s, 0, open);
	vec_init(&out);
	out.elem_size = sizeof(char *);
	combine(&out, pre, &mids, &posts);
	xfree(pre);
	vec_destroy(&mids, free_str_elem);
	vec_destroy(&posts, free_str_elem);
	return (out);
}

/* Recursively brace-expand `s`. Always returns at least one string (a copy of
   `s` when there is nothing to expand). Caller owns the returned strings. */
t_vec	brace_expand_str(const char *s)
{
	t_vec	out;
	int		open;
	int		close;

	open = brace_find_expandable(s, &close);
	if (open < 0)
	{
		vec_init(&out);
		out.elem_size = sizeof(char *);
		vec_push(&out, &(char *){ft_strdup(s)});
		return (out);
	}
	return (expand_at(s, open, close));
}
