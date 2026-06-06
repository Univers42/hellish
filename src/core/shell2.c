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

/* Output buffering was removed: the tmpfile approach it replaced swallowed
   output whenever the shell process exited via exec or the exit builtin
   before we could flush (the tmpfile just disappeared). Now we return -1 to
   signal "no buffer active" and flush_output_buffer() is a safe no-op when
   buf_fd < 0. Keeping the pair of functions lets us switch buffering back on
   per-fd in the future without touching the call sites in the REPL. */
int	setup_output_buffer(t_shell *state, int *bak)
{
	(void)state;
	*bak = -1;
	return (-1);
}

/* Drain the tmpfile buffer back to the real stdout in 4 KB chunks. With
   buf_fd == -1 (buffering disabled) we return immediately. The lseek to
   offset 0 rewinds the tmpfile before reading -- easy to forget and produces
   a silent no-output bug. dup2 restores the original stdout fd first so the
   write goes to the right place even if the shell inherited a weird setup. */
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
