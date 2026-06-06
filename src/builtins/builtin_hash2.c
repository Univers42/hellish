/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_hash2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "cmd_hash.h"
#include "env.h"

/* Remove one entry from the cache (hash -d name). hash_del returns the old
   value so we can free its heap memory; if it returns NULL the name was not
   cached — silent no-op matches bash. */
void	cmd_hash_remove(t_hash *ht, const char *name)
{
	t_cmd_hash_entry	*e;

	e = (t_cmd_hash_entry *)hash_del(ht, name);
	if (!e)
		return ;
	xfree(e->name);
	xfree(e->path);
	xfree(e);
}

/* Flush the entire cache (hash -r). Free + reinit is the simplest correct
   approach: it resets the bucket array without leaving dangling pointers. */
void	cmd_hash_clear(t_hash *ht)
{
	cmd_hash_free(ht);
	cmd_hash_init(ht);
}

/* Print the full cache table in bash `hash` format: hits + path, with the
   "hits\tcommand" header. We walk the raw bucket array rather than using an
   iterator callback to keep the code simple and avoid an extra indirection. */
void	cmd_hash_print_all(t_hash *ht)
{
	size_t				i;
	t_hash_entry		*entries;
	t_cmd_hash_entry	*ce;

	entries = (t_hash_entry *)ht->ctx;
	ft_printf("hits\tcommand\n");
	i = 0;
	while (i < ht->cap)
	{
		if (entries[i].key && entries[i].value)
		{
			ce = (t_cmd_hash_entry *)entries[i].value;
			ft_printf("  %2d\t%s\n", ce->hits, ce->path);
		}
		i++;
	}
}

/* Search PATH for `name` and insert the result into the cache (hash name).
   Used both by the hash builtin and by the executor's "remember this command"
   path. Returns 1 if the executable was not found so the caller can report
   an error; 0 on success. */
int	hash_add_from_path(t_shell *state, const char *name)
{
	char	*path;
	char	**dirs;
	int		perm;

	path = env_expand(state, "PATH");
	if (!path)
		return (1);
	dirs = ft_split(path, ':');
	if (!dirs)
		return (1);
	perm = 0;
	path = exe_path(dirs, (char *)name, &perm);
	free_tab(dirs);
	if (!path)
	{
		ft_eprintf("%s: hash: %s: not found\n", state->ctx, name);
		return (1);
	}
	cmd_hash_insert(&state->cmd_cache, name, path);
	xfree(path);
	return (0);
}

/* Dispatch the two flag forms: -r (reset the cache) and -d name (delete one
   entry). Returns 0 on success, -1 if the flag was not recognised (so
   builtin_hash can fall through to the name-adding path). */
int	handle_hash_flags(t_shell *state, char **av, int ac)
{
	if (ft_strcmp(av[1], "-r") == 0)
	{
		cmd_hash_clear(&state->cmd_cache);
		return (0);
	}
	if (ft_strcmp(av[1], "-d") == 0 && ac >= 3)
	{
		cmd_hash_remove(&state->cmd_cache, av[2]);
		return (0);
	}
	return (-1);
}
