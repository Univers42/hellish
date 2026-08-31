/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_run.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 12:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 12:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include <sys/wait.h>
#include <unistd.h>

/* Run argv to completion and hand back its EXIT STATUS, leaving stdin,
** stdout and stderr exactly as they are.
**
** update_capture() cannot do this job, and using it here was issue #76's
** second bug. It returns the number of BYTES it read from the child's
** stdout, so a command that succeeds silently and a command that fails
** silently both return 0; and it points the child's stderr at /dev/null,
** which is right for curl (whose noise the user can do nothing with) and
** exactly wrong for sudo, whose "Sorry, try again" is the whole diagnosis.
**
** With `sudo install` behind that, a refused password produced a zero-byte
** capture that read as success, the installed binary was never replaced, and
** the shell printed "✓ updated 2.7.2 → 2.7.6". Reproduced in
** tests/update_sudo_fail_test.py.
**
** Nothing is redirected, so sudo's password prompt and its errors reach the
** terminal the user is sitting at -- the only place they are any use.
*/
int	update_run_visible(char *const argv[])
{
	pid_t	pid;
	int		st;

	pid = fork();
	if (pid < 0)
		return (-1);
	if (pid == 0)
	{
		execvp(argv[0], argv);
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return (-1);
	if (WIFEXITED(st))
		return (WEXITSTATUS(st));
	return (-1);
}
