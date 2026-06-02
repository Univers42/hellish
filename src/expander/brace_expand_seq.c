/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   brace_expand_seq.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "brace_expand.h"

typedef struct s_seq
{
	long	a;
	long	b;
	long	step;
	int		width;
	bool	alpha;
}	t_seq;

static bool	is_int_str(const char *s)
{
	int	i;

	i = (s[0] == '-' || s[0] == '+');
	if (!s[i])
		return (false);
	while (s[i])
		if (!ft_isdigit((unsigned char)s[i++]))
			return (false);
	return (true);
}

/* Format `v` left-padded with zeros to `width` digits (sign kept outside). */
static char	*fmt_num(long v, int width)
{
	char	*digits;
	char	*pad;
	char	*out;
	int		dlen;

	digits = ft_itoa(v < 0 ? -v : v);
	dlen = (int)ft_strlen(digits);
	while (dlen++ < width)
	{
		pad = ft_strjoin("0", digits);
		free(digits);
		digits = pad;
	}
	if (v >= 0)
		return (digits);
	out = ft_strjoin("-", digits);
	return (free(digits), out);
}

/* Parse "A..B[..S]" into a t_seq; return false if not a valid sequence. */
static bool	parse_seq(const char *body, t_seq *q)
{
	char	**p;
	int		n;

	if (!ft_strnstr(body, "..", ft_strlen(body)))
		return (false);
	p = ft_split_str((char *)body, ".");
	n = 0;
	while (p[n])
		n++;
	q->step = 0;
	if ((n != 2 && n != 3) || (n == 3 && !is_int_str(p[2])))
		return (free_tab(p), false);
	if (n == 3)
		q->step = ft_atoi(p[2]);
	q->alpha = (!is_int_str(p[0]) && ft_strlen(p[0]) == 1
			&& !is_int_str(p[1]) && ft_strlen(p[1]) == 1);
	if (!q->alpha && !(is_int_str(p[0]) && is_int_str(p[1])))
		return (free_tab(p), false);
	q->a = ((q->alpha) ? p[0][0] : ft_atoi(p[0]));
	q->b = ((q->alpha) ? p[1][0] : ft_atoi(p[1]));
	q->width = 0;
	if (!q->alpha && ((p[0][0] == '0' && ft_strlen(p[0]) > 1)
			|| (p[1][0] == '0' && ft_strlen(p[1]) > 1)))
	{
		q->width = (int)ft_strlen(p[0]);
		if ((int)ft_strlen(p[1]) > q->width)
			q->width = (int)ft_strlen(p[1]);
	}
	return (free_tab(p), true);
}

static void	run_seq(t_seq q, t_vec *out)
{
	long	i;
	long	st;
	char	*s;

	st = q.step;
	if (st == 0)
		st = 1;
	if ((q.a <= q.b) != (st > 0))
		st = -st;
	i = q.a;
	while ((st > 0 && i <= q.b) || (st < 0 && i >= q.b))
	{
		if (q.alpha)
			s = ft_strndup((char []){(char)i, 0}, 1);
		else
			s = fmt_num(i, q.width);
		vec_push(out, &s);
		i += st;
	}
}

/* If `body` is a brace sequence ("A..B[..S]") report true; when `out` is
   non-NULL, also generate the elements into it. */
bool	brace_gen_sequence(const char *body, t_vec *out)
{
	t_seq	q;

	if (!parse_seq(body, &q))
		return (false);
	if (out)
		run_seq(q, out);
	return (true);
}
