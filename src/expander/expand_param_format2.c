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
#include "case_match.h"

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

/* Prefix/suffix trimming (${v#p} ${v%p}) and ${v/a/b} substitution match
** with the SAME matcher `case` and `[[ == ]]` use.
**
** This was a private recursive descent that handled `*` and `?` and, by its
** own comment, "not [ranges]". So a bracket expression in a trim pattern
** matched nothing, silently:
**
**     v=abc; echo ${v%%[bx]*}       bash: a        here: abc
**
** No error, status 0, the original string handed back -- indistinguishable
** from a pattern that legitimately did not match. That is exactly what
** case_match.h's header comment warns about: a second matcher eventually
** disagrees with the first, and a pattern means two things depending on
** where it was written. Now there is one, so brackets, character classes
** and extglob groups work everywhere or nowhere.
**
** The argument order is (pattern, string) here and (string, pattern) there;
** the wrapper stays so the call sites do not all have to flip. */
bool	pat_match_pub(const char *p, const char *s)
{
	return (case_match(s, p));
}
