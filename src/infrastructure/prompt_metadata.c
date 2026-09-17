/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_metadata.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/20 16:35:57 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/02 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include <fcntl.h>
#include <sys/stat.h>

/* Turn a .git/HEAD payload into a branch name (or short SHA when detached).
   Which of the two it was is left in git_detached_cell for vcs_info, so
   that question costs no second read of the same file. */
static char	*parse_head(const char *buf)
{
	int	len;

	*git_detached_cell() = (ft_strncmp(buf, "ref: ", 5) != 0);
	if (ft_strncmp(buf, "ref: refs/heads/", 16) == 0)
	{
		len = 0;
		while (buf[16 + len] && buf[16 + len] != '\n' && buf[16 + len] != '\r')
			len++;
		return (ft_strndup(buf + 16, len));
	}
	if (ft_isalnum((unsigned char)buf[0]))
		return (ft_strndup(buf, 7));
	return (NULL);
}

static char	*read_head_file(const char *path)
{
	char	buf[256];
	int		fd;
	ssize_t	n;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (NULL);
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n <= 0)
		return (NULL);
	buf[n] = '\0';
	return (parse_head(buf));
}

/* The git directory of the repository rooted at `root`, into `out`:
** <root>/.git itself, or the directory a `gitdir:` file there names
** (submodules and linked worktrees). False when neither can be read.
**
** A relative gitdir is relative to the directory holding the file, which
** is `root` -- and submodules write theirs relative (`gitdir:
** ../.git/modules/x`). It used to be opened as written, i.e. relative to
** the CWD, so the branch vanished from the prompt in any subdirectory of
** a submodule. */
bool	git_dir_for(const char *root, char *out, size_t cap)
{
	char		buf[PATH_MAX];
	int			fd;
	ssize_t		n;
	struct stat	st;

	ft_strlcpy(out, root, cap);
	ft_strlcat(out, "/.git", cap);
	if (stat(out, &st) == 0 && S_ISDIR(st.st_mode))
		return (true);
	fd = open(out, O_RDONLY);
	if (fd < 0)
		return (false);
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n <= 8 || ft_strncmp(buf, "gitdir: ", 8) != 0)
		return (false);
	buf[n] = '\0';
	buf[ft_strcspn(buf, "\r\n")] = '\0';
	out[0] = '\0';
	if (buf[8] != '/')
		(ft_strlcpy(out, root, cap), ft_strlcat(out, "/", cap));
	ft_strlcat(out, buf + 8, cap);
	return (true);
}

/* Branch for the repo whose root is `dir`: <dir>/.git/HEAD -- one open,
   the case every prompt in an ordinary repository takes -- or, when .git
   is a gitdir file, the HEAD of the directory it names. */
char	*branch_for_dir(const char *dir)
{
	char	path[PATH_MAX];
	char	*br;

	ft_strlcpy(path, dir, sizeof(path));
	ft_strlcat(path, "/.git/HEAD", sizeof(path));
	br = read_head_file(path);
	if (br != NULL || !git_dir_for(dir, path, sizeof(path)))
		return (br);
	ft_strlcat(path, "/HEAD", sizeof(path));
	return (read_head_file(path));
}

/* Branch + dirty state for the prompt. The root comes from repo_locate's
   cwd-keyed cache (one stat on the hot path, not a walk), the branch is a
   single read of that root's HEAD, and the dirty flag is git_dirty_cached's
   TTL-throttled `git status` — so a prompt outside a repo costs one stat,
   and inside a repo one stat + one small read. */
void	get_git_info(char **branch, int *dirty)
{
	t_gitloc	*loc;

	*branch = NULL;
	*dirty = 0;
	loc = repo_locate();
	if (!loc || !loc->has)
		return ;
	*branch = branch_for_dir(loc->root);
	if (*branch)
		*dirty = git_dirty_cached(loc->root);
}
