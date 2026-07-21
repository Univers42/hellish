/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_assoc2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

/* Write-side helpers for associative arrays. Each mutation rebuilds a
   fresh encoded value; insertion order is preserved (an existing key is
   updated in place, a new key is appended). */

/* Append one "key<US>value" record. */
static void	arec_append(t_string *out, const char *k, int kl,
				const char *v, int vl)
{
	if (out->len > 1)
		vec_push_char(out, ARR_RS);
	if (k && kl > 0)
		vec_push_nstr(out, (char *)k, kl);
	vec_push_char(out, ARR_US);
	if (v && vl > 0)
		vec_push_nstr(out, (char *)v, vl);
}

/* New value = `old` with `key` set to `nv`. If the key exists its value
   is replaced in place (order kept); otherwise it is appended. */
char	*assoc_with_set(const char *old, const char *key, int klen,
			const char *nv)
{
	t_string	out;
	const char	*cur;
	const char	*k;
	const char	*v;
	int			kl;
	int			vl;
	bool		done;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, ARR_ASSOC_MAGIC);
	cur = "";
	if (assoc_is(old))
		cur = old + 1;
	done = false;
	while (assoc_next(&cur, &k, &kl, &v, &vl))
	{
		if (kl == klen && ft_strncmp(k, key, klen) == 0)
			(arec_append(&out, key, klen, nv, (int)ft_strlen(nv)),
				done = true);
		else
			arec_append(&out, k, kl, v, vl);
	}
	if (!done)
		arec_append(&out, key, klen, nv, (int)ft_strlen(nv));
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* New value = `old` minus `key` (unset h[key]); the array stays assoc. */
char	*assoc_without(const char *old, const char *key, int klen)
{
	t_string	out;
	const char	*cur;
	const char	*k;
	const char	*v;
	int			kl;
	int			vl;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, ARR_ASSOC_MAGIC);
	cur = "";
	if (assoc_is(old))
		cur = old + 1;
	while (assoc_next(&cur, &k, &kl, &v, &vl))
	{
		if (!(kl == klen && ft_strncmp(k, key, klen) == 0))
			arec_append(&out, k, kl, v, vl);
	}
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* declare -p / set display form: ([key]="val" ...) with keys quoted. */
char	*assoc_format(const char *val)
{
	t_string	out;
	const char	*cur;
	const char	*k;
	const char	*v;
	int			kl;
	int			vl;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, '(');
	cur = val + 1;
	while (assoc_next(&cur, &k, &kl, &v, &vl))
	{
		if (out.len > 1)
			vec_push_char(&out, ' ');
		vec_push_char(&out, '[');
		vec_push_nstr(&out, (char *)k, kl);
		vec_push_str(&out, "]=\"");
		vec_push_nstr(&out, (char *)v, vl);
		vec_push_char(&out, '"');
	}
	vec_push_char(&out, ')');
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}
