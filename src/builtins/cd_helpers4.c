/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cd_helpers4.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/07 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/07 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>

/* Store a new working-directory string into state->cwd (the prompt/$PWD
   source of truth), reusing the buffer. */
static void	cd_set_cwd(t_shell *state, char *path)
{
	state->cwd.len = 0;
	vec_push_str(&state->cwd, path);
}

/* chdir failed: emit "<sh>: cd: <dir>: <errno-string>" so internationalised
   kernels stay readable, and report failure. */
static int	cd_err_path(t_shell *state, const char *disp)
{
	ft_eprintf("%s: cd: %s: %s\n", state->ctx, disp, strerror(errno));
	return (1);
}

/* Logical (-L) move: chdir to the textually-canonicalized path so ".." undoes
   the path as typed, then record that same logical path as the new cwd -- not
   what getcwd() would resolve symlinks to. Mirrors bash's default. */
static int	cd_do_logical(t_shell *state, char *target)
{
	char	*logical;

	logical = cd_logical_path(state, target);
	if (!logical)
		return (1);
	if (chdir(logical) == -1)
		return (xfree(logical), cd_err_path(state, target));
	cd_set_cwd(state, logical);
	return (xfree(logical), 0);
}

/* Physical (-P) move: chdir straight to the operand and let the kernel resolve
   symlinks, then read the canonical path back with getcwd(). */
static int	cd_do_physical(t_shell *state, char *target)
{
	char	*cwd;

	if (chdir(target) == -1)
		return (cd_err_path(state, target));
	cwd = x_getcwd();
	if (cwd)
		cd_set_cwd(state, cwd);
	return (xfree(cwd), 0);
}

/* Perform the directory change (logical or physical per -L/-P), then -- on
   success -- echo the destination when asked (CDPATH hit, `cd -`, `cd a b`)
   and rotate $PWD/$OLDPWD.  The operand is already fully resolved.
     This is the single point at which a cd SUCCEEDS, which is why zsh's
   chpwd hooks fire from here rather than from builtin_cd: `cd -`, `cd a b`
   and a CDPATH hit all arrive through this one function, and a hook that
   ran for some of those and not others would be worse than none. */
int	cd_apply(t_shell *state, t_cdopt *o, char *target)
{
	int	ret;

	if (o->physical)
		ret = cd_do_physical(state, target);
	else
		ret = cd_do_logical(state, target);
	if (ret)
		return (1);
	if (o->echo)
		ft_printf("%s\n", (char *)state->cwd.ctx);
	update_pwd_vars(state);
	if (!o->quiet)
		run_chpwd_hooks(state);
	return (0);
}
