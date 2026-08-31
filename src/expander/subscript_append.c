/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   subscript_append.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* `a[i]+=v` and `M[k]+=v` -- appending to one ELEMENT.
**
**     a=(1 2); a[1]+=Z      bash: a[1] becomes "2Z"
**                           hellish: a[1] became "Z"
**
** scalar_append() stripped the trailing `+` and then bailed out on any key
** containing `[`, which turned `+=` into plain `=`.  So the append silently
** OVERWROTE, which is the worst shape a bug can take here: the script asked
** to add to a value and the shell threw the old one away, with no message
** and a zero status.
**
** The fix reads the element's current value and splices it in FRONT of the
** new one, after which the ordinary set path runs untouched -- so there is
** one place that writes an element, not two that must agree.
*/

/* The element's current value, or "" when the array, the index or the key
   does not exist yet -- bash treats all three as an empty left operand, so
   `a[9]+=x` on a short array simply stores "x". */
static char	*elem_current(t_shell *state, const char *sub, const char *old)
{
	char	*key;
	char	*cur;
	long	idx;

	if (assoc_is(old))
	{
		key = expand_param_word(state, (char *)sub,
				(int)ft_strlen(sub) - 1, false);
		cur = assoc_get(old, key, (int)ft_strlen(key));
		return (xfree(key), cur);
	}
	key = expand_param_word(state, (char *)sub,
			(int)ft_strlen(sub) - 1, false);
	idx = arr_sub_index(state, key, arr_count(old));
	xfree(key);
	if (idx < 0)
		return (NULL);
	return (arr_get_idx(old, idx));
}

/* Does this assignment key end in `+`?  Strips it when it does, so the rest
   of the write path sees the plain name it expects. */
bool	subscript_take_append(t_env *ret)
{
	size_t	n;

	if (!ret->key)
		return (false);
	n = ft_strlen(ret->key);
	if (n == 0 || ret->key[n - 1] != '+')
		return (false);
	ret->key[n - 1] = '\0';
	return (true);
}

/* Turn ret->value into "current + new" so the caller can store it as an
   ordinary assignment.  `sub` is the subscript text including its closing
   bracket; `old` is the variable's whole current value. */
void	subscript_append_value(t_shell *state, t_env *ret, const char *sub,
			const char *old)
{
	char	*cur;
	char	*joined;

	cur = elem_current(state, sub, old);
	if (!cur)
		return ;
	joined = ft_strjoin(cur, ret->value);
	xfree(cur);
	if (!joined)
		return ;
	xfree(ret->value);
	ret->value = joined;
}

/* The whole append step for subscript_assign: nothing to do unless the key
   ends in `+`, in which case the element's current value is spliced in
   front and the caller stores the result as an ordinary assignment.  `br`
   points at the key's `[`; it is temporarily cut so env_expand sees the
   bare name, then put back for the caller. */
void	subscript_prepend_current(t_shell *state, t_env *ret, char *br)
{
	char	*old;

	if (!subscript_take_append(ret))
		return ;
	*br = '\0';
	old = env_expand(state, ret->key);
	*br = '[';
	subscript_append_value(state, ret, br + 1, old);
}
