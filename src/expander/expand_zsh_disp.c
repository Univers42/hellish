/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_disp.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 09:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 09:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* The zsh-only ${...} forms, tried ahead of every bash form and only in the
** zsh dialect.  One entry point so expand_param_format grows by one line
** rather than one per feature, and so the ORDER lives in a single place:
**
**     ${${...}...}   a nested expansion as the operand   -- must come first,
**                    because the operand has to be resolved before any
**                    operator on it can be read
**     ${:-word}      an EMPTY name with a default -- always takes it, since
**                    an empty name is always unset. zsh code uses it as
**                    "apply this to a literal word", which is what makes
**                    `${(%):-%N}` mean "prompt-expand %N". bash reads it as
**                    a bad substitution, so claiming it costs nothing.
**     ${x:h}         the modifier run
**
** `${x:#pat}` is NOT here: whether it filters elements or a joined string
** depends on the quoting, and only the TOKEN knows that. It is handled by
** zsh_hash_token (expand_zsh_hash2.c), one layer up.
**
** Nesting is first for a reason worth stating: `${${A}:-b}` has BOTH an
** inner expansion and an operator, and reading the operator against the
** literal text `${A}` would compare a variable's name to a default instead
** of its value.
*/

/* Is the body one nested `${...}` followed by the rest?  Returns the length
   of the nested part including both braces, or 0.  Only a body that STARTS
   with `${` qualifies; a `${` later in the body belongs to an operator's
   word and is expanded by that operator, not here. */
static int	zd_nested_len(const char *s, int slen)
{
	int	depth;
	int	i;

	if (slen < 4 || s[0] != '$' || s[1] != '{')
		return (0);
	depth = 0;
	i = 1;
	while (i < slen)
	{
		if (s[i] == '{')
			depth++;
		else if (s[i] == '}' && --depth == 0)
			return (i + 1);
		i++;
	}
	return (0);
}

/* Evaluate the nested part, BIND it to a scratch parameter, and read the
** outer operator against that parameter.
**
** THE VALUE, NOT ITS TEXT. Splicing the value in as text was the first
** attempt and it is wrong in a way that reads as working: with A=1,
** `${${A}:-b}` spliced to `1:-b`, which re-parses as positional $1 with a
** default -- unset, so the answer was "b" instead of "1". With A=/x/y,
** `${${A}:t}` spliced to `/x/y:t`, which is not a parameter name at all and
** came back as a bad substitution about text nobody wrote. Both look like
** the nesting simply "did not work"; neither says why.
**
** Binding instead means every outer operator -- the :- family, trim,
** substitution, the modifiers, :#, a subscript -- applies to the value
** through the code that already implements it. There is no second
** implementation of any of them, which is why this is a few lines rather
** than a parallel expander.
**
** The scratch parameter is removed afterwards, and its name is depth-keyed
** so a doubly-nested body cannot clobber the level still reading it. */
static char	*zd_nested(t_shell *state, const char *s, int slen, int n)
{
	static int	depth = 0;
	char		*inner;
	char		*body;
	char		*name;
	char		*out;

	inner = zf_inner_text(state, s, n);
	name = zd_bind_name(depth++);
	if (!inner || !name)
		return (depth--, xfree(inner), xfree(name), NULL);
	env_set(&state->env, env_create(ft_strdup(name), inner, false));
	body = zd_splice(name, s + n, slen - n);
	out = NULL;
	if (body)
		out = expand_param_format(state, body, (int)ft_strlen(body), false);
	if (body && !out)
		out = zd_plain(state, name, (int)ft_strlen(name));
	zd_unbind(state, name);
	depth--;
	return (xfree(body), xfree(name), out);
}

char	*zsh_dispatch(t_shell *state, const char *s, int slen, bool arr)
{
	char	*out;
	int		n;

	if (!zsh_mode(state) || slen <= 0)
		return (NULL);
	n = zd_nested_len(s, slen);
	if (n > 0)
		return (zd_nested(state, s, slen, n));
	(void)arr;
	if (slen > 1 && s[0] == ':' && (s[1] == '-' || s[1] == '+'))
		return (pf_word_pipeline(state, s + 2, slen - 2, false));
	out = zsh_modifier(state, s, slen);
	if (out)
		return (out);
	return (NULL);
}
