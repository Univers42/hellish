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

/* -A function: the user-defined function table, sorted -- definition
   order is an implementation detail and bash does not expose it. */
static int	cg_functions(t_shell *st, t_cgopt *o, const char *pfx)
{
	size_t	i;
	t_vec	out;

	out = (t_vec){0};
	i = 0;
	while (i < st->functions.len)
		cg_add(&out, ((t_shell_func *)vec_idx(&st->functions, i++))->name,
			pfx);
	return (cg_flush(o, &out));
}

/* -v: every variable that has a value, the same "set" test `test -v` uses
   so the two cannot disagree about what exists. Sorted: the env vec is in
   assignment order, which no caller should be able to observe. */
static int	cg_vars(t_shell *st, t_cgopt *o, const char *pfx)
{
	size_t	i;
	t_env	*e;
	t_vec	out;

	out = (t_vec){0};
	i = 0;
	while (i < st->env.len)
	{
		e = &((t_env *)st->env.ctx)[i++];
		if (e->key && e->value)
			cg_add(&out, e->key, pfx);
	}
	return (cg_flush(o, &out));
}

/* -b builtins and -k keywords, from the tables the shell itself uses,
   sorted -- hellish's dispatch table is in registration order. */
static int	cg_names(t_cgopt *o, char act, const char *pfx)
{
	static const char	*kw[] = {"if", "then", "else", "elif", "fi", "case",
		"esac", "for", "select", "while", "until", "do", "done", "in",
		"function", "time", "coproc", "{", "}", "!", "[[", "]]", NULL};
	int					i;
	t_vec				out;

	out = (t_vec){0};
	i = 0;
	if (act == 'k')
	{
		while (kw[i])
			cg_add(&out, kw[i++], pfx);
		return (cg_flush(o, &out));
	}
	while (g_builtins[i])
		cg_add(&out, g_builtins[i++], pfx);
	return (cg_flush(o, &out));
}

/* -f / -d / -c: the filesystem and PATH sources. -c offers builtins and
   functions too, because those are commands the user can actually run --
   bash does the same, and a completion list that omits them sends people
   looking for a binary that was never going to exist. */
static int	cg_paths(t_shell *st, t_cgopt *o, const char *pfx)
{
	int	hit;

	hit = cg_glob_paths(o, pfx);
	if (o->act != 'c')
		return (hit);
	hit += cg_names(o, 'b', pfx);
	return (hit + cg_functions(st, o, pfx));
}

/* Route one action letter to its source. */
int	cg_source(t_shell *st, t_cgopt *o, const char *pfx)
{
	if (o->act == 'A')
		return (cg_functions(st, o, pfx));
	if (o->act == 'v')
		return (cg_vars(st, o, pfx));
	if (o->act == 'b' || o->act == 'k')
		return (cg_names(o, o->act, pfx));
	if (o->act == 'a')
		return (cg_aliases(st, o, pfx));
	return (cg_paths(st, o, pfx));
}
