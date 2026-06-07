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

/* Resolve a single operand to a heap path to hand chdir. A plain name is
   first looked up through CDPATH (which, on a hit via a non-empty component,
   sets o->echo so the destination is printed bash-style); otherwise it is
   used as-is. Returns a freshly-allocated string the caller must xfree. */
static char	*cd_resolve(t_shell *state, char *op, t_cdopt *o)
{
	char	*found;

	found = cd_cdpath(state, op, &o->echo);
	if (found)
		return (found);
	return (ft_strdup(op));
}

/* The 0-or-1-operand cases. No operand -> $HOME. Empty operand -> POSIX
   no-op success (bash stays put and returns 0). "-" -> $OLDPWD. Everything
   else is resolved (CDPATH) then handed to cd_apply, which does the
   logical/physical chdir and refreshes $PWD/$OLDPWD. */
static int	cd_one(t_shell *state, t_cdopt *o, char *op)
{
	char	*target;
	int		ret;

	target = NULL;
	if (!op)
	{
		if (cd_target_home(state, &target))
			return (1);
	}
	else if (!op[0])
		return (0);
	else if (!ft_strcmp(op, "-"))
	{
		if (cd_target_dash(state, &target, o))
			return (1);
	}
	else
		target = cd_resolve(state, op, o);
	if (!target)
		return (1);
	ret = cd_apply(state, o, target);
	return (xfree(target), ret);
}

/* cd [-L|-P] [-e] [-@] [dir] (plus the zsh-style `cd old new`). Must run in
   the parent process -- the whole reason cd is a builtin: a forked copy
   would chdir into oblivion and the parent shell would never notice.
   Options are parsed first (invalid option -> usage + exit 2), then operands
   are counted: 0/1 go through cd_one, exactly 2 trigger the substitution
   extension, 3+ are the bash "too many arguments" error. In POSIX mode
   (--posix / set -o posix) the extension is off, so 2 operands error too. */
int	builtin_cd(t_shell *state, t_vec argv)
{
	t_cdopt	o;
	size_t	first;
	char	*op0;
	char	*op1;
	int		n;

	o = (t_cdopt){0};
	n = cd_parse_opts(state, argv, &o, &first);
	if (n)
		return (n);
	n = cd_collect_ops(argv, first, &op0, &op1);
	if (n >= 3 || (n == 2 && state->opt_posix))
		return (ft_eprintf("%s: cd: too many arguments\n", state->ctx), 1);
	if (n == 2)
		return (cd_two_arg(state, &o, op0, op1));
	if (n == 1)
		return (cd_one(state, &o, op0));
	return (cd_one(state, &o, NULL));
}
