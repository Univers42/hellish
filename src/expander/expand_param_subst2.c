/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_subst2.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"

/* Anchored forms of ${v/pat/rep}: `/#pat` replaces only a match that starts
   at the very beginning of the value, `/%pat` only a match that ends at the
   very end.  Both replace at most once (an anchor can only match one spot).
   An empty pattern matches the empty string at the anchor, so ${v/#/X}
   prepends and ${v/%/X} appends — same as bash. */

/* Detect the anchor character right after the operator slashes: 2 for a
   '#' (prefix-anchored), 3 for '%' (suffix-anchored), 0 for none. */
int	subst_anchor(t_trim_ctx ctx, int g)
{
	if (ctx.name_len + 1 + g < ctx.slen
		&& ctx.name[ctx.name_len + 1 + g] == '#')
		return (2);
	if (ctx.name_len + 1 + g < ctx.slen
		&& ctx.name[ctx.name_len + 1 + g] == '%')
		return (3);
	return (0);
}

/* ${v/#pat/rep}: if pat matches a prefix of val, rebuild as rep + tail. */
char	*patsub_prefix(const char *val, const char *pat, const char *rep)
{
	t_string	out;
	int			k;

	k = patsub_match_len(pat, val);
	if (k < 0)
		return (ft_strdup(val));
	vec_init(&out);
	out.elem_size = 1;
	vec_push_str(&out, (char *)rep);
	vec_push_str(&out, (char *)val + k);
	return (vec_push_char(&out, '\0'), (char *)out.ctx);
}

/* ${v/%pat/rep}: find the earliest position whose match runs exactly to the
   end of val (patsub_match_len returns the longest match, which is capped
   by the remaining length, so equality is the suffix test), and rebuild as
   head + rep.  The i <= len bound lets the empty pattern match at the very
   end so ${v/%/X} appends. */
char	*patsub_suffix(const char *val, const char *pat, const char *rep)
{
	t_string	out;
	int			len;
	int			i;
	int			k;

	len = (int)ft_strlen(val);
	i = 0;
	while (i <= len)
	{
		k = patsub_match_len(pat, val + i);
		if (k >= 0 && i + k == len)
		{
			vec_init(&out);
			out.elem_size = 1;
			vec_push_nstr(&out, (char *)val, i);
			vec_push_str(&out, (char *)rep);
			return (vec_push_char(&out, '\0'), (char *)out.ctx);
		}
		i++;
	}
	return (ft_strdup(val));
}
