/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd_helpers2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:21:58 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/23 14:35:23 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Walk the operand words (argv[first..]) and count them, recording the first
   two (op0, op1) for the 0/1/2-operand dispatch. Redirection operators are
   defensively skipped (along with the target word of a bare redirection) in
   case any leak into argv -- they are not directory operands. */
int	cd_collect_ops(t_vec argv, size_t first, char **op0, char **op1)
{
	size_t	i;
	char	*a;
	bool	skip;
	int		n;

	*op0 = NULL;
	*op1 = NULL;
	skip = false;
	n = 0;
	i = first;
	while (i < argv.len)
	{
		a = ((char **)argv.ctx)[i++];
		if (skip)
			skip = false;
		else if (is_redir_operator(a))
			skip = redir_needs_next(a);
		else if (n++ == 0)
			*op0 = a;
		else if (n == 2)
			*op1 = a;
	}
	return (n);
}

/* No-operand cd target: $HOME. POSIX mandates an error when HOME is unset
   rather than guessing, so we mirror bash. *out gets a fresh copy the caller
   frees. Returns non-zero (and prints) on error. */
int	cd_target_home(t_shell *state, char **out)
{
	char	*home;

	home = env_expand(state, "HOME");
	if (!home || !home[0])
		return (ft_eprintf("%s: cd: HOME not set\n", state->ctx), 1);
	*out = ft_strdup(home);
	return (*out == NULL);
}

/* `cd -` target: $OLDPWD. Sets o->echo so the destination is printed, matching
   bash. Errors when OLDPWD has never been set. */
int	cd_target_dash(t_shell *state, char **out, t_cdopt *o)
{
	char	*old;

	old = env_expand(state, OLDPWD_NAME);
	if (old == NULL)
		return (ft_eprintf(OLDPWD_NO_SET, state->ctx), 1);
	*out = ft_strdup(old);
	o->echo = true;
	return (*out == NULL);
}
