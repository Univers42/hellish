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

static void	free_old_argv(t_executable_cmd *cmd)
{
	int	i;

	i = 0;
	while (i < (int)cmd->argv.len)
		word_free(((char **)cmd->argv.ctx)[i++]);
	free(cmd->argv.ctx);
}

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

void	restore_fds(int *bak)
{
	dup2(bak[0], 0);
	dup2(bak[1], 1);
	dup2(bak[2], 2);
	close(bak[0]);
	close(bak[1]);
	close(bak[2]);
}
