/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   shell2.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 23:34:03 by marvin            #+#    #+#             */
/*   Updated: 2026/01/09 23:34:03 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "helpers.h"
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* Output buffering is disabled: it lost output when the shell process was
   replaced (exec) or exited directly (exit). */
int	setup_output_buffer(t_shell *state, int *bak)
{
	(void)state;
	*bak = -1;
	return (-1);
}

void	flush_output_buffer(int buf_fd, int bak)
{
	char	tmp[4096];
	ssize_t	n;

	if (buf_fd < 0)
		return ;
	dup2(bak, STDOUT_FILENO);
	close(bak);
	lseek(buf_fd, 0, SEEK_SET);
	n = 1;
	while (n > 0)
	{
		n = read(buf_fd, tmp, sizeof(tmp));
		if (n > 0 && write(STDOUT_FILENO, tmp, n) < 0)
			break ;
	}
	close(buf_fd);
}
