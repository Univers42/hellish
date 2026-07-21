/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format5.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/06 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/06 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"
#include "sh_input.h"

void	exit_clean(t_shell *state, int code);

/* True when the whole ${...} body is a plain parameter that needs no
   operator handling: a special single character, a positional number, or a
   NAME of alnum/underscore — each optionally behind a '!' (indirection is
   not implemented but must not be rejected as a bad substitution).
   Anything else (${}, ${ }, ${v!x}, ...) is a bad substitution. */
bool	pf_valid_plain(const char *s, int n)
{
	int	i;

	if (n <= 0)
		return (false);
	if (n == 1 && ft_strchr("@*#?-$!", s[0]))
		return (true);
	i = 0;
	if (s[0] == '!')
		i = 1;
	if (ft_isdigit(s[i]))
	{
		while (i < n && ft_isdigit(s[i]))
			i++;
		return (i == n);
	}
	if (s[i] != '_' && !ft_isalpha(s[i]))
		return (false);
	while (i < n && (s[i] == '_' || ft_isalnum(s[i])))
		i++;
	return (i == n);
}

/* ${!name}: is `name` a plain identifier (so this is indirection, not the
   ${!a[@]} keys form or ${!pre*} prefix form)? */
bool	pf_is_indirect(const char *s, int n)
{
	int	i;

	if (n <= 0 || (s[0] != '_' && !ft_isalpha((unsigned char)s[0])))
		return (false);
	i = 1;
	while (i < n && (s[i] == '_' || ft_isalnum((unsigned char)s[i])))
		i++;
	return (i == n);
}

/* ${!name}: expand `name` to get a variable NAME, then expand that. An
   unset or empty middle name yields the empty string, like bash. */
char	*expand_indirect(t_shell *state, const char *s, int n)
{
	char	*mid;
	char	*val;

	mid = env_expand_n(state, (char *)s, n);
	if (!mid || !*mid)
		return (ft_strdup(""));
	val = env_expand(state, mid);
	if (!val)
		return (ft_strdup(""));
	return (ft_strdup(val));
}

/* bash parity for a malformed ${...}: report "bad substitution", set $? to
   127, and abort a non-interactive shell (bash --posix -c exits 127). */
char	*pf_bad_subst(t_shell *state, const char *s, int slen)
{
	ft_eprintf("%s: ${%.*s}: bad substitution\n", state->ctx, slen, s);
	set_cmd_status(state, create_exec_state(127, false));
	if (state->metinp != INP_RL)
		exit_clean(state, 127);
	return (ft_strdup(""));
}

/* Route a trim/substitution ctx to the right evaluator (split out of
   expand_param_format to keep it inside the norm line budget). */
char	*pf_trim_or_subst(t_shell *state, t_trim_ctx ctx)
{
	if (*ctx.op == '/')
		return (expand_subst(state, ctx));
	return (expand_trim(state, ctx));
}

/* Assemble the trim/subst context handed to the evaluators. */
t_trim_ctx	pf_make_ctx(const char *s, int slen, const char *op, int nlen)
{
	t_trim_ctx	ctx;

	ctx.name = s;
	ctx.name_len = nlen;
	ctx.op = op;
	ctx.slen = slen;
	return (ctx);
}

/* Detect the substring form NAME:... — a valid plain parameter followed by
   a ':' that find_param_op did not already claim as :-/:=/:?/:+ — and hand
   back the name length. */
bool	pf_find_substr(const char *s, int slen, int *name_len)
{
	int	i;

	i = 0;
	if (i < slen && ft_strchr("@*#?-$!", s[i]) && i + 1 < slen
		&& s[i + 1] == ':')
		return (*name_len = 1, true);
	while (i < slen && (s[i] == '_' || ft_isalnum(s[i])))
		i++;
	if (i == 0 || i >= slen || s[i] != ':')
		return (false);
	return (*name_len = i, true);
}
