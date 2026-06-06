/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   extract2.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "heredoc_private.h"
#include "lexer.h"
#include "redir.h"

void	collect_body(const char **p, size_t *cur, t_string *body, t_hd *s);
int		collect_specs(const char *str, t_hd **out);

/* Release the spec array produced by collect_specs.  Each entry owns its
   delim string (allocated by hd_delim); the array itself is xfree'd last. */
static void	free_specs(t_hd *sp, int n)
{
	int	i;

	i = 0;
	while (i < n)
		xfree(sp[i++].delim);
	xfree(sp);
}

/* Is the delimiter line reachable from p without consuming it? */
static bool	delim_present(const char *p, t_hd *s)
{
	const char	*ls;

	while (*p)
	{
		ls = p;
		while (*p && *p != '\n')
			p++;
		if (*p == '\n')
			p++;
		if (is_delim_line(ls, p - ls, s))
			return (true);
	}
	return (false);
}

/* Collect every heredoc body whose operator sits on the current line. ln pins
   the operator line so multiple specs sharing it (`cat <<A <<B`) all match even
   as collect_body bumps *cur past consumed body lines, keeping the line counter
   in sync so the next command's heredocs still resolve. */
static void	advance_hd(const char **p, size_t *cur,
				t_string *out, t_walk_ctx *c)
{
	size_t	ln;

	ln = *cur;
	while (c->si < c->n && c->sp[c->si].line < ln)
		c->si++;
	while (c->si < c->n && c->sp[c->si].line == ln)
	{
		if (delim_present(*p, &c->sp[c->si]))
			c->got += (collect_body(p, cur, &out[1], &c->sp[c->si]), 1);
		c->si++;
	}
}

/* Walk the source line by line: emit each line into out[0] (the stripped
   source for the parser), and after each line call advance_hd to pull any
   heredoc bodies that start on that line into out[1].  The result is a
   parser-safe source string (no heredoc bodies) plus a packed body stream
   in source order.  Returns the count of heredoc bodies actually extracted
   (some operators may not have their delimiter present in the string). */
static int	walk_and_strip(const char *str, t_hd *sp, int n, t_string *out)
{
	const char	*p;
	const char	*ls;
	size_t		cur;
	t_walk_ctx	c;

	p = str;
	cur = 0;
	c = (t_walk_ctx){.sp = sp, .n = n, .si = 0, .got = 0};
	while (*p)
	{
		ls = p;
		while (*p && *p != '\n')
			p++;
		if (*p == '\n')
			p++;
		vec_push_nstr(&out[0], ls, p - ls);
		advance_hd(&p, &cur, out, &c);
		cur++;
	}
	return (c.got);
}

/* Public entry point: split `str` into a body-free version (*stripped) and
   a packed heredoc body stream (*bodies).  Returns false if no heredoc
   bodies could be extracted (no << operators, or none with a matching
   delimiter in the string).  Callers must xfree both output strings. */
bool	split_heredocs(const char *str, char **stripped, char **bodies)
{
	t_hd		*sp;
	t_string	out[2];
	int			n;
	int			got;

	n = collect_specs(str, &sp);
	if (n == 0)
		return (xfree(sp), false);
	vec_init(&out[0]);
	vec_init(&out[1]);
	out[0].elem_size = 1;
	out[1].elem_size = 1;
	got = walk_and_strip(str, sp, n, out);
	free_specs(sp, n);
	if (got == 0)
		return (xfree(out[0].ctx), xfree(out[1].ctx), false);
	*stripped = (char *)out[0].ctx;
	*bodies = (char *)out[1].ctx;
	return (true);
}
