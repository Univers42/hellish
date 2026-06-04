/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ifs_split2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

static void	split_loop(t_vec *out, const char *s,
				size_t n, const char *ifs)
{
	size_t	i;
	size_t	start;

	i = 0;
	while (i < n && is_ws_ifs(s[i], ifs))
		i++;
	start = i;
	while (i < n)
	{
		if (is_nw_ifs(s[i], ifs))
		{
			push_f(out, s, start, i++);
			start = i;
		}
		else if (is_ws_ifs(s[i], ifs))
		{
			push_f(out, s, start, i);
			i = skip_ws_delimiter(s, n, ifs, i);
			start = i;
		}
		else
			i++;
	}
	if (start < n)
		push_f(out, s, start, n);
}

char	**ifs_split_posix(const char *s, const char *ifs)
{
	t_vec	out;

	vec_init(&out);
	out.elem_size = sizeof(char *);
	split_loop(&out, s, ft_strlen(s), ifs);
	return (vec_push(&out, &(char *){NULL}), (char **)out.ctx);
}
