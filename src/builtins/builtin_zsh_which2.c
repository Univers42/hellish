/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_zsh_which2.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/18 03:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/10/18 03:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "sh_alias.h"
#include "cmd_hash.h"

/* zsh's `which`: the per-name lookup, in zsh's order -- alias, reserved
   word, function, builtin, then the command itself. builtin_zsh_which.c
   holds the option parsing.

   Caveat: zsh prints a function by deparsing it, one command per line;
   this prints the body's source text as written, so `f() { a; b; }` comes
   out as "\ta; b;" where zsh prints "\ta\n\tb". A one-command body matches
   byte for byte. */
static int	which_function(t_shell_func *fn)
{
	char	*body;
	size_t	n;

	if (fn->text)
		body = func_text_trimmed(fn->text);
	else
		body = ft_strdup("");
	if (!body)
		return (1);
	n = ft_strlen(body);
	while (n > 0 && body[n - 1] == ';')
		body[--n] = '\0';
	ft_printf("%s () {\n\t%s\n}\n", fn->name, body);
	xfree(body);
	return (1);
}

/* Caveat: the reserved words are hellish's POSIX set (type_is_keyword);
   zsh 5.x also reserves [[, function, time, repeat, coproc, foreach and
   the typeset family, so `which [[` says "shell built-in command" here
   where zsh says "shell reserved word". */
static int	which_shell(t_shell *state, char *name, bool all)
{
	char			*val;
	t_shell_func	*fn;
	int				hits;

	hits = 0;
	val = alias_get(&state->aliases, name);
	if (val)
		hits += (ft_printf("%s: aliased to %s\n", name, val), 1);
	if ((all || !hits) && type_is_keyword(name))
		hits += (ft_printf("%s: shell reserved word\n", name), 1);
	fn = func_lookup(state, name);
	if ((all || !hits) && fn)
		hits += which_function(fn);
	if ((all || !hits) && builtin_func(state, name))
		hits += (ft_printf("%s: shell built-in command\n", name), 1);
	return (hits);
}

/* The command: a name with a slash is itself when executable; otherwise
   the hashed path, then $PATH. -a asks exe_path() one directory at a time,
   so every match goes through the same is-a-file, is-executable test the
   executor uses and is listed in $PATH order. */
static int	which_paths(t_shell *state, char *name, bool all)
{
	char	*dir[2];
	char	*path;
	int		denied;
	int		hits;
	size_t	i;

	if (ft_strchr(name, '/'))
		return (access(name, X_OK) == 0 && ft_printf("%s\n", name) >= 0);
	path = cmd_hash_lookup(&state->cmd_cache, name);
	if (path && !all)
		return (ft_printf("%s\n", path), 1);
	path_cache_sync(state);
	hits = 0;
	i = 0;
	dir[1] = NULL;
	while (*name && state->path_dirs && state->path_dirs[i] && (all || !hits))
	{
		dir[0] = state->path_dirs[i++];
		path = exe_path(dir, name, &denied);
		if (path)
			hits += (ft_printf("%s\n", path), xfree(path), 1);
	}
	return (hits);
}

/* Returns 0 when something answered to `name`, 1 after "not found". */
int	which_one(t_shell *state, char *name, int flags)
{
	int	hits;

	hits = 0;
	if (!(flags & WHICH_PATH))
		hits = which_shell(state, name, (flags & WHICH_ALL) != 0);
	if (!hits || (flags & WHICH_ALL))
		hits += which_paths(state, name, (flags & WHICH_ALL) != 0);
	if (hits)
		return (0);
	ft_printf("%s not found\n", name);
	return (1);
}
