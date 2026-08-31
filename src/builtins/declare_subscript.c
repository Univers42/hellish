/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   declare_subscript.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"

/* `declare NAME[sub]=VALUE` as a single OPERAND -- the quoted spelling:
**
**     declare "a[1]=Z"          bash: a[1] becomes Z
**     declare "M[a b]=v"        bash: the key is literally `a b`
**
** did nothing at all.  declare_assign() split on the first `=` and stored a
** variable whose NAME was the text `a[1]`, so `declare -p a` reported the
** array unchanged and no error was printed.  Silent, and it is the spelling
** a careful script uses precisely BECAUSE it survives word splitting -- a
** key with a space has no other portable form.
**
** The subscript machinery already exists and is what `a[1]=Z` uses when the
** parser produces it; this only routes the builtin's operand into it.
*/

/* The `=` that separates NAME[sub] from VALUE.  Not simply the first one: a
   subscript may legitimately contain `=`, and bash accepts `M[a=b]=v` with
   `a=b` as the key.  So a leading [...] is stepped over before looking.
   Returns NULL for a bare NAME, which declare treats as "just accept it". */
char	*declare_assign_eq(const char *word)
{
	const char	*p;

	p = word;
	while (*p && *p != '=' && *p != '[')
		p++;
	if (*p == '[')
	{
		while (*p && *p != ']')
			p++;
		if (*p == ']')
			p++;
	}
	if (*p == '+' && p[1] == '=')
		p++;
	if (*p == '=')
		return ((char *)p);
	return (NULL);
}

/* One NAME or NAME=VALUE operand: NAME=VALUE assigns (export flag from -x),
   a bare NAME with no '=' is a no-op that just accepts the name.
     subscript_assign() rewrites key and value in place when the key carries
   a [subscript], so a plain name falls through it untouched and the two
   spellings cannot drift apart. */
void	declare_assign(t_shell *state, const char *word, int exprt)
{
	char	*eq;
	t_env	ev;

	eq = declare_assign_eq(word);
	if (!eq)
	{
		if (exprt && env_get(&state->env, (char *)word))
			env_get(&state->env, (char *)word)->exported = true;
		return ;
	}
	ev = env_create(ft_strndup(word, eq - word), ft_strdup(eq + 1),
			exprt != 0);
	subscript_assign(state, &ev);
	env_set(&state->env, ev);
}
