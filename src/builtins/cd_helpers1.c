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
