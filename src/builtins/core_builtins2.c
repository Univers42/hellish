/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   core_builtins2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:16:24 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/27 16:16:39 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

int	builtin_pwd(t_shell *state, t_vec argv)
{
	(void)argv;
	if (state->cwd.ctx == NULL)
		ft_eprintf(PWD_ERR_CUR_DIR);
	else
		ft_printf("%s\n", (char *)state->cwd.ctx);
	return (0);
}

int	cd_home(int *e, t_shell *state)
{
	char	*home;

	home = env_expand(state, "HOME");
	if (home == NULL)
		return (ft_eprintf("%s: cd: HOME not set\n", state->ctx), 1);
	*e = chdir(home);
	return (0);
}

int	builtin_cd(t_shell *state, t_vec argv)
{
	char	*cwd;
	int		e;
	char	*arg;

	e = 0;
	if (check_args(argv))
	{
		ft_eprintf("%s: %s: too many arguments\n", state->ctx,
			((char **)argv.ctx)[0]);
		return (1);
	}
	if (cd_do_chdir(state, argv, &e))
		return (1);
	arg = get_first_real_arg(argv);
	if (!arg)
		arg = "";
	if (e == -1)
	{
		ft_eprintf("%s: %s: %s: %s\n", state->ctx,
			((char **)argv.ctx)[0], arg, strerror(errno));
		return (1);
	}
	cwd = getcwd(NULL, 0);
	return (cd_refresh_cwd(state, argv, cwd), free(cwd),
		update_pwd_vars(state), 0);
}
