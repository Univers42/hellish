/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   func_scope2.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "env.h"
#include "execution_private.h"
#include "ft_builtins.h"

int		try_unset(t_shell *state, char *key);

/* Roll back temporary NAME=val assignments after a builtin or function
   returns.  We iterate in reverse (LIFO) so nested saves unwind in the
   correct order.  The saves vec backing is freed with xfree after the
   loop because restore_one does NOT free the vec itself. */
void	restore_temp_assigns(t_shell *state, t_vec *saves)
{
	size_t	i;

	i = saves->len;
	while (i > 0)
		restore_one(state, (t_scope_save *)vec_idx(saves, --i));
	xfree(saves->ctx);
}

/* Write the initial value for a `local` variable.  If no '=' was given
   the variable is set to "" (not unset) so `${local_var:-default}` does
   not fall through to the default.  Under `set -a` (allexport) a valued
   `local NAME=v` is exported, exactly like bash and dash; a valueless
   `local NAME` stays unexported because bash leaves it unset — exporting
   our "" placeholder would put NAME= in the environment where bash shows
   nothing.  The key string is owned by the env entry after env_create;
   do not xfree it here. */
static void	local_set_var(t_shell *state, char *key, char *eq)
{
	if (eq)
		env_set(&state->env,
			env_create(key, ft_strdup(eq + 1), state->opt_allexport));
	else
		env_set(&state->env, env_create(key, ft_strdup(""), false));
}

/* Consume leading option words and report the first operand.
**
** local did NO option parsing at all: it started at argv[1] and strdup'd
** every word as a variable name. So `local -n ref=var` created a shell
** variable literally called "-n" and bound `ref` to the STRING "var" --
** silently, with status 0. Issue #71 item 5.3 rates that worse than
** unimplemented, and rightly: a nameref that quietly yields the name instead
** of the value corrupts data rather than failing.
**
** `n` is reported through *term because it changes what the operands MEAN
** (a target name, not a value). The rest of declare's letters are consumed
** and ignored exactly as declare consumes them, so `local -r x=1` is a
** normal local rather than a variable named "-r". */
static size_t	local_opts(t_vec argv, char *term)
{
	size_t	i;
	char	*w;

	*term = 0;
	i = 1;
	while (i < argv.len)
	{
		w = ((char **)argv.ctx)[i];
		if (w[0] != '-' || !w[1])
			break ;
		if (ft_strchr(w, 'n'))
			*term = 'n';
		i++;
	}
	return (i);
}

/* local -n ref[=target]: record the nameref attribute the way declare -n
   does (env_attr.c owns that table), but scope_save first so the binding is
   undone when the function returns -- which is the whole point of `local`. */
static int	local_nameref(t_shell *state, t_vec argv, size_t i)
{
	char	*eq;
	char	*name;

	while (i < argv.len)
	{
		eq = ft_strchr(((char **)argv.ctx)[i], '=');
		if (eq)
			name = ft_strndup(((char **)argv.ctx)[i],
					eq - ((char **)argv.ctx)[i]);
		else
			name = ft_strdup(((char **)argv.ctx)[i]);
		scope_save(state, name);
		attr_set(state, name, 'n', eq ? eq + 1 : "");
		xfree(name);
		i++;
	}
	return (0);
}

/* local [-n] name[=value] ... : make each name local to the current
   function. */
int	builtin_local(t_shell *state, t_vec argv)
{
	char	**av;
	size_t	i;
	char	*eq;
	char	*key;
	char	term;

	av = (char **)argv.ctx;
	if (state->func_depth <= 0)
	{
		ft_eprintf("%s: local: can only be used in a function\n", state->ctx);
		return (1);
	}
	i = local_opts(argv, &term);
	if (term == 'n')
		return (local_nameref(state, argv, i));
	while (i < argv.len)
	{
		eq = ft_strchr(av[i], '=');
		if (eq)
			key = ft_strndup(av[i], eq - av[i]);
		else
			key = ft_strdup(av[i]);
		scope_save(state, key);
		local_set_var(state, key, eq);
		i++;
	}
	return (0);
}
