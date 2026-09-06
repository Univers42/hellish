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
   string; single converts only the first character. (The optional
   char-class pattern form ${v^^[abc]} is a documented v1 scope-out —
   bare-op case conversion is the near-universal usage.) */

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
   character, else just the first; the rest is copied as is.  Shared with
   ${v@U} / ${v@L} / ${v@u} (expand_param_xform.c), the same three shapes
   under other spellings. */
char	*case_body(const char *val, char op, bool all)
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
		if (i == 0 || all)
			case_push(&out, val + i, n, op);
		else
			vec_push_nstr(&out, (char *)val + i, n);
		i += n;
	}
	vec_push_char(&out, '\0');
	return ((char *)out.ctx);
}

char	*expand_case(t_shell *state, const char *s, int slen, int name_len)
{
	char	op;
	bool	all;
	char	*val;

	op = s[name_len];
	all = (name_len + 1 < slen && s[name_len + 1] == op);
	val = pf_get_var_value(state, s, name_len);
	if (!val)
		return (ft_strdup(""));
	if (arr_is(val))
		return (ft_strdup(val));
	return (case_body(val, op, all));
}

/* Is s a ${name^...} / ${name,...} / ${name~...} form? Sets *nl to the
   name length and returns true. Rejects the substring ':' cases and the
   trim/subst ops (those are matched earlier by their own finders). */
bool	find_case_op(const char *s, int slen, int *nl)
{
	int	i;

	i = 0;
	if (i < slen && ft_isdigit((unsigned char)s[i]))
		while (i < slen && ft_isdigit((unsigned char)s[i]))
			i++;
	else if (i < slen && (s[i] == '_' || ft_isalpha((unsigned char)s[i])))
	{
		i++;
		while (i < slen && (s[i] == '_' || ft_isalnum((unsigned char)s[i])))
			i++;
	}
	else
		return (false);
	*nl = i;
	return (i < slen && (s[i] == '^' || s[i] == ',' || s[i] == '~'));
}
