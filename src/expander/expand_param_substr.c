/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_substr.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sh_input.h"
#include "arith.h"
#include <limits.h>

void	exit_clean(t_shell *state, int code);

/* bash substring expansion: ${v:off} and ${v:off:len}, both fields being
   arithmetic expressions.  A negative off counts back from the end of the
   value; a negative len marks where to STOP counting back from the end; a
   stop before the start is bash's fatal "substring expression < 0" (status
   1).  Out-of-range offsets just yield an empty string.  LLONG_MAX is the
   private "no len field" sentinel — an actual len can never reach it since
   it would be clamped to the value length anyway. */

/* Evaluate one substring field as arithmetic; empty text means 0 (so
   ${v::2} works).  On an arithmetic error the field falls back to 0 —
   a garbage offset is not worth aborting the line for.
     The field is WORD-EXPANDED first, then evaluated — the same two
   steps, in the same order, as an array subscript: bash accepts nested
   expansions in the bounds, and `${cur:0:${#words[i]}}` is the exact
   spelling bash-completion walks the cursor with (issue #105, wave 2:
   the raw `${#w}` meant nothing to the arithmetic parser, the bound
   fell back to 0, and every completion word came back empty). */
static long long	ss_eval(t_shell *state, const char *s, int len)
{
	bool		err;
	char		*x;
	long long	r;
	int			i;

	i = 0;
	while (i < len && (s[i] == ' ' || s[i] == '\t'))
		i++;
	if (i == len)
		return (0);
	err = false;
	x = expand_param_word(state, (char *)s, len, false);
	if (!x)
		return (arith_eval(state, s, len, &err));
	r = arith_eval(state, x, (int)ft_strlen(x), &err);
	return (xfree(x), r);
}

/* Locate the top-level ':' separating off from len (parens shield nested
   colons in ?: arithmetic).  Returns the index within [from, slen) or slen. */
static int	ss_len_sep(const char *s, int slen, int from)
{
	int	depth;

	depth = 0;
	while (from < slen)
	{
		if (s[from] == '(')
			depth++;
		else if (s[from] == ')')
			depth--;
		else if (s[from] == ':' && depth == 0)
			return (from);
		from++;
	}
	return (slen);
}

/* Normalise off/len against the value and duplicate the slice.  Order
   matters: off is resolved first (negative = from the end), then len is
   applied relative to the resolved off (>= 0) or the end (< 0). */
static char	*ss_build(t_shell *state, const char *val, long long off,
			long long len)
{
	long long	l;
	long long	end;

	l = (long long)ft_strlen(val);
	if (off < 0)
		off = l + off;
	if (off < 0 || off > l)
		return (ft_strdup(""));
	if (len == LLONG_MAX)
		end = l;
	else if (len < 0)
		end = l + len;
	else
		end = off + len;
	if (end > l)
		end = l;
	if (end < off)
	{
		ft_eprintf("%s: substring expression < 0\n", state->ctx);
		set_cmd_status(state, create_exec_state(1, false));
		if (state->metinp != INP_RL)
			exit_clean(state, 1);
		return (ft_strdup(""));
	}
	return (ft_strndup(val + off, (size_t)(end - off)));
}

/* Entry: s[0..name_len) is the parameter, s[name_len] the ':'. */
char	*expand_substr(t_shell *state, const char *s, int slen, int name_len)
{
	char		*val;
	long long	off;
	int			sep;

	val = pf_get_var_value(state, s, name_len);
	if (!val)
		return (ft_strdup(""));
	sep = ss_len_sep(s, slen, name_len + 1);
	off = ss_eval(state, s + name_len + 1, sep - name_len - 1);
	if (sep >= slen)
		return (ss_build(state, val, off, LLONG_MAX));
	return (ss_build(state, val, off,
			ss_eval(state, s + sep + 1, slen - sep - 1)));
}
