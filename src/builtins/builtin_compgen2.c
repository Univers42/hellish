/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_compgen2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "env.h"
#include <dirent.h>

extern char	*g_builtins[];

/* The sources behind compgen's action letters. Each walks a table the
   shell already keeps and prints the names that start with the prefix --
   nothing here builds a new registry, because a completion list that is
   assembled separately from the thing it describes goes stale silently. */

/* -A function: the user-defined function table. */
static int	cg_functions(t_shell *st, const char *pfx)
{
	size_t	i;
	int		hit;

	i = 0;
	hit = 0;
	while (i < st->functions.len)
		hit += cg_emit(((t_shell_func *)vec_idx(&st->functions, i++))->name,
				pfx);
	return (hit);
}

/* -v: every variable that has a value, the same "set" test `test -v` uses
   so the two cannot disagree about what exists. */
static int	cg_vars(t_shell *st, const char *pfx)
{
	size_t	i;
	t_env	*e;
	int		hit;

	i = 0;
	hit = 0;
	while (i < st->env.len)
	{
		e = &((t_env *)st->env.ctx)[i++];
		if (e->key && e->value)
			hit += cg_emit(e->key, pfx);
	}
	return (hit);
}

/* -b builtins and -k keywords, from the tables the shell itself uses. */
static int	cg_names(char act, const char *pfx)
{
	static const char	*kw[] = {"if", "then", "else", "elif", "fi", "case",
		"esac", "for", "select", "while", "until", "do", "done", "in",
		"function", "time", "coproc", "{", "}", "!", "[[", "]]", NULL};
	int					i;
	int					hit;

	i = 0;
	hit = 0;
	if (act == 'k')
	{
		while (kw[i])
			hit += cg_emit(kw[i++], pfx);
		return (hit);
	}
	while (g_builtins[i])
		hit += cg_emit(g_builtins[i++], pfx);
	return (hit);
}

/* -f / -d / -c: the filesystem and PATH sources. -c offers builtins and
   functions too, because those are commands the user can actually run --
   bash does the same, and a completion list that omits them sends people
   looking for a binary that was never going to exist. */
static int	cg_paths(t_shell *st, char act, const char *pfx)
{
	int	hit;

	hit = cg_glob_paths(act, pfx);
	if (act != 'c')
		return (hit);
	hit += cg_names('b', pfx);
	return (hit + cg_functions(st, pfx));
}

/* Route one action letter to its source. */
int	cg_source(t_shell *st, char act, const char *pfx)
{
	if (act == 'A')
		return (cg_functions(st, pfx));
	if (act == 'v')
		return (cg_vars(st, pfx));
	if (act == 'b' || act == 'k')
		return (cg_names(act, pfx));
	if (act == 'a')
		return (cg_aliases(st, pfx));
	return (cg_paths(st, act, pfx));
}
