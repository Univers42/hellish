/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rc_shebang.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 17:02:53 by dlesieur          #+#    #+#             */
/*   Updated: 2026/03/16 01:14:50 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "brand.h"
#include "libft.h"
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>

bool	rc_shebang_is_ours(const char *path);

/* True when the word at `s` has basename `name`: the bytes after its last
   '/' up to the next blank. Compared in place, nothing allocated. A '\r' is
   a blank so a file saved with CRLF endings still matches. */
static bool	word_is(const char *s, const char *name)
{
	size_t	len;
	size_t	base;

	len = 0;
	base = 0;
	while (s[len] && s[len] != ' ' && s[len] != '\t' && s[len] != '\r')
	{
		if (s[len++] == '/')
			base = len;
	}
	return (len - base == ft_strlen(name)
		&& ft_strncmp(s + base, name, len - base) == 0);
}

static const char	*next_word(const char *s)
{
	while (*s && *s != ' ' && *s != '\t')
		s++;
	while (*s == ' ' || *s == '\t')
		s++;
	return (s);
}

/* `#!/usr/bin/polish`, `#!/usr/bin/env polish`, `#!/usr/bin/env -S polish`
   and the same with hellish: the interpreter, or the program env runs,
   names this shell under either brand. */
static bool	interp_is_ours(const char *s)
{
	while (*s == ' ' || *s == '\t')
		s++;
	if (word_is(s, "env"))
	{
		s = next_word(s);
		while (*s == '-')
			s = next_word(s);
	}
	return (word_is(s, BRAND_NAME) || word_is(s, BRAND_LEGACY));
}

/* Whether a file's first line is a shebang naming polish or hellish. This
   is what lets an extensionless config module load: the shebang, not the
   name, says which program reads it. O_NONBLOCK so a FIFO dropped into a
   config directory cannot hang shell startup; a directory fails the read. */
bool	rc_shebang_is_ours(const char *path)
{
	char	buf[256];
	ssize_t	n;
	int		fd;
	char	*nl;

	fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0)
		return (false);
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n < 2 || buf[0] != '#' || buf[1] != '!')
		return (false);
	buf[n] = '\0';
	nl = ft_strchr(buf, '\n');
	if (nl)
		*nl = '\0';
	return (interp_is_ours(buf + 2));
}
