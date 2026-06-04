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

#include "heredoc_private.h"

/* Special/positional param names are a single char: digit ($1..$9),
** or $#, $?, $$, $!, $@, $* (not recognised by env_len). */
static int	special_param_len(char c)
{
	if (ft_isdigit((unsigned char)c) || ft_strchr("#?$!@*", c))
		return (1);
	return (0);
}

void	expand_dolar(t_shell *state, int *i, t_string *full_file, char *line)
{
	int		len;
	char	*env;

	(*i)++;
	if (line[*i] == '{')
		return (expand_braced(state, i, full_file, line));
	len = env_len(line + *i);
	if (len == 0)
		len = special_param_len(line[*i]);
	if (len)
	{
		if (!full_file->ctx)
			vec_init(full_file);
		env = env_expand_n(state, line + *i, len);
		if (env)
			vec_push_str(full_file, env);
		else
			vec_push_nstr(full_file, "", 0);
	}
	else
		vec_push(full_file, &line[*i]);
	*i += len;
}

void	expand_bs(int *i, t_string *full_file, char *line)
{
	char	tmp;

	if (is_escapable(line[*i]))
		vec_push(full_file, &line[*i]);
	else if (line[*i] != '\n')
	{
		tmp = '\\';
		vec_push(full_file, &tmp);
		tmp = line[*i];
		vec_push(full_file, &tmp);
	}
	(*i)++;
}

static bool	expand_dollar_char(t_shell *state, t_string *ff,
				char *line, int *i)
{
	int	consumed;

	consumed = expand_dollar_sub(state, line + *i,
			(int)ft_strlen(line + *i), ff);
	if (consumed > 0)
	{
		*i += consumed;
		return (true);
	}
	expand_dolar(state, i, ff, line);
	return (true);
}

void	expand_line(t_shell *state, t_string *full_file, char *line)
{
	int		i;
	bool	bs;

	i = 0;
	bs = false;
	while (line[i])
	{
		if (bs)
		{
			expand_bs(&i, full_file, line);
			bs = false;
		}
		else if (line[i] == '$')
			expand_dollar_char(state, full_file, line, &i);
		else if (line[i] == '\\')
			bs = (i++, true);
		else
			vec_push(full_file, &line[i++]);
	}
}
