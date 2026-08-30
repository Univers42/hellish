/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_shift.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 02:40:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 02:40:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"

bool	is_valid_ident(char *s, int len);

/* zsh's `shift [n] name` -- shifting an ARRAY rather than the positional
** parameters.
**
**     if [[ $#dirhistory_past -ge $DIRHISTORY_SIZE ]]; then
**       shift dirhistory_past
**     fi
**
** is oh-my-zsh's dirhistory capping its stack at thirty entries, and there
** is no bash equivalent: `shift name` there is "numeric argument required".
** So this is gated on the dialect and cannot reach bash input, and the
** positional builtin behind it is untouched.
*/

/* Is `s` a plain NAME, the way an array variable is spelled?  A count is
   digits and a name is not, which is the whole of the distinction that
   tells `shift 2` from `shift a`. */
static bool	shift_is_name(const char *s)
{
	return (s && *s && is_valid_ident((char *)s, (int)ft_strlen(s)));
}

/* Split `shift [n] name` into its parts.  False when the last argument is
   not a name -- `shift`, `shift 2`, `shift $n` -- leaving those to the
   positional form. */
static bool	shift_arr_args(t_shell *state, t_vec argv, char **name, long *n)
{
	char	*last;

	if (!zsh_mode(state) || argv.len < 2 || argv.len > 3)
		return (false);
	last = ((char **)argv.ctx)[argv.len - 1];
	if (!shift_is_name(last))
		return (false);
	*name = last;
	*n = 1;
	if (argv.len == 3)
		*n = ft_atol(((char **)argv.ctx)[1]);
	return (true);
}

/* Drop the first `n` elements of `val`, renumbering what is left.  Built
   out of arr_splice at index 0 rather than a second walk of the encoding:
   "remove this element and close the array up behind it" is exactly that
   primitive, and these arrays hold tens of entries, not millions. */
static char	*shift_drop(const char *val, long n)
{
	char	*cur;
	char	*next;

	cur = ft_strdup(val);
	while (n-- > 0)
	{
		next = arr_splice(cur, 0, NULL, 0);
		xfree(cur);
		cur = next;
	}
	return (cur);
}

/* `shift [n] name`.  Returns -1 when this is not the array form so the
   caller falls through to the positional builtin.
     A count larger than the array is an error and leaves the array ALONE:
   zsh does not do a partial shift, and half-emptying a plugin's stack
   because the count was one too big is the kind of quiet damage that shows
   up three navigations later. */
int	zsh_shift_array(t_shell *state, t_vec argv)
{
	char	*name;
	char	*val;
	long	n;

	if (!shift_arr_args(state, argv, &name, &n))
		return (-1);
	if (n < 0)
		return (ft_eprintf("%s: shift: argument to shift must be "
				"non-negative\n", state->ctx), 1);
	val = env_expand(state, name);
	if (!arr_is(val))
		return (0);
	if (n > arr_count(val))
		return (ft_eprintf("%s: shift: shift count must be <= $#\n",
				state->ctx), 1);
	env_set(&state->env, env_create(ft_strdup(name),
			shift_drop(val, n), false));
	return (0);
}
