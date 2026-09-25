/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_bind.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/25 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/25 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "zle.h"

/* bash's `bind`: key bindings and readline settings from a config file.
**
**     bind '"\e[A": history-search-backward'     Up searches by prefix
**     bind 'set completion-ignore-case on'
**     bind -p | grep search                      what is bound now
**
** Before it, `set -o emacs` / `set -o vi` were the only line-editing
** control an rc had: no history-prefix search on Up, no inputrc setting,
** no rebinding C-w (#134 item 7, and the core half of #136). Lines are
** recorded, not applied: readline may not exist yet, and the editing
** mode's keymap replaces whatever was bound before it (bind_lines.c).
** The listings bring readline up first, so they show what a prompt would.
**
** -x (a key that runs a shell command) is a bash extension on top of
** readline, and is not here yet: it says so and fails rather than bind a
** key that would do nothing. */

/* The option letter at av[*i][*j]; one that takes an argument takes the
   rest of the word or the next one, like getopt. NULL when there is none.
   Either way the word it came from is used up: *j is left on its last
   byte (the caller steps past it), which for an empty word wraps to
   SIZE_MAX so the step lands on its NUL. */
static const char	*bind_optarg(t_vec argv, size_t *i, size_t *j)
{
	char	**av;
	char	*arg;

	av = (char **)argv.ctx;
	arg = av[*i] + *j + 1;
	if (!*arg)
	{
		if (*i + 1 >= argv.len)
			return (NULL);
		arg = av[++*i];
	}
	*j = ft_strlen(av[*i]) - 1;
	return (arg);
}

static const char	**bind_slot(t_bindopt *o, char c)
{
	if (c == 'm')
		return (&o->map);
	if (c == 'q')
		return (&o->query);
	if (c == 'u')
		return (&o->unbind);
	if (c == 'r')
		return (&o->remove);
	if (c == 'f')
		return (&o->file);
	if (c == 'x')
		return (&o->unix_cmd);
	return (NULL);
}

static void	bind_letter(t_bindopt *o, t_vec argv, size_t *i, size_t *j)
{
	char		c;
	const char	**slot;
	size_t		n;

	c = ((char **)argv.ctx)[*i][*j];
	slot = bind_slot(o, c);
	if (slot)
	{
		*slot = bind_optarg(argv, i, j);
		if (!*slot)
			o->missing = c;
	}
	else if (ft_strchr("lpPvVsSX", c))
	{
		n = ft_strlen(o->list);
		if (!ft_strchr(o->list, c) && n + 1 < sizeof(o->list))
			o->list[n] = c;
	}
	else
		o->bad = c;
}

/* Options up to the first word that is not one; returns its index. */
static size_t	bind_parse(t_bindopt *o, t_vec argv)
{
	char	**av;
	size_t	i;
	size_t	j;

	av = (char **)argv.ctx;
	i = 1;
	while (i < argv.len && av[i][0] == '-' && av[i][1]
		&& !o->bad && !o->missing)
	{
		if (!ft_strcmp(av[i], "--"))
			return (i + 1);
		j = 1;
		while (av[i][j] && !o->bad && !o->missing)
		{
			bind_letter(o, argv, &i, &j);
			j++;
		}
		i++;
	}
	return (i);
}

/* bash's order: listings, -f, -q, -u, -r, then the bindings. A request
   that readline would carry out later is checked now, as bash checks it:
   the keymap, -u's function, -f's file. */
int	builtin_bind(t_shell *state, t_vec argv)
{
	t_bindopt	o;
	size_t		i;
	int			st;

	o = (t_bindopt){0};
	i = bind_parse(&o, argv);
	if (o.bad || o.missing)
		return (bind_usage(state, o.bad, o.missing));
	if (o.unix_cmd)
		return (ft_eprintf("%s: bind: -x: not supported yet\n",
				state->ctx), 1);
	if (o.map && !bind_rl_keymap_ok(o.map))
		return (ft_eprintf("%s: bind: `%s': invalid keymap name\n",
				state->ctx, o.map), 1);
	st = bind_list_all(state, o.map, o.list);
	st |= bind_file(state, o.map, o.file);
	if (o.query)
		st |= bind_rl_query(state, o.map, o.query);
	st |= bind_unbind(state, o.map, o.unbind);
	if (o.remove)
		bind_line_add('r', o.map, o.remove);
	while (i < argv.len)
		bind_line_add(0, o.map, ((char **)argv.ctx)[i++]);
	return (st);
}
