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

/* unset [-f|-v] name ...: remove variables (-v, the default) or functions
   (-f). Like export and cd, must run in the parent shell; a forked child
   unsetting a variable would have zero visible effect. The -f/-v flag is
   optional and only detected when it is a standalone single-char word —
   `-fv` is not parsed to keep the logic simple (bash behaves the same). */
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

/* pwd: print the shell's cached current working directory. We use the cached
   value rather than calling getcwd() every time — that avoids a syscall and
   means `pwd` still works when the directory has been deleted (the kernel
   would return ENOENT from getcwd). The cache is kept up to date by cd and
   pushd/popd. */
int	builtin_pwd(t_shell *state, t_vec argv)
{
	(void)argv;
	if (state->cwd.ctx == NULL)
		ft_eprintf(PWD_ERR_CUR_DIR);
	else
		ft_printf("%s\n", (char *)state->cwd.ctx);
	return (0);
}

/* `cd` with no argument: go to $HOME. We call env_expand rather than
   getenv so the lookup goes through the shell's own environment table. If
   HOME is not set, POSIX mandates an error rather than silently staying put
   or guessing from /etc/passwd. */
int	cd_home(int *e, t_shell *state)
{
	char	*home;

	home = env_expand(state, "HOME");
	if (home == NULL)
		return (ft_eprintf("%s: cd: HOME not set\n", state->ctx), 1);
	*e = chdir(home);
	return (0);
}

/* cd [-L|-P] [dir]: change the working directory. Must run in the parent
   process — that is the whole reason cd is a builtin at all; a forked copy
   would chdir into oblivion and the parent shell would never notice.

   `cd -` switches to $OLDPWD (prints the new path, bash-style). After a
   successful chdir we call x_getcwd() to get the canonical path and refresh
   both state->cwd and the environment variables PWD/OLDPWD so `$PWD`
   expansions stay correct. On ENOENT we emit the strerror message rather
   than a hardcoded string so internationalised kernels stay readable. */
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
