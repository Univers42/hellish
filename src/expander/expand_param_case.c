/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_case.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "mbchar.h"
#include <limits.h>

/* ${v^} ${v^^} ${v,} ${v,,} ${v~} ${v~~}: bash case conversion.
   ^ upper, , lower, ~ toggle. Doubled operator converts the whole
   string; single converts only the first character. A pattern after the
   operator narrows it to the characters that match: ${v^^[ab]} upcases
   only a and b, and ${v^b} upcases the first character only if it is b.
   The pattern used to be ignored, so ${v^b} upcased whatever came first.
   (The array and "$@" forms are still a scope-out.) */

/* Convert one char under op ('^' upper, ',' lower, '~' toggle). */
static char	case_conv(char c, char op)
{
	if (op == '^')
		return ((char)ft_toupper((unsigned char)c));
	if (op == ',')
		return ((char)ft_tolower((unsigned char)c));
	if (ft_isupper((unsigned char)c))
		return ((char)ft_tolower((unsigned char)c));
	return ((char)ft_toupper((unsigned char)c));
}

/* One character of the value under op, appended to out: ASCII through the
   byte table, a multibyte character decoded, converted and re-encoded
   (mb_conv, issue #120: ${x^^} on café is CAFÉ), kept as written when it
   does not decode. */
static void	case_push(t_string *out, const char *s, size_t n, char op)
{
	char	buf[MB_LEN_MAX];
	size_t	w;

	if (n == 1)
		return ((void)vec_push_char(out, case_conv(*s, op)));
	w = mb_conv(s, n, op, buf);
	if (w == 0)
		return ((void)vec_push_nstr(out, (char *)s, n));
	vec_push_nstr(out, buf, w);
}

/* Apply the operator to a fresh copy of the value: `all` converts every
   character, else just the first, and only a character `pat` matches
   (NULL: every one); the rest is copied as is. */
char	*case_body_pat(const char *val, char op, bool all, const char *pat)
{
	t_string	out;
	size_t		i;
	size_t		n;

	vec_init(&out);
	out.elem_size = 1;
	i = 0;
	while (val[i])
	{
		n = mb_len0(val + i);
		if ((i == 0 || all) && case_hit(val + i, n, pat))
			case_push(&out, val + i, n, op);
		else
			vec_push_nstr(&out, (char *)val + i, n);
		i += n;
	}
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

/* The unpatterned shape, shared with ${v@U} / ${v@L} / ${v@u}
   (expand_param_xform.c), the same three conversions under other
   spellings. */
char	*case_body(const char *val, char op, bool all)
{
	return (case_body_pat(val, op, all, NULL));
}

char	*expand_case(t_shell *state, const char *s, int slen, int name_len)
{
	char	op;
	int		at;
	char	*val;
	char	*pat;

	op = s[name_len];
	at = name_len + 1 + (name_len + 1 < slen && s[name_len + 1] == op);
	val = pf_get_var_value(state, s, name_len);
	if (!val)
		return (ft_strdup(""));
	if (arr_is(val))
		return (ft_strdup(val));
	pat = NULL;
	if (at < slen)
		pat = expand_param_pattern(state, s + at, slen - at);
	val = case_body_pat(val, op, at > name_len + 1, pat);
	return (xfree(pat), val);
}
