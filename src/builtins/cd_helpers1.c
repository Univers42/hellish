/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd.c                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:21 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:30:21 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

/* Perform the chdir for `cd`. Handles three cases:
     1. No real arg -> cd to $HOME via cd_home.
     2. Arg is "-"  -> cd to $OLDPWD (and print the new path, bash-style).
     3. Anything else -> chdir(arg) directly.
   The caller checks *e for -1 to detect chdir failure; we never call
   strerror here so the error message can include the errno-string later. */
int	cd_do_chdir(t_shell *state, t_vec argv, int *e)
{
	char	*oldpwd;
	char	*path;

	oldpwd = env_expand(state, OLDPWD_NAME);
	path = get_first_real_arg(argv);
	if (!path)
		return (cd_home(e, state));
	if (!ft_strcmp("-", path))
	{
		if (oldpwd == NULL)
		{
			ft_eprintf(OLDPWD_NO_SET, state->ctx);
			return (1);
		}
		ft_printf("%s\n", oldpwd);
		*e = chdir(oldpwd);
	}
	else
		*e = chdir(path);
	return (0);
}

/* Update state->cwd after a successful cd. When x_getcwd() returned a path,
   use it (the canonical form). When it returned NULL (unlikely but possible
   in a deleted-directory edge case), fall back to appending the argument to
   the old cwd so at least something shows up in the prompt. */
void	cd_refresh_cwd(t_shell *state, t_vec argv, char *cwd)
{
	if (cwd)
	{
		state->cwd.len = 0;
		vec_push_str(&state->cwd, cwd);
	}
	else
	{
		ft_eprintf(CD_ERROR);
		if (!vec_str_ends_with_str(&state->cwd, "/") && state->cwd.ctx)
			vec_push_str(&state->cwd, "/");
		if (argv.len == 2)
			vec_push_str(&state->cwd, ((char **)argv.ctx)[1]);
	}
}
