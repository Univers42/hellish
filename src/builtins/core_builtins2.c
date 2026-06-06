/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   core_builtins2.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/23 14:16:24 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 01:44:36 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"

int	builtin_unset(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	int		fmode;

	av = (char **)argv.ctx;
	i = 1;
	fmode = 0;
	if (i < argv.len && av[i][0] == '-'
		&& (av[i][1] == 'f' || av[i][1] == 'v')
		&& !av[i][2])
		fmode = (av[i++][1] == 'f');
	while (i < argv.len)
	{
		if (fmode)
			unset_function(state, av[i]);
		else
			try_unset(state, av[i]);
		i++;
	}
	return (0);
}

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
	cwd = x_getcwd();
	return (cd_refresh_cwd(state, argv, cwd), xfree(cwd),
		update_pwd_vars(state), 0);
}
