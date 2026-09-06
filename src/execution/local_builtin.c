/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   local_builtin.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "ft_builtins.h"
#include "env.h"

/* The `local` builtin. Split out of func_scope2.c only because the 42 norm
   caps a file at five functions; scope_save/scope_leave in func_scope.c are
   still what makes any of this actually local. */

void	local_set_var(t_shell *state, char *key, char *eq);

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
		if (eq)
			attr_set(state, name, 'n', eq + 1);
		else
			attr_set(state, name, 'n', "");
		xfree(name);
		i++;
	}
	return (0);
}

/* Is a bare `local name` a re-declaration of something ALREADY local at
   this depth? bash keeps the value then -- and bash-completion counts on
   it: `local "$2" "$3" "$4"` re-locals the caller-named out-parameters
   its helpers have already filled, so wiping them emptied every word
   list between assembly and _comp_upvars (issue #105, wave 2). Only the
   valueless form is a no-op; `local x=v` still assigns. */
static bool	local_already(t_shell *state, const char *name, char *eq)
{
	size_t			i;
	t_scope_save	*s;

	if (eq || state->local_saves.elem_size == 0)
		return (false);
	i = 0;
	while (i < state->local_saves.len)
	{
		s = (t_scope_save *)state->local_saves.ctx + i;
		if (s->depth == state->func_depth
			&& ft_strcmp(s->key, name) == 0)
			return (true);
		i++;
	}
	return (false);
}

/* The ordinary operand loop: name, or name=value. */
static int	local_plain(t_shell *state, t_vec argv, size_t i)
{
	char	**av;
	char	*eq;
	char	*key;

	av = (char **)argv.ctx;
	while (i < argv.len)
	{
		eq = ft_strchr(av[i], '=');
		if (eq)
			key = ft_strndup(av[i], eq - av[i]);
		else
			key = ft_strdup(av[i]);
		if (local_already(state, key, eq))
		{
			xfree(key);
			i++;
			continue ;
		}
		scope_save(state, key);
		local_set_var(state, key, eq);
		i++;
	}
	return (0);
}

/* local [-n] name[=value] ... : make each name local to the current
   function.  Outside one, zsh's local is typeset -- a plain declaration
   -- and oh-my-zsh themes write `local` at file scope (fino, agnoster),
   so in the dialect it goes to declare instead of failing. */
int	builtin_local(t_shell *state, t_vec argv)
{
	size_t	i;
	char	term;

	if (state->func_depth <= 0)
	{
		if (zsh_mode(state))
			return (builtin_declare(state, argv));
		ft_eprintf("%s: local: can only be used in a function\n", state->ctx);
		return (1);
	}
	i = local_opts(argv, &term);
	if (term == 'n')
		return (local_nameref(state, argv, i));
	return (local_plain(state, argv, i));
}
