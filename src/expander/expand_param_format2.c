/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_param_format2.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/14 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/14 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "sh_input.h"

void	exit_clean(t_shell *state, int code);

/* Handle the error/assign family:
     ${p?w}  → error-exit if p is UNSET          (write `w` to stderr)
     ${p:?w} → error-exit if p is UNSET OR EMPTY
     ${p=w}  → assign `w` to p if p is UNSET,     return p's (new) value
     ${p:=w} → assign `w` to p if p is UNSET OR EMPTY
   The ? form exits the shell in non-interactive mode (metinp != INP_RL);
   in interactive use it just prints the error and returns empty so the
   user can keep typing.  The = form calls env_set to actually store the
   word, so subsequent uses of $p see the assigned value. */
char	*err_or_assign(t_shell *state, char *val, t_pe_op o)
{
	bool	act;

	if (o.colon)
		act = (val == NULL || *val == '\0');
	else
		act = (val == NULL);
	if (o.opc == '?')
	{
		if (!act)
			return (ft_strdup(val));
		ft_eprintf("%s: %.*s: %.*s\n", state->ctx, o.name_len, o.name,
			o.wlen, o.word);
		state->last_cmd_st_exe = create_exec_state(127, false);
		set_cmd_status(state, state->last_cmd_st_exe);
		if (state->metinp != INP_RL)
			exit_clean(state, 127);
		return (ft_strdup(""));
	}
	if (!act)
		return (ft_strdup(val));
	val = expand_param_word(state, o.word, o.wlen);
	env_set(&state->env, env_create(ft_strndup(o.name, o.name_len),
			ft_strdup(val), false));
	return (val);
}

/* Dispatch the four modifying operators (- = ? +) after parsing has filled
   `o`.  Splits into two families: default/alt (- and +) versus error/assign
   (? and =) since they have different behaviours on an empty but set var. */
char	*expand_param_op(t_shell *state, t_pe_op o)
{
	char	*val;

	val = pf_get_var_value(state, o.name, o.name_len);
	if (o.opc == '-' || o.opc == '+')
		return (default_or_alt(state, val, o));
	return (err_or_assign(state, val, o));
}

/* Parse the contents of a ${...} for the operator forms (- = ? +).  The
   name can be a digit-only positional or an alphanumeric identifier; after
   it an optional ':' (colon flag) and then one of -=?+ must follow.  On
   success `o` is filled and true is returned so the caller can skip the
   trim/substitution checks. */
bool	find_param_op(const char *s, int slen, t_pe_op *o)
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
	o->name = s;
	o->name_len = i;
	o->colon = (i < slen && s[i] == ':');
	i += o->colon;
	if (i >= slen || !ft_strchr("-=?+", s[i]))
		return (false);
	o->opc = s[i];
	o->word = s + i + 1;
	o->wlen = slen - i - 1;
	return (true);
}

/* Minimal shell-pattern matcher used by prefix/suffix trimming and ${/} subst.
   Handles * (any sequence) and ? (any single character) only — not [ranges].
   Recursive descent: consume one pattern unit and recurse on the rest.
   The * branch advances `s` one character at a time to find the shortest
   position where the remainder of the pattern still matches. */
bool	pat_match_pub(const char *p, const char *s)
{
	if (*p == '\0')
		return (*s == '\0');
	if (*p == '*')
	{
		while (*p == '*')
			p++;
		if (*p == '\0')
			return (true);
		while (*s)
		{
			if (pat_match_pub(p, s))
				return (true);
			s++;
		}
		return (pat_match_pub(p, s));
	}
	if (*p == '?' && *s != '\0')
		return (pat_match_pub(p + 1, s + 1));
	if (*p == *s && *s != '\0')
		return (pat_match_pub(p + 1, s + 1));
	return (false);
}

/* ${val%pattern} — remove the SHORTEST suffix of `val` that matches
   `pattern`.  We scan from the end of val backwards until we find the
   rightmost position where pat_match_pub(pattern, val+i) succeeds.
   Shortest means we try from the end first and stop at the first match. */
char	*trim_suffix_shortest(const char *val, const char *pattern)
{
	int	vlen;
	int	i;

	vlen = ft_strlen(val);
	i = vlen;
	while (i >= 0)
	{
		if (pat_match_pub(pattern, val + i))
			return (ft_strndup(val, i));
		i--;
	}
	return (ft_strdup(val));
}
