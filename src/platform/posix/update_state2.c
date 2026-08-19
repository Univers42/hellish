/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update_state2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/19 20:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/08/19 20:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "update.h"
#include "version.h"
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

/* Persist the state through a temp file and rename(2).

   The timestamps go out through %d rather than %ld because ft_snprintf has
   no length modifiers -- it copies "%ld" through literally, which silently
   wrote a state file full of the format string itself and made the header
   gate think it had never run. Casting to int caps these at 2038; they are
   cache timestamps, so the worst a rollover can do is re-show a banner.
   The real fix is %ld support in libft, noted in backlog.md.
   and a foreground `update` can run at the same time, and a half-written
   record read by the prompt would announce a version that does not exist;
   rename is atomic, so a reader sees either the old record or the new one. */
int	update_state_save(const t_upd_state *s)
{
	char	path[512];
	char	tmp[544];
	char	buf[512];
	int		fd;
	int		len;

	if (!update_cache_mkdir()
		|| !update_cache_file("state", path, sizeof(path)))
		return (0);
	ft_strlcpy(tmp, path, sizeof(tmp));
	ft_strlcat(tmp, ".new", sizeof(tmp));
	fd = open(tmp, O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if (fd < 0)
		return (0);
	len = ft_snprintf(buf, sizeof(buf), "latest=%s\nchecked=%d\n"
			"notified=%d\nheader_shown=%d\nheader_rev=%d\n"
			"header_ver=%s\n", s->latest, (int)s->checked,
			(int)s->notified, (int)s->header_shown, (int)s->header_rev,
			s->header_ver);
	if (len <= 0 || write(fd, buf, (size_t)len) != len)
		return (close(fd), unlink(tmp), 0);
	close(fd);
	if (rename(tmp, path) != 0)
		return (unlink(tmp), 0);
	return (1);
}

/* True when the known latest release is newer than this build. */
int	update_available(const t_upd_state *s)
{
	if (!s->latest[0])
		return (0);
	return (hellish_version_cmp(s->latest, HELLISH_VERSION) > 0);
}
