/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_compgen4.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/30 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/08/30 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include <sys/stat.h>

/* Small shared pieces: the directory test, the "paste back on the command
   line" join, and the option scanner both compgen and complete use. */

int	cg_is_dir(const char *path)
{
	struct stat	st;

	if (stat(path, &st) != 0)
		return (0);
	return (S_ISDIR(st.st_mode) != 0);
}

/* A completion must be a REPLACEMENT for the word the user typed, so the
   directory part of the prefix is kept and only the last component is the
   entry's name. Printing the bare name instead would look right for
   `compgen -f x` and silently break `compgen -f /etc/x`. */
char	*cg_join_prefix(const char *pfx, const char *name)
{
	const char	*slash;

	slash = ft_strrchr((char *)pfx, '/');
	if (!slash)
		return (ft_strdup((char *)name));
	return (ft_asprintf("%.*s%s", (int)(slash - pfx) + 1, pfx, name));
}

/* One option word. Returns the number of extra argv slots consumed (the
   value-takers -W and -A take one), or -1 on an unknown option. */
static int	cg_one_opt(t_shell *st, t_vec argv, size_t i, t_cgopt *o)
{
	char	*w;

	w = ((char **)argv.ctx)[i];
	if (ft_strcmp(w, "-W") == 0 && i + 1 < argv.len)
		return (o->words = ((char **)argv.ctx)[i + 1], 1);
	if (ft_strcmp(w, "-A") == 0 && i + 1 < argv.len)
	{
		o->act = cg_action_of(((char **)argv.ctx)[i + 1]);
		if (!o->act)
			return (ft_eprintf("%s: compgen: %s: invalid action name\n",
					st->ctx, ((char **)argv.ctx)[i + 1]), -1);
		return (1);
	}
	if (w[1] && !w[2] && ft_strchr("abcdfkv", w[1]))
		return (o->act = w[1], 0);
	return (ft_eprintf("%s: compgen: %s: invalid option\n", st->ctx, w), -1);
}

/* Scan leading options; returns the index of the word to complete, or
   (size_t)-1 after an error the caller reports as status 2. */
size_t	cg_parse_opts(t_shell *st, t_vec argv, t_cgopt *o)
{
	size_t	i;
	int		n;

	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		n = cg_one_opt(st, argv, i, o);
		if (n < 0)
			return (CG_OPT_ERR);
		i += (size_t)n + 1;
	}
	return (i);
}
