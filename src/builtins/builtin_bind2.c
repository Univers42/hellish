/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_bind2.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "zle.h"

/* bash's wording, and its status 2 for a malformed call. */
int	bind_usage(t_shell *state, char bad, char missing)
{
	if (missing)
		ft_eprintf("%s: bind: -%c: option requires an argument\n",
			state->ctx, missing);
	else
		ft_eprintf("%s: bind: -%c: invalid option\n", state->ctx, bad);
	ft_eprintf("bind: usage: bind [-lpsvPSVX] [-m keymap] [-f filename] "
		"[-q name] [-u name] [-r keyseq] [-x keyseq:shell-command] "
		"[keyseq:readline-function or readline-command]\n");
	return (2);
}

/* Every listing asked for, in bash's order whatever the order given.
   -X lists `bind -x` bindings, of which there are none. */
int	bind_list_all(t_shell *state, const char *map, const char *list)
{
	const char	*order;

	order = "lpPsSvV";
	while (*order)
	{
		if (ft_strchr(list, *order))
			bind_rl_list(state, map, *order);
		order++;
	}
	return (0);
}

/* -f FILE: bash reads it on the spot, and so reports a missing file on
   the spot; the reading itself waits with everything else, as readline's
   own `$include`. A relative name is made absolute now, while it still
   means what the caller meant. */
int	bind_file(t_shell *state, const char *map, const char *file)
{
	char	*line;

	if (!file)
		return (0);
	if (access(file, R_OK) != 0)
		return (ft_eprintf("%s: bind: %s: cannot read: %s\n",
				state->ctx, file, strerror(errno)), 1);
	if (file[0] == '/' || !state->cwd.ctx)
		line = ft_strjoin("$include ", file);
	else
		line = ft_asprintf("$include %s/%s", (char *)state->cwd.ctx, file);
	if (!line)
		return (1);
	bind_line_add(0, map, line);
	xfree(line);
	return (0);
}

/* -u NAME: unbind every key running it. */
int	bind_unbind(t_shell *state, const char *map, const char *name)
{
	if (!name)
		return (0);
	if (!bind_rl_fn_ok(name))
		return (ft_eprintf("%s: bind: `%s': unknown function name\n",
				state->ctx, name), 1);
	bind_line_add('u', map, name);
	return (0);
}
