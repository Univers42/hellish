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
int		collect_specs(const char *str, t_deque_tok *tt, t_hd **out);

static void	free_specs(t_hd *sp, int n)
{
	int	i;

	i = 0;
	while (i < n)
		free(sp[i++].delim);
	free(sp);
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

static void	advance_hd(const char **p, size_t cur,
				t_string *out, t_walk_ctx *c)
{
	while (c->si < c->n && c->sp[c->si].line < cur)
		c->si++;
	while (c->si < c->n && c->sp[c->si].line == cur)
	{
		if (delim_present(*p, &c->sp[c->si]))
			c->got += (collect_body(p, &cur, &out[1], &c->sp[c->si]), 1);
		c->si++;
	}
}

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
		advance_hd(&p, cur++, out, &c);
	}
	return (c.got);
}

bool	split_heredocs(const char *str, char **stripped, char **bodies)
{
	t_deque_tok	tt;
	t_hd		*sp;
	t_string	out[2];
	int			n;
	int			got;

	tt = (t_deque_tok){0};
	deque_init(&tt.deqtok, 100, sizeof(t_token));
	tokenizer((char *)str, &tt);
	n = collect_specs(str, &tt, &sp);
	free(tt.deqtok.buff);
	if (n == 0)
		return (free(sp), false);
	vec_init(&out[0]);
	vec_init(&out[1]);
	out[0].elem_size = 1;
	out[1].elem_size = 1;
	got = walk_and_strip(str, sp, n, out);
	free_specs(sp, n);
	if (got == 0)
		return (free(out[0].ctx), free(out[1].ctx), false);
	*stripped = (char *)out[0].ctx;
	*bodies = (char *)out[1].ctx;
	return (true);
}
