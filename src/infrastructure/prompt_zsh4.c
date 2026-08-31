/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   prompt_zsh4.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/31 04:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/31 04:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"
#include "env.h"

/* The zsh path escapes, with their numeric arguments:
       %~  %d  %/          the cwd, ~-abbreviated or not, n components
       %c  %C  %.          the trailing component (%c ≡ %1~, %C ≡ %1/)
   Measured on the oracle at /usr/local/bin:
       %2~ -> local/bin    trailing components, no leading slash
       %-1d -> /usr        LEADING components, slash kept
   Computed here and injected, because \w cannot count components. */

/* Collapse a leading $HOME to ~, in place. Shared with the %(nc..)
   conditional, which counts ~-relative components. */
void	zsh_path_abbrev(t_shell *state, char *buf)
{
	char	*home;
	size_t	hl;

	home = env_expand(state, "HOME");
	hl = 0;
	if (home && *home)
		hl = ft_strlen(home);
	if (hl && ft_strncmp(buf, home, hl) == 0
		&& (buf[hl] == '\0' || buf[hl] == '/'))
	{
		ft_memmove(buf + 1, buf + hl, ft_strlen(buf + hl) + 1);
		buf[0] = '~';
	}
}

/* The cwd into buf, with $HOME collapsed to ~ when the escape wants it. */
static void	zsh_path_text(t_shell *state, char *buf, size_t n, bool abbrev)
{
	if (!getcwd(buf, n))
		ft_strlcpy(buf, "?", n);
	if (abbrev)
		zsh_path_abbrev(state, buf);
}

/* The last n slash-separated components: walk backwards over n
   slashes; running out means the whole string. "/" alone stays "/". */
static const char	*zsh_path_tail(const char *p, int n)
{
	const char	*e;

	if (p[0] == '/' && p[1] == '\0')
		return (p);
	e = p + ft_strlen(p);
	while (e > p && n > 0)
	{
		e--;
		while (e > p && *e != '/')
			e--;
		n--;
	}
	if (n > 0 || e == p)
		return (p);
	return (e + 1);
}

/* The first n components, leading slash kept: "/usr/local/bin" with
   n=1 is "/usr". Truncates buf in place. */
static void	zsh_path_head(char *buf, int n)
{
	int	i;

	i = 0;
	if (buf[i] == '/')
		i++;
	while (buf[i] && n > 0)
	{
		while (buf[i] && buf[i] != '/')
			i++;
		n--;
		if (buf[i] && n > 0)
			i++;
	}
	buf[i] = '\0';
}

bool	zsh_cwd(t_shell *state, t_string *out, t_zesc *z, char c)
{
	char	buf[PATH_MAX + 2];
	int		n;

	if (!ft_strchr("~d/cC.", c) || c == '\0')
		return (false);
	n = 0;
	if (c == 'c' || c == 'C' || c == '.')
		n = 1;
	if (z->has_n)
		n = z->n;
	zsh_path_text(state, buf, sizeof(buf),
		(c == '~' || c == 'c' || c == '.'));
	if (n > 0)
		zsh_inject(out, zsh_path_tail(buf, n));
	else if (n < 0)
	{
		zsh_path_head(buf, -n);
		zsh_inject(out, buf);
	}
	else
		zsh_inject(out, buf);
	return (true);
}
