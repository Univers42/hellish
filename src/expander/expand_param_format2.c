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
   The ? form is delegated to pf_err_word, which owns the bash-parity
   status/exit dance.  The = form calls env_set to actually store the
   word, so subsequent uses of $p see the assigned value. */
char	*err_or_assign(t_shell *state, char *val, t_pe_op o)
{
	bool	act;

	if (o.opc == '?')
		return (pf_err_word(state, val, o));
	if (o.colon)
		act = (val == NULL || *val == '\0');
	else
		act = (val == NULL);
	if (!act)
		return (ft_strdup(val));
	val = expand_param_word(state, o.word, o.wlen, o.dq);
	env_set(&state->env, env_create(ft_strndup(o.name, o.name_len),
			ft_strdup(val), false));
	return (val);
}

/* Dispatch the four modifying operators (- = ? +) after parsing has filled
   `o`.  Splits into two families: default/alt (- and +) versus error/assign
   (? and =) since they have different behaviours on an empty but set var.
   The @ and * names only land here from the arith and heredoc contexts
   (token context routes them through expand_positional_op instead); for
   those the joined positionals stand in for the value, NULL when $# is 0. */
char	*expand_param_op(t_shell *state, t_pe_op o)
{
	char	*val;
	char	*ret;
	bool	at;

	at = (o.name_len == 1 && (o.name[0] == '@' || o.name[0] == '*'));
	if (at && state->pos.count > 0)
		val = join_positionals(state);
	else if (at)
		val = NULL;
	else
		val = pf_get_var_value(state, o.name, o.name_len);
	if (o.opc == '-' || o.opc == '+')
		ret = default_or_alt(state, val, o);
	else
		ret = err_or_assign(state, val, o);
	if (at && val)
		xfree(val);
	return (ret);
}

/* Parse the contents of a ${...} for the operator forms (- = ? +).  The
   name can be a digit-only positional, an alphanumeric identifier, or the
   special positional aggregates @ and *; after it an optional ':' (colon
   flag) and then one of -=?+ must follow.  On success `o` is filled and
   true is returned so the caller can skip the trim/substitution checks. */
bool	find_param_op(const char *s, int slen, t_pe_op *o)
{
	int	i;

	i = pf_scan_name(s, slen);
	if (i <= 0)
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
	o->dq = false;
	return (true);
}

/* The '*' arm of pat_match_pub: swallow the star run, then advance `s` one
   character at a time until the rest of the pattern matches — the shortest
   viable position.  Split out to keep pat_match_pub in the line budget. */
static bool	pat_match_star(const char *p, const char *s)
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

/* Minimal shell-pattern matcher used by prefix/suffix trimming and ${/} subst.
   Handles * (any sequence) and ? (any single character) only — not [ranges].
   Recursive descent: consume one pattern unit and recurse on the rest.
   The * branch advances `s` one character at a time to find the shortest
   position where the remainder of the pattern still matches. */
bool	pat_match_pub(const char *p, const char *s)
{
	if (*p == '\0')
		return (*s == '\0');
	if (*p == '\\' && p[1] != '\0')
	{
		if (p[1] == *s && *s != '\0')
			return (pat_match_pub(p + 2, s + 1));
		return (false);
	}
	if (*p == '*')
		return (pat_match_star(p, s));
	if (*p == '?' && *s != '\0')
		return (pat_match_pub(p + 1, s + 1));
	if (*p == *s && *s != '\0')
		return (pat_match_pub(p + 1, s + 1));
	return (false);
}
