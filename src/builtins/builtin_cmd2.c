/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_cmd2.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "executor.h"

/* return [n]: stop the current function or sourced script with status n
   (default: status of the last command). */
int	builtin_return(t_shell *state, t_vec argv)
{
	char	*cur;
	int		n;

	cur = env_expand(state, "?");
	n = ((cur) ? ft_atoi(cur) : 0);
	if (argv.len >= 2)
		n = ft_atoi(((char **)argv.ctx)[1]);
	state->func_return = 1;
	return (n & 0xFF);
}

static char	*join_from(t_vec argv, size_t from)
{
	char	**av;
	char	*acc;
	char	*tmp;

	av = (char **)argv.ctx;
	acc = ft_strdup("");
	while (from < argv.len)
	{
		tmp = ft_strjoin(acc, av[from]);
		free(acc);
		acc = tmp;
		if (++from < argv.len)
		{
			tmp = ft_strjoin(acc, " ");
			free(acc);
			acc = tmp;
		}
	}
	return (acc);
}

/* command -v name: print how `name` would be resolved (builtin name or path). */
static int	command_v(t_shell *state, char *name)
{
	char	*path;

	if (builtin_func(name))
		return (ft_printf("%s\n", name), 0);
	if (find_cmd_path(state, name, &path) == 0)
		return (ft_printf("%s\n", path), free(path), 0);
	return (1);
}

/* command [-p] [-v|-V] cmd [args]: run cmd (args already split) bypassing
   functions. -p (use a default PATH) is accepted and skipped. */
int	builtin_command(t_shell *state, t_vec argv)
{
	char	**av;
	t_vec	sub;
	size_t	i;
	size_t	start;
	char	*cur;

	av = (char **)argv.ctx;
	start = 1;
	while (start < argv.len && !ft_strcmp(av[start], "-p"))
		start++;
	if (start + 1 < argv.len && (!ft_strcmp(av[start], "-v")
			|| !ft_strcmp(av[start], "-V")))
		return (command_v(state, av[start + 1]));
	if (start >= argv.len)
		return (0);
	vec_init(&sub);
	sub.elem_size = sizeof(char *);
	i = start;
	while (i < argv.len)
		vec_push(&sub, &av[i++]);
	if (builtin_func(av[start]))
		return (i = builtin_func(av[start])(state, sub), free(sub.ctx), i);
	free(sub.ctx);
	cur = join_from(argv, start);
	i = exec_string(state, cur);
	return (free(cur), i);
}
