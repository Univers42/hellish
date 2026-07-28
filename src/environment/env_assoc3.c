/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_assoc3.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/28 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

/* Whole-array views of an associative value (split out of env_assoc.c
   for the 42-norm five-function file limit). */

/* Space-joined keys (${!h[@]}) or values (${h[@]}, sep=' '). */
static char	*assoc_join(const char *val, char sep, int want_keys)
{
	t_string	out;
	t_assoc_it	it;

	vec_init(&out);
	out.elem_size = 1;
	assoc_it_init(&it, val);
	while (assoc_next(&it))
	{
		if (out.len && sep)
			vec_push_char(&out, sep);
		if (want_keys)
			vec_push_nstr(&out, (char *)it.k, it.kl);
		else
			vec_push_nstr(&out, (char *)it.v, it.vl);
	}
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

char	*assoc_keys(const char *val)
{
	return (assoc_join(val, ' ', 1));
}

char	*assoc_values(const char *val, char sep)
{
	return (assoc_join(val, sep, 0));
}
