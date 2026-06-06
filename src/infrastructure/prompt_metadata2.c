/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_metadata2.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:35:57 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include <fcntl.h>

/* Detect an active Python/conda virtual environment. CONDA_DEFAULT_ENV wins
   over VIRTUAL_ENV because conda users usually have both set but the conda
   one is the meaningful name. For VIRTUAL_ENV we use only the last path
   component, matching how fish and starship show the env name. */
char	*get_venv_name(void)
{
	const char	*venv;
	const char	*conda;
	const char	*p;

	venv = getenv("VIRTUAL_ENV");
	conda = getenv("CONDA_DEFAULT_ENV");
	if (conda && *conda)
		return (ft_strdup(conda));
	if (venv && *venv)
	{
		p = ft_strrchr(venv, '/');
		if (p)
			return (ft_strdup(p + 1));
		return (ft_strdup(venv));
	}
	return (NULL);
}

/* Fill `buf` with the current local time in HH:MM:SS format. localtime_r is
   the thread-safe variant of localtime; we use it even though the shell is
   single-threaded as a defensive habit. */
void	get_timebuf(char *buf, size_t buflen)
{
	time_t		now;
	struct tm	tm;

	now = time(NULL);
	localtime_r(&now, &tm);
	strftime(buf, buflen, "%H:%M:%S", &tm);
}
