/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   brace_expand_seq2.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "brace_expand.h"

void	run_seq(t_seq q, t_vec *out)
{
	long	i;
	long	st;
	char	s_char[2];
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
		{
			s_char[0] = (char)i;
			s_char[1] = 0;
			s = ft_strndup(s_char, 1);
		}
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
