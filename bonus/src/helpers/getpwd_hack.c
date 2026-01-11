/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   getpwd_hack.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:33:36 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:33:36 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include <fcntl.h>
#include <unistd.h>
# include "helpers.h"

char	*getpid_hack(void)
{
	int			fd;
	t_string	file;
	char		*ret;
	char		**temp;

	fd = open("/proc/self/stat", O_RDONLY);
	if (fd < 0)
	{
		warning_error("Cannot get PID.");
		return (0);
	}
	vec_init(&file);
	vec_append_fd(fd, &file);
	close(fd);
	temp = ft_split((char *)file.ctx, ' ');
	free(file.ctx);
	ret = ft_strdup(temp[0]);
	free_tab(temp);
	return (ret);
}
