/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   env_assoc2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/28 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "shell.h"

/* Write-side helpers for associative arrays. Each mutation rebuilds a
   fresh encoded value; insertion order is preserved (an existing key is
   updated in place, a new key is appended). */

/* Append one "key<US>value" record taken from an iterator slice. */
static void	arec_append(t_string *out, const t_assoc_it *r)
{
	if (out->len > 1)
		vec_push_char(out, ARR_RS);
	if (r->k && r->kl > 0)
		vec_push_nstr(out, (char *)r->k, r->kl);
	vec_push_char(out, ARR_US);
	if (r->v && r->vl > 0)
		vec_push_nstr(out, (char *)r->v, r->vl);
}

/* New value = `old` with `key` set to `nv`. If the key exists its value
   is replaced in place (order kept); otherwise it is appended. */
char	*assoc_with_set(const char *old, const char *key, int klen,
			const char *nv)
{
	t_string	out;
	t_assoc_it	it;
	t_assoc_it	rec;
	bool		done;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, ARR_ASSOC_MAGIC);
	rec = (t_assoc_it){.k = key, .kl = klen,
		.v = nv, .vl = (int)ft_strlen(nv)};
	assoc_it_init(&it, old);
	done = false;
	while (assoc_next(&it))
	{
		if (it.kl == klen && ft_strncmp(it.k, key, klen) == 0)
			(arec_append(&out, &rec), done = true);
		else
			arec_append(&out, &it);
	}
	if (!done)
		arec_append(&out, &rec);
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* New value = `old` minus `key` (unset h[key]); the array stays assoc. */
char	*assoc_without(const char *old, const char *key, int klen)
{
	t_string	out;
	t_assoc_it	it;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, ARR_ASSOC_MAGIC);
	assoc_it_init(&it, old);
	while (assoc_next(&it))
	{
		if (!(it.kl == klen && ft_strncmp(it.k, key, klen) == 0))
			arec_append(&out, &it);
	}
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* One [key]="value" record.  The key is quoted only when it needs it and
   the value always is, which is what bash prints -- see env_quote.c. */
static void	assoc_rec(t_string *out, t_assoc_it *it)
{
	bool	q;

	q = assoc_key_quoted(it->k, it->kl);
	vec_push_char(out, '[');
	if (q)
		vec_push_char(out, '"');
	vec_push_dquoted(out, it->k, it->kl);
	if (q)
		vec_push_char(out, '"');
	vec_push_str(out, "]=\"");
	vec_push_dquoted(out, it->v, it->vl);
	vec_push_char(out, '"');
}

/* declare -p / set display form: ([key]="val" ...) with keys quoted.
   Bash quirk: a non-empty associative array carries a trailing space
   before the closing paren ( [k]="v" ) — indexed arrays do not. */
char	*assoc_format(const char *val)
{
	t_string	out;
	t_assoc_it	it;

	vec_init(&out);
	out.elem_size = 1;
	vec_push_char(&out, '(');
	assoc_it_init(&it, val);
	while (assoc_next(&it))
	{
		if (out.len > 1)
			vec_push_char(&out, ' ');
		assoc_rec(&out, &it);
	}
	if (out.len > 1)
		vec_push_char(&out, ' ');
	vec_push_char(&out, ')');
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}
