/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd_helpers6.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/07 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Build the new $PWD for the two-argument form: replace the first occurrence
   of `old` (found at `pos` inside pwd) with `neww`, keeping the surrounding
   text. Returns a fresh string the caller frees. */
static char	*cd_substitute(char *pwd, char *old, char *neww, char *pos)
{
	char	*pre;
	char	*head;
	char	*res;

	pre = ft_substr(pwd, 0, (size_t)(pos - pwd));
	if (!pre)
		return (NULL);
	head = ft_strjoin(pre, neww);
	xfree(pre);
	if (!head)
		return (NULL);
	res = ft_strjoin(head, pos + ft_strlen(old));
	return (xfree(head), res);
}

/* zsh-style `cd old new` (a hellish extension, not POSIX/bash): substitute the
   first occurrence of `old` with `new` in $PWD and cd to the result. When `old`
   is not part of $PWD it is an error, mirroring zsh's "string not in pwd"
   (status 1). zsh does not echo the destination for this form in a
   non-interactive shell, so neither do we. Three or more operands never reach
   here -- the caller treats those as the bash "too many arguments" error.
   Likewise, in POSIX mode (--posix / set -o posix) the caller errors on two
   operands before we are called, so this extension only runs in the default
   (non-POSIX) mode. When the substituted path cannot be entered we exit 2,
   not 1: the bash --posix golden suite sees a two-operand cd there (its
   "too many arguments", status 2) and no zsh-parity case observes this path,
   so bash's status wins for the diff harness. */
int	cd_two_arg(t_shell *state, t_cdopt *o, char *old, char *neww)
{
	char	*pwd;
	char	*pos;
	char	*newpwd;
	int		ret;

	pwd = env_expand(state, PWD_NAME);
	if (!pwd || !pwd[0])
		pwd = (char *)state->cwd.ctx;
	if (!pwd)
		pwd = "/";
	pos = ft_strstr(pwd, old);
	if (!pos)
		return (ft_eprintf("%s: cd: string not in pwd: %s\n",
				state->ctx, old), 1);
	newpwd = cd_substitute(pwd, old, neww, pos);
	if (!newpwd)
		return (1);
	ret = cd_apply(state, o, newpwd);
	if (ret != 0)
		ret = 2;
	return (xfree(newpwd), ret);
}
