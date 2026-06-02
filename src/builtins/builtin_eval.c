/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_eval.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/02 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "executor.h"
#include <fcntl.h>

/* Join argv[1..] with single spaces into one allocated string. */
static char	*join_args(t_vec argv, size_t from)
{
	char	**av;
	char	*acc;
	char	*tmp;
	size_t	i;

	av = (char **)argv.ctx;
	acc = ft_strdup("");
	i = from;
	while (i < argv.len)
	{
		tmp = ft_strjoin(acc, av[i]);
		free(acc);
		acc = tmp;
		if (++i < argv.len)
		{
			tmp = ft_strjoin(acc, " ");
			free(acc);
			acc = tmp;
		}
	}
	return (acc);
}

/* eval: concatenate the arguments and execute the result as shell input. */
int	builtin_eval(t_shell *state, t_vec argv)
{
	char	*joined;
	int		status;

	if (argv.len < 2)
		return (0);
	joined = join_args(argv, 1);
	status = exec_string(state, joined);
	free(joined);
	return (status);
}

/* . file (a.k.a. source): read `file` and execute in this shell. */
int	builtin_source(t_shell *state, t_vec argv)
{
	char		**av;
	t_string	buf;
	char		*content;
	int			fd;
	int			status;

	av = (char **)argv.ctx;
	if (argv.len < 2)
		return (ft_eprintf("%s: .: filename argument required\n",
				state->ctx), 2);
	fd = open(av[1], O_RDONLY);
	if (fd < 0)
		return (ft_eprintf("%s: .: %s: No such file or directory\n",
				state->ctx, av[1]), 1);
	vec_init(&buf);
	buf.elem_size = 1;
	vec_append_fd(fd, &buf);
	close(fd);
	content = ft_strndup((char *)buf.ctx, buf.len);
	free(buf.ctx);
	status = exec_string(state, content);
	free(content);
	return (status);
}
