/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_state.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/* Path of `name` inside the hellish cache dir, honouring XDG_CACHE_HOME and
   falling back to $HOME/.cache. Remembering which version is available must
   never need root, so this is deliberately a user-level location. */
int	update_cache_file(const char *name, char *buf, size_t n)
{
	char	*base;

	base = getenv("XDG_CACHE_HOME");
	if (base && *base)
		ft_strlcpy(buf, base, n);
	else
	{
		base = getenv("HOME");
		if (!base || !*base)
			return (0);
		ft_strlcpy(buf, base, n);
		ft_strlcat(buf, "/.cache", n);
	}
	ft_strlcat(buf, "/hellish/", n);
	ft_strlcat(buf, name, n);
	return (ft_strlen(buf) + 1 < n);
}

/* Create the cache directory chain. mkdir failing because it already exists
   is the normal case, so only the closing access() decides. */
int	update_cache_mkdir(void)
{
	char	path[512];
	char	*slash;

	if (!update_cache_file("x", path, sizeof(path)))
		return (0);
	slash = ft_strrchr(path, '/');
	if (!slash)
		return (0);
	*slash = '\0';
	slash = ft_strrchr(path, '/');
	if (slash)
	{
		*slash = '\0';
		mkdir(path, 0755);
		*slash = '/';
	}
	mkdir(path, 0755);
	return (access(path, W_OK) == 0);
}

/* Apply one "key=value" line. Unknown keys are ignored on purpose: a state
   file written by a NEWER hellish must not break an older one. */
static void	state_set(t_upd_state *s, char *key, char *val)
{
	if (!ft_strcmp(key, "latest"))
		ft_strlcpy(s->latest, val, sizeof(s->latest));
	else if (!ft_strcmp(key, "checked"))
		s->checked = ft_atoi(val);
	else if (!ft_strcmp(key, "notified"))
		s->notified = ft_atoi(val);
	else if (!ft_strcmp(key, "header_shown"))
		s->header_shown = ft_atoi(val);
	else if (!ft_strcmp(key, "header_rev"))
		s->header_rev = ft_atoi(val);
	else if (!ft_strcmp(key, "header_ver"))
		ft_strlcpy(s->header_ver, val, sizeof(s->header_ver));
}

/* Split the file into lines and feed each key=value pair to state_set. */
static void	state_parse(t_upd_state *s, char *buf)
{
	char	*line;
	char	*eq;
	char	*nl;

	line = buf;
	while (line && *line)
	{
		nl = ft_strchr(line, '\n');
		if (nl)
			*nl = '\0';
		eq = ft_strchr(line, '=');
		if (eq)
		{
			*eq = '\0';
			state_set(s, line, eq + 1);
		}
		if (!nl)
			return ;
		line = nl + 1;
	}
}

/* Load the persisted state. A missing, unreadable or truncated file is not
   an error: the caller gets a zeroed struct, which reads as "nothing known"
   and simply makes the next check run. */
int	update_state_load(t_upd_state *s)
{
	char	path[512];
	char	buf[1024];
	int		fd;
	ssize_t	r;

	ft_memset(s, 0, sizeof(*s));
	if (!update_cache_file("state", path, sizeof(path)))
		return (0);
	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (0);
	r = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (r <= 0)
		return (0);
	buf[r] = '\0';
	state_parse(s, buf);
	return (s->latest[0] != '\0');
}
