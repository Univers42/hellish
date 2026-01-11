/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heredoc3.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:32:09 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:32:09 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
# include "redir.h"

void	expand_dolar(t_shell *state, int *i, t_string *full_file, char *line)
{
	int		len;
	char	*env;

	(*i)++;
	len = env_len(line + *i);
	if (len)
	{
		env = env_expand_n(state, line + *i, len);
		if (env)
			vec_push_str(full_file, env);
	}
	else
	{
		char tmp = line[*i];
		vec_push(full_file, &tmp);
	}
	*i += len;
}

void	expand_bs(int *i, t_string *full_file, char *line)
{
	if (is_escapable(line[*i]))
	{
		char tmp = line[*i];
		vec_push(full_file, &tmp);
	}
	else if (line[*i] != '\n')
	{
		char tmp = '\\';
		vec_push(full_file, &tmp);
		tmp = line[*i];
		vec_push(full_file, &tmp);
	}
	(*i)++;
}

void	expand_line(t_shell *state, t_string *full_file, char *line)
{
	int		i;
	bool	bs;

	i = 0;
	bs = 0;
	while (line[i])
	{
		if (bs)
		{
			expand_bs(&i, full_file, line);
			bs = false;
			continue ;
		}
		if (line[i] == '$')
		{
			expand_dolar(state, &i, full_file, line);
			continue ;
		}
		else if (line[i] == '\\')
			bs = true;
		else
		{
			char tmp = line[i];
			vec_push(full_file, &tmp);
		}
		i++;
	}
}
