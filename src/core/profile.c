/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   profile.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "core.h"
#include <fcntl.h>
#include <unistd.h>

int			exec_string(t_shell *state, char *str);
char		*env_expand(t_shell *state, char *key);

/* Read a whole file into one freshly-allocated, NUL-terminated string. We grow
   a t_string in 4 KB gulps instead of stat()-ing the size up front -- simpler,
   and it also works on pipes/things that have no real size. That trailing '\0'
   is what lets the rest of the shell treat the result like any C string. NULL
   if the file will not even open. */
char	*read_file(const char *path)
{
	char		buf[4096];
	t_string	content;
	int			fd;
	ssize_t		n;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return (NULL);
	vec_init(&content);
	content.elem_size = 1;
	n = read(fd, buf, sizeof(buf));
	while (n > 0)
	{
		vec_push_nstr(&content, buf, n);
		n = read(fd, buf, sizeof(buf));
	}
	close(fd);
	vec_push(&content, &(char){0});
	return ((char *)content.ctx);
}

/* Run one startup file in *this* shell if it is readable. A missing or
   unreadable file is not an error -- plenty of systems ship no ~/.profile at
   all, and a login must never fail over that. */
static void	source_file(t_shell *state, const char *path)
{
	char	*content;

	content = read_file(path);
	if (!content)
		return ;
	exec_string(state, content);
	xfree(content);
}

/* Login shells only (argv[0] began with '-', or --login was passed). This is
   the hook every distro assumes a login shell provides: /etc/profile is where
   PATH actually gets built -- on Debian/Ubuntu it is also the thing that loops
   over the .sh snippets in /etc/profile.d, so we must NOT walk that directory
   ourselves or every snippet would run twice. Skipping this is why a chsh'd
   hellish used to lose /snap/bin and anything else the system adds at login.
   Order matters: system file, then the user's, then ~/.hellishrc last -- so
   your own config always gets the final word on PS1 and friends. */
void	source_profile(t_shell *state)
{
	char	*home;
	char	*path;

	if (!(state->option_flags & OPT_FLAG_LOGIN))
		return ;
	source_file(state, "/etc/profile");
	home = env_expand(state, "HOME");
	if (!home || !*home)
		return ;
	path = ft_strjoin(home, "/.profile");
	if (!path)
		return ;
	source_file(state, path);
	xfree(path);
}
