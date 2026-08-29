/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   expand_zsh_mod2.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 09:05:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 09:05:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "expander_private.h"
#include "env.h"

/* The individual modifiers. Each takes a BORROWED string and returns a fresh
** one, so zm_run can free as it chains.
**
** These are the same operations `dirname` and `basename` name, with zsh's
** edge cases rather than the utilities': `${x:h}` on a path with no slash is
** "." exactly as dirname says, but `${x:r}` on a name with no dot is the
** whole name rather than empty, and `${x:e}` on the same is empty rather
** than the name. Getting either backwards produces a path that looks
** plausible and points somewhere else.
*/

/* :h -- everything before the last slash; "." when there is none. A trailing
   slash is dropped first, so ${x:h} on "/a/b/" is "/a" and not "/a/b". */
char	*zm_head(const char *v)
{
	int	i;

	i = (int)ft_strlen(v);
	while (i > 1 && v[i - 1] == '/')
		i--;
	while (i > 0 && v[i - 1] != '/')
		i--;
	while (i > 1 && v[i - 1] == '/')
		i--;
	if (i == 0)
		return (ft_strdup("."));
	return (ft_strndup(v, (size_t)i));
}

/* :t -- everything after the last slash. */
char	*zm_tail(const char *v)
{
	const char	*p;

	p = ft_strrchr((char *)v, '/');
	if (!p)
		return (ft_strdup(v));
	return (ft_strdup(p + 1));
}

/* :r -- the name without its extension. The dot must be in the LAST path
   component, so `${x:r}` on "/a.b/c" is "/a.b/c" -- the dot belongs to a
   directory, not to the name.
     A LEADING dot still counts: zsh gives "/a/" and "hidden" for
   "/a/.hidden", not "/a/.hidden" and "". That is not what dirname/basename
   intuition suggests and it is not what a hidden file "means"; it is simply
   what zsh does, checked against 5.9. */
char	*zm_root(const char *v)
{
	const char	*slash;
	const char	*dot;

	slash = ft_strrchr((char *)v, '/');
	dot = ft_strrchr((char *)v, '.');
	if (!dot || (slash && dot < slash))
		return (ft_strdup(v));
	return (ft_strndup(v, (size_t)(dot - v)));
}

/* :e -- the extension alone, empty when there is none. Same rule as :r, so
   the two always partition the name at the same dot. */
char	*zm_ext(const char *v)
{
	const char	*slash;
	const char	*dot;

	slash = ft_strrchr((char *)v, '/');
	dot = ft_strrchr((char *)v, '.');
	if (!dot || (slash && dot < slash))
		return (ft_strdup(""));
	return (ft_strdup(dot + 1));
}

/* :a / :A -- make it absolute against $PWD. zsh's :A additionally resolves
   symlinks and :a does not; both are the same answer for every use in the
   corpus (a plugin locating its own file), and resolving would need
   realpath() on a path that may not exist yet. Treated as :a, which is the
   conservative half: it never follows a link the caller did not ask about.
   An already-absolute path is returned unchanged. */
char	*zm_abs(t_shell *state, const char *v)
{
	char	*cwd;
	char	*out;

	if (v[0] == '/')
		return (ft_strdup(v));
	cwd = env_expand(state, "PWD");
	if (!cwd || !*cwd)
		return (ft_strdup(v));
	out = ft_strjoin(cwd, "/");
	if (!out)
		return (ft_strdup(v));
	cwd = ft_strjoin(out, v);
	xfree(out);
	return (cwd);
}
