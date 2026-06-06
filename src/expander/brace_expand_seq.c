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
char	*fmt_num(long v, int width)
{
	char	*digits;
	char	*pad;
	char	*out;
	int		dlen;

	if (v < 0)
		digits = ft_itoa(-v);
	else
		digits = ft_itoa(v);
	dlen = (int)ft_strlen(digits);
	while (dlen++ < width)
	{
		pad = ft_strjoin("0", digits);
		xfree(digits);
		digits = pad;
	}
	if (v >= 0)
		return (digits);
	out = ft_strjoin("-", digits);
	return (xfree(digits), out);
}

static void	set_seq_width(t_seq *q, char **p)
{
	q->width = 0;
	if (q->alpha)
		return ;
	if ((p[0][0] == '0' && ft_strlen(p[0]) > 1)
		|| (p[1][0] == '0' && ft_strlen(p[1]) > 1))
	{
		q->width = (int)ft_strlen(p[0]);
		if ((int)ft_strlen(p[1]) > q->width)
			q->width = (int)ft_strlen(p[1]);
	}
}

static bool	fill_seq_ab(t_seq *q, char **p, int n)
{
	q->step = 0;
	if ((n != 2 && n != 3) || (n == 3 && !is_int_str(p[2])))
		return (free_tab(p), false);
	if (n == 3)
		q->step = ft_atoi(p[2]);
	q->alpha = (!is_int_str(p[0]) && ft_strlen(p[0]) == 1
			&& !is_int_str(p[1]) && ft_strlen(p[1]) == 1);
	if (!q->alpha && !(is_int_str(p[0]) && is_int_str(p[1])))
		return (free_tab(p), false);
	if (q->alpha)
	{
		q->a = p[0][0];
		q->b = p[1][0];
	}
	else
	{
		q->a = ft_atoi(p[0]);
		q->b = ft_atoi(p[1]);
	}
	set_seq_width(q, p);
	return (free_tab(p), true);
}

/* Parse "A..B[..S]" into a t_seq; return false if not a valid sequence. */
bool	parse_seq(const char *body, t_seq *q)
{
	char	**p;
	int		n;

	if (!ft_strnstr(body, "..", ft_strlen(body)))
		return (false);
	p = ft_split_str((char *)body, ".");
	n = 0;
	while (p[n])
		n++;
	return (fill_seq_ab(q, p, n));
}
