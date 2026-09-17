/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   complete_dollar.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 05:10:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 05:10:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* Completing a path that goes THROUGH a variable: `ls $HOME/.con<TAB>`.
**
** The dispatcher sends a word holding a '/' to readline's own filename
** generator, which opened a directory literally called "$HOME" and found
** nothing: the word simply would not complete, which is how it was
** reported ("the auto complete is completely broken").
**
** readline has the hook for it. rl_directory_rewrite_hook rewrites the
** name handed to opendir(2) and NOT what is displayed or inserted, so the
** matches are read from /home/you/ while the line keeps the `$HOME/` the
** user typed -- which is what bash shows too. rl_filename_stat_hook is
** the same expansion for the stat() readline does to decide whether to
** append '/', without which a completed directory got no slash and the
** path stopped there.
**
** Only a LEADING $NAME or ${NAME} is expanded, and only when the name is
** in the environment. A '$' in the middle of a real filename stays a '$'
** (there is a test for `dollar$var.txt`), and an unset name leaves the
** word alone rather than completing against the wrong directory.
**
** environ, not the shell's variable table, for the same reason
** complete_variables.c scans it: the completion callback has no t_shell.
** A non-exported variable does not expand here -- the known limitation
** that file already documents. Memory is libc's on both sides: readline
** malloc'd the string and will free what we put back. */

#include "completion_private.h"
#include "libft.h"
#include <readline/readline.h>
#include <stdlib.h>
#include <string.h>

/* Length of the variable name at s, which points just past '$' or '${'.
   Shared with the quoter, which must leave that span unescaped. */
size_t	comp_dollar_len(const char *s)
{
	size_t	n;

	n = 0;
	while (s[n] && (ft_isalnum((int)(unsigned char)s[n]) || s[n] == '_'))
		n++;
	return (n);
}

/* The value of the leading $NAME / ${NAME}, with *used set to the bytes it
   occupies in `s`. NULL when there is no name, no closing brace, or the
   name is not exported. */
static char	*dollar_value(const char *s, size_t *used)
{
	int		brace;
	size_t	n;
	char	name[256];

	brace = (s[1] == '{');
	n = comp_dollar_len(s + 1 + brace);
	if (n == 0 || n >= sizeof(name))
		return (NULL);
	if (brace && s[1 + brace + n] != '}')
		return (NULL);
	ft_memcpy(name, s + 1 + brace, n);
	name[n] = '\0';
	*used = 1 + brace + n + (size_t)brace;
	return (getenv(name));
}

/* value + the rest of the word, in one libc allocation. */
static char	*dollar_join(const char *val, const char *rest)
{
	size_t	vl;
	size_t	rl;
	char	*out;

	vl = ft_strlen(val);
	rl = ft_strlen(rest);
	out = malloc(vl + rl + 1);
	if (!out)
		return (NULL);
	ft_memcpy(out, val, vl);
	ft_memcpy(out + vl, rest, rl + 1);
	return (out);
}

/* The hook itself: 1 when it replaced the string, 0 when it left it. */
int	comp_dollar_rewrite(char **name)
{
	char	*val;
	char	*out;
	size_t	used;

	if (!name || !*name || (*name)[0] != '$')
		return (0);
	used = 0;
	val = dollar_value(*name, &used);
	if (!val)
		return (0);
	out = dollar_join(val, *name + used);
	if (!out)
		return (0);
	free(*name);
	*name = out;
	return (1);
}

/* Install both halves: the one opendir() reads through, and the one the
   trailing-slash decision stats through. */
void	setup_dollar_hooks(void)
{
	rl_directory_rewrite_hook = comp_dollar_rewrite;
	rl_filename_stat_hook = comp_dollar_rewrite;
}
