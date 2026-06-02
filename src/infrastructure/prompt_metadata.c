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

/* Turn a .git/HEAD payload into a branch name (or short SHA when detached). */
static char	*parse_head(const char *buf)
{
	int	len;

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

/* Branch for the repo whose root is `dir`: <dir>/.git/HEAD, or, when .git is a
   gitdir file (submodules/worktrees), the referenced gitdir's HEAD. */
static char	*branch_for_dir(const char *dir)
{
	char	path[PATH_MAX];
	char	buf[PATH_MAX];
	char	*br;
	int		fd;
	ssize_t	n;

	ft_strlcpy(path, dir, sizeof(path));
	ft_strlcat(path, "/.git/HEAD", sizeof(path));
	br = read_head_file(path);
	if (br != NULL)
		return (br);
	ft_strlcpy(path, dir, sizeof(path));
	ft_strlcat(path, "/.git", sizeof(path));
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (NULL);
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n <= 0 || ft_strncmp(buf, "gitdir: ", 8) != 0)
		return (NULL);
	buf[ft_strcspn(buf, "\r\n")] = '\0';
	ft_strlcpy(path, buf + 8, sizeof(path));
	ft_strlcat(path, "/HEAD", sizeof(path));
	return (read_head_file(path));
}

/* Fork-free git branch: walk up from $PWD reading .git/HEAD directly. No
   subprocess (the old version forked sh->timeout->git twice per prompt). */
void	get_git_info(char **branch, int *dirty)
{
	char	cwd[PATH_MAX];
	char	*slash;

	*branch = NULL;
	*dirty = 0;
	if (!getcwd(cwd, sizeof(cwd)))
		return ;
	while (1)
	{
		if (cwd[0])
			*branch = branch_for_dir(cwd);
		else
			*branch = branch_for_dir("/");
		if (*branch)
			return ;
		slash = ft_strrchr(cwd, '/');
		if (!slash || slash == cwd)
			return ;
		*slash = '\0';
	}
}
