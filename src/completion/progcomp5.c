/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   progcomp5.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "progcomp_private.h"
#include "env.h"
#include "zle.h"
#include "ft_builtins.h"

/* The completion call, in a sandbox.
**
** The command pc_call_str builds sets `set -f`, IFS=newline and the COMP_*
** variables, and never puts any of it back: it ran in the child the line
** was read in, "whose option state dies with the line". It runs in the
** shell now, and without this a TAB left the session with globbing off
** (or on, after `set +f` undid the user's own `set -f`) and IFS a
** newline. What changes is saved and restored around the call, and the
** COMP_* variables are unset afterwards, as bash does. */

static void	pc_sandbox_enter(t_shell *st, t_pc_sandbox *sb)
{
	t_env	*e;

	sb->ifs = NULL;
	e = env_get(&st->env, "IFS");
	if (e && e->value)
		sb->ifs = ft_strdup(e->value);
	sb->noglob = st->opt_noglob;
}

static void	pc_sandbox_leave(t_shell *st, t_pc_sandbox *sb)
{
	static const char	*gone[] = {"COMP_LINE", "COMP_POINT", "COMP_WORDS",
		"COMP_CWORD", "COMPREPLY", NULL};
	int					i;

	if (sb->ifs)
		env_set(&st->env, env_create(ft_strdup("IFS"), sb->ifs, false));
	else
		try_unset(st, "IFS");
	sb->ifs = NULL;
	st->opt_noglob = sb->noglob;
	i = -1;
	while (gone[++i])
		try_unset(st, (char *)gone[i]);
}

/* Run the spec for the word being completed and collect its matches --
   COMPREPLY is read before the sandbox takes it away. */
bool	pc_build(t_shell *st, t_compspec *c, const char *text, int start)
{
	char			*cmd;
	t_pc_sandbox	sb;
	bool			got;

	pc_reset();
	cmd = pc_call_str(c, text, start);
	if (!cmd)
		return (false);
	pc_sandbox_enter(st, &sb);
	rl_shell_exec(st, cmd);
	xfree(cmd);
	got = pc_collect(st);
	pc_sandbox_leave(st, &sb);
	return (got);
}
