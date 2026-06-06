/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   execute_simple_command2.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:30:52 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/09 23:30:52 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "execution_private.h"
#include "sh_alias.h"

/* Splice the alias expansion into argv: alias words come first (from the
   split of the alias value), then the original trailing arguments starting
   at index 1 (we skip argv[0], the un-expanded name).  Each word gets its
   own ft_strdup because free_old_argv will word_free the originals. */
static void	build_alias_argv(t_vec *new_argv, char **words,
				t_executable_cmd *cmd)
{
	char	*dup;
	int		i;

	vec_init(new_argv);
	new_argv->elem_size = sizeof(char *);
	i = 0;
	while (words[i])
	{
		dup = ft_strdup(words[i++]);
		vec_push(new_argv, &dup);
	}
	i = 1;
	while (i < (int)cmd->argv.len)
	{
		dup = ft_strdup(((char **)cmd->argv.ctx)[i++]);
		vec_push(new_argv, &dup);
	}
}

/* The old argv slots may be slab-allocated (word_strndup) rather than
   plain heap, so they must go through word_free not xfree.  The backing
   array (cmd->argv.ctx) is ordinary heap and goes through xfree. */
static void	free_old_argv(t_executable_cmd *cmd)
{
	int	i;

	i = 0;
	while (i < (int)cmd->argv.len)
		word_free(((char **)cmd->argv.ctx)[i++]);
	xfree(cmd->argv.ctx);
}

/* One-level alias expansion: look up argv[0] in the alias table, split
   the value on spaces, and rebuild argv with alias words prepended.  We
   do not recurse (no alias-of-alias) to keep it simple.  If the alias
   value is empty or the lookup fails we leave argv untouched. */
void	apply_alias(t_shell *state, t_executable_cmd *cmd)
{
	char	*name;
	char	*val;
	char	**words;
	t_vec	new_argv;

	if (cmd->argv.len == 0 || !cmd->argv.ctx)
		return ;
	name = ((char **)cmd->argv.ctx)[0];
	if (!name)
		return ;
	val = alias_get(&state->aliases, name);
	if (!val)
		return ;
	words = ft_split(val, ' ');
	if (!words || !words[0])
		return (free_tab(words));
	build_alias_argv(&new_argv, words, cmd);
	free_old_argv(cmd);
	cmd->argv = new_argv;
	free_tab(words);
}

/* After glob/IFS expansion some argv slots can be NULL or a low pointer
   (sentinel values from the slab allocator).  execve(2) would crash or
   behave oddly if it saw those, so we replace any such slot with a fresh
   empty string.  The uintptr_t < 4096 guard catches slab sentinels that
   are not literally NULL but still invalid as C strings. */
void	replace_null_argv_with_empty(t_executable_cmd *cmd)
{
	size_t	i;
	char	*p;

	i = 0;
	while (i < cmd->argv.len)
	{
		p = ((char **)cmd->argv.ctx)[i];
		if (p == NULL || (uintptr_t)p < 4096)
			((char **)cmd->argv.ctx)[i] = ft_strdup("");
		i++;
	}
}

/* Restore the three standard fds from a bak[3] produced by dup().  Used
   by callers that saved before redirecting (exec_builtin, func_call).
   Must be paired with take_backup_fds and called even on error paths to
   avoid leaking the dup'd fds. */
void	restore_fds(int *bak)
{
	dup2(bak[0], 0);
	dup2(bak[1], 1);
	dup2(bak[2], 2);
	close(bak[0]);
	close(bak[1]);
	close(bak[2]);
}
