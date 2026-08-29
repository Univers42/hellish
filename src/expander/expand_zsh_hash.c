/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_hash.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 09:15:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 09:15:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

bool	case_match(const char *s, const char *p);

/* `${x:#pattern}` -- keep what does NOT match, and with the (M) flag keep
** only what DOES.
**
**     0="${${ZERO:-${0:#$ZSH_ARGZERO}}:-${(%):-%N}}"
**     0="${${(M)0:#/[*]}:-$PWD/$0}"     <- the pattern is a slash-star;
**                                            bracketed here so this comment
**                                            does not open a nested one
**
** is the standard zsh plugin preamble for "find my own path", and it uses
** both halves. On an ARRAY it filters elements; on a scalar it is all or
** nothing -- the value or the empty string.
**
** It is a FILTER, not a trim: `${x:#f*}` on "foo" is empty, not "oo". The
** difference matters because `${x#f*}` (no colon) IS the trim, one character
** apart and quietly different -- which is why this is a separate operator
** rather than a flag on the existing one.
*/

/* One element: keep it or drop it. `want` is true for (M).
   case_match is the shell's OWN pattern matcher -- the one `case` and
   `[[ = ]]` use -- so `*`, `?` and `[...]` mean here exactly what they mean
   everywhere else, and a second matcher cannot drift from the first. */
static bool	zh_keep(const char *v, const char *pat, bool want)
{
	return (case_match(v, pat) == want);
}

/* The SCALAR answer. An array reaches here when the expansion is quoted --
   `"${(M)a:#f*}"` -- because quoting joins the array to one string BEFORE
   the filter runs, exactly as it does for (o) and (u). So `a=(foo bar fig)`
   quoted matches "foo bar fig" against `f*` as a whole and keeps all of it,
   while unquoted it keeps only `foo` and `fig`. Both look like the filter,
   and they are different answers; checked against zsh 5.9. */
static char	*zh_scalar(const char *val, const char *pat, bool want)
{
	char	*joined;
	char	*out;

	joined = NULL;
	if (arr_is(val))
	{
		joined = arr_join(val, ' ');
		val = joined;
	}
	if (zh_keep(val, pat, want))
		out = ft_strdup(val);
	else
		out = ft_strdup("");
	return (xfree(joined), out);
}

/* Filter an encoded array, returning a fresh encoded array. */
static char	*zh_filter_array(const char *val, const char *pat, bool want)
{
	char		*elems[256];
	const char	*cur;
	const char	*v;
	long		idx;
	int			n[2];

	n[0] = 0;
	cur = val + 1;
	while (arr_next(&cur, &idx, &v, &n[1]) && n[0] < 256)
	{
		elems[n[0]] = ft_strndup(v, (size_t)n[1]);
		if (zh_keep(elems[n[0]], pat, want))
			n[0]++;
		else
			xfree(elems[n[0]]);
	}
	v = arr_from_elems(elems, n[0], NULL);
	while (n[0]-- > 0)
		xfree(elems[n[0]]);
	return ((char *)v);
}

/* Find the `:#` that follows the name, or -1.  The pattern runs from there
   to the end of the body. */
int	zh_find(const char *s, int slen, int *name_len)
{
	int	i;

	i = pf_scan_name(s, slen);
	if (i <= 0 || i + 1 >= slen)
		return (-1);
	if (s[i] != ':' || s[i + 1] != '#')
		return (-1);
	*name_len = i;
	return (i + 2);
}

/* ${name:#pat} and ${(M)name:#pat}. `want` comes from the (M) flag, which
   the caller has already parsed. Returns a fresh value; for an array that
   value is still encoded, so a caller in a split context can emit fields. */
char	*zsh_hash_op(t_shell *state, const char *s, int slen, t_zhash h)
{
	char	*val;
	char	*pat;
	char	*out;
	int		nl;
	int		at;

	at = zh_find(s, slen, &nl);
	if (at < 0)
		return (NULL);
	val = env_expand_n(state, (char *)s, nl);
	if (!val)
		val = "";
	pat = expand_param_pattern(state, s + at, slen - at);
	if (arr_is(val) && h.as_array)
		out = zh_filter_array(val, pat, h.want);
	else
		out = zh_scalar(val, pat, h.want);
	return (xfree(pat), out);
}
