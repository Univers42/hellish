/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history_expand.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/16 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "history.h"
#include "history_private.h"
#include "libft.h"
#include <stdlib.h>

char	*hist_entry_at(t_shell *state, int idx)
{
	t_vec	*cmds;

	cmds = &state->hist.hist_cmds;
	if (idx < 0)
		idx = (int)cmds->len + idx;
	if (idx < 0 || idx >= (int)cmds->len)
		return (NULL);
	return (((char **)cmds->ctx)[idx]);
}

char	*hist_last(t_shell *state)
{
	return (hist_entry_at(state, -1));
}

char	*hist_search_prefix(t_shell *state, const char *prefix, int len)
{
	int	i;

	i = (int)state->hist.hist_cmds.len - 1;
	while (i >= 0)
	{
		if (ft_strncmp(((char **)state->hist.hist_cmds.ctx)[i],
			prefix, len) == 0)
			return (((char **)state->hist.hist_cmds.ctx)[i]);
		i--;
	}
	return (NULL);
}

/* !?str[?] : most recent history entry containing `str`. */
char	*hist_search_contains(t_shell *state, const char *needle, int len)
{
	int		i;
	char	*nd;
	char	*cmd;

	nd = ft_strndup(needle, len);
	if (!nd)
		return (NULL);
	i = (int)state->hist.hist_cmds.len - 1;
	while (i >= 0)
	{
		cmd = ((char **)state->hist.hist_cmds.ctx)[i];
		if (ft_strnstr(cmd, nd, ft_strlen(cmd)))
			return (free(nd), cmd);
		i--;
	}
	return (free(nd), NULL);
}
