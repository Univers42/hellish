/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_read.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                                            */
/*   Created: 2026/03/15 02:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/15 02:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <unistd.h>

static char	*read_one_line(void)
{
	char		ch;
	t_string	buf;
	ssize_t		n;

	vec_init(&buf);
	buf.elem_size = 1;
	n = read(STDIN_FILENO, &ch, 1);
	while (n > 0 && ch != '\n')
	{
		vec_push(&buf, &ch);
		n = read(STDIN_FILENO, &ch, 1);
	}
	if (n <= 0 && buf.len == 0)
		return (free(buf.ctx), NULL);
	ch = '\0';
	vec_push(&buf, &ch);
	return ((char *)buf.ctx);
}

static void	assign_words(t_shell *state, char *line, t_vec argv)
{
	size_t	i;
	char	*name;
	char	*p;

	i = 1;
	p = line;
	while (i < argv.len)
	{
		name = ((char **)argv.ctx)[i];
		while (*p == ' ' || *p == '\t')
			p++;
		if (i == argv.len - 1 || *p == '\0')
			break ;
		line = p;
		while (*p && *p != ' ' && *p != '\t')
			p++;
		if (*p)
			*(p++) = '\0';
		env_set(&state->env,
			env_create(ft_strdup(name), ft_strdup(line), false));
		i++;
	}
	name = ((char **)argv.ctx)[i];
	env_set(&state->env,
		env_create(ft_strdup(name), ft_strdup(p), false));
}

int	builtin_read(t_shell *state, t_vec argv)
{
	char	*line;

	line = read_one_line();
	if (!line)
		return (1);
	if (argv.len <= 1)
		env_set(&state->env,
			env_create(ft_strdup("REPLY"), line, false));
	else
	{
		assign_words(state, line, argv);
		free(line);
	}
	return (0);
}
