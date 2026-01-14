/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:35:00 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:35:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "env.h"

void	free_tab(char **tab)
{
	int	i;

	i = 0;
	while (tab[i])
		free(tab[i++]);
	free(tab);
}

// 0 on success
int	write_to_file(char *str, int fd)
{
	int	wrote_total;
	int	wrote;
	int	len;

	if (!str)
		return (1);
	len = ft_strlen(str);
	if (len == 0)
		return (0);
	wrote_total = 0;
	while (wrote_total != len)
	{
		wrote = write(fd, str + wrote_total, len - wrote_total);
		if (wrote < 0)
			return (1);
		wrote_total += wrote;
	}
	return (0);
}

void	forward_exit_status(t_exe_res res)
{
	ft_assert(res.status != -1);
	if (res.c_c)
	{
		default_signal_handlers();
		raise(SIGINT);
		_exit(128 + SIGINT);
	}
	_exit(res.status);
}

void	set_cmd_status(t_shell *state, t_exe_res res)
{
	state->last_cmd_status_res = res;
	free(state->last_cmd_status_s);
	state->last_cmd_status_s = ft_itoa(res.status);
}

