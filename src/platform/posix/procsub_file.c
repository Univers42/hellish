/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   procsub_file.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 12:30:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 12:30:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "shell.h"
#include "libft.h"
#include "sys.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

int		exec_string(t_shell *state, char *content);
void	reset_traps_child(t_shell *state);

/* zsh's `=(cmd)` -- process substitution to a REAL TEMPORARY FILE.
**
**     local tmp_name==(:)
**
** is oh-my-zsh's extract asking for a fresh unique name, and it is the only
** thing in that plugin left after the glob qualifiers.
**
** WHY IT IS NOT `<(cmd)`. Both substitute a path, but `<(cmd)` gives
** /dev/fd/N, which is a PIPE: it can be read once, front to back, and
** cannot be seeked, stat'd for a size, or reopened. `=(cmd)` gives a file
** on disk, which can be all of those. A consumer that seeks -- an archiver,
** an editor, anything that reads a file twice -- works with one and fails
** with the other, so implementing `=()` as `<()` would produce a path that
** works until it does not.
**
** The file is created, written, and left for the SESSION rather than
** unlinked at once: the whole point is to hand the path to something that
** will open it later. zsh cleans up when the enclosing command finishes;
** here it goes on the same proc-sub list everything else uses, so it is
** removed at the points that list is already drained.
*/

/* Run `cmd` with stdout on `fd`, in a child, and wait for it.  Waiting is
   the difference from the pipe forms: the file has to be COMPLETE before
   the path is handed over, because the consumer will open it by name and
   would otherwise read an empty or half-written file. */
static int	run_into_fd(t_shell *state, const char *cmd, int fd)
{
	pid_t	pid;
	int		st;

	pid = fork();
	if (pid < 0)
		return (-1);
	if (pid == 0)
	{
		if (dup2(fd, STDOUT_FILENO) < 0)
			_exit(1);
		close(fd);
		reset_traps_child(state);
		_exit(exec_string(state, (char *)cmd));
	}
	close(fd);
	waitpid(pid, &st, 0);
	return (0);
}

/* Create the temp file and record it so the session cleans it up. Returns
   the path (owned by the caller), or NULL. */
char	*create_procsub_file(t_shell *state, const char *cmd)
{
	t_procsub_entry	entry;
	char			buf[64];
	int				fd;

	ft_snprintf(buf, sizeof(buf), "%s/hsh%d-%d", TMP_DIR,
		(int)getpid(), (int)state->proc_subs.len);
	fd = open(buf, O_RDWR | O_CREAT | O_EXCL, 0600);
	if (fd < 0)
		return (NULL);
	if (cmd && *cmd && run_into_fd(state, cmd, fd) < 0)
		return (close(fd), unlink(buf), NULL);
	if (!cmd || !*cmd)
		close(fd);
	entry.pid = -1;
	entry.fd = -1;
	entry.path = ft_strdup(buf);
	if (!state->proc_subs.elem_size)
		state->proc_subs.elem_size = sizeof(t_procsub_entry);
	vec_push(&state->proc_subs, &entry);
	return (ft_strdup(buf));
}
