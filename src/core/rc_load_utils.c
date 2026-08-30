/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rc_load_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/29 12:52:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/29 12:52:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "env.h"

/* Small helpers for the config load path. Split from rc_load.c only because
   the 42 norm caps a file at five functions. */

bool	str_ends_with(const char *s, const char *suf)
{
	size_t	ls;
	size_t	lf;

	ls = ft_strlen(s);
	lf = ft_strlen(suf);
	if (lf > ls)
		return (false);
	return (ft_strcmp(s + (ls - lf), suf) == 0);
}

char	*path_join(const char *a, const char *b)
{
	char	*mid;
	char	*out;

	mid = ft_strjoin(a, "/");
	if (!mid)
		return (NULL);
	out = ft_strjoin(mid, b);
	xfree(mid);
	return (out);
}

/* Insertion sort over char* from index `from`. n is tiny (the files in one
   directory) and this keeps the ordering guarantee in one obvious place. */
void	sort_strvec(t_vec *v, size_t from)
{
	char	**a;
	char	*tmp;
	size_t	i;
	size_t	j;

	a = (char **)v->ctx;
	i = from + 1;
	while (i < v->len)
	{
		tmp = a[i];
		j = i;
		while (j > from && ft_strcmp(a[j - 1], tmp) > 0)
		{
			a[j] = a[j - 1];
			j--;
		}
		a[j] = tmp;
		i++;
	}
}

/* $XDG_CONFIG_HOME/hellish, else $HOME/.config/hellish. Honouring XDG is what
   lets a user relocate the whole tree, which is the same property BASH_SOURCE
   gives an individual plugin. */
char	*xdg_config_hellish(t_shell *state, const char *home)
{
	char	*xdg;
	char	*base;
	char	*out;

	xdg = env_expand(state, "XDG_CONFIG_HOME");
	if (xdg && *xdg)
		return (path_join(xdg, "hellish"));
	if (!home || !*home)
		return (NULL);
	base = path_join(home, ".config");
	if (!base)
		return (NULL);
	out = path_join(base, "hellish");
	xfree(base);
	return (out);
}
