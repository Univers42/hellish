/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_utils2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 15:18:16 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/20 18:43:15 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

void	vec_push_ansi(t_string *v, const char *seq)
{
	vec_push_char(v, '\001');
	vec_push_str(v, seq);
	vec_push_char(v, '\002');
}

int	get_cols(void)
{
	char			*env;
	int				c;
	struct winsize	ws;

	env = getenv("COLUMNS");
	if (env)
	{
		c = atoi(env);
		if (c > 0)
			return (c);
	}
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
		return ((int)ws.ws_col);
	return (80);
}

static void	process_measurement(const char *p, mbstate_t *state, int *total)
{
	int		w;
	wchar_t	wc;
	size_t	len;

	while (*p)
	{
		len = mbrtowc(&wc, p, MB_CUR_MAX, state);
		if (len == (size_t) - 1 || len == (size_t) - 2)
		{
			(*total) += 1;
			p++;
			ft_memset(state, 0, sizeof(*state));
			continue ;
		}
		if (len == 0)
			break ;
		w = wcwidth(wc);
		if (w < 0)
			w = 0;
		(*total) += w;
		p += len;
	}
}

int	measure_width(const char *str)
{
	mbstate_t	state;
	int			total;
	const char	*p = str;

	total = 0;
	ft_memset(&state, 0, sizeof(state));
	process_measurement(p, &state, &total);
	return (total);
}

char	*shorten_path(const char *path, int maxlen)
{
	size_t			plen;
	const char		*p;
	int				kept;
	char			*out;

	plen = ft_strlen(path);
	if ((int)plen <= maxlen)
		return (ft_strdup(path));
	p = path + plen;
	kept = 0;
	while (p > path && kept < maxlen - 4)
	{
		p--;
		if (*p == '/')
			kept = (int)(path + plen - p - 1);
	}
	if (p <= path || kept == 0)
		return (ft_strdup(path + plen - (maxlen - 4)));
	out = ft_strjoin(".../", p + 1);
	return (out);
}
