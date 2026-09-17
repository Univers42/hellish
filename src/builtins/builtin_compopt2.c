/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_compopt2.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/17 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "libft.h"

t_compspec	*comp_find(t_shell *st, const char *name);

/* Argument scanning, printing and the two targets compopt can act on. */

/* One of bash's nine option names? */
bool	co_valid(const char *name)
{
	int	i;

	i = -1;
	while (++i < 9)
		if (!ft_strcmp(co_name_at(i), name))
			return (true);
	return (false);
}

/* Validate the flags and leave *FIRST on the first NAME. Status 2 is
   bash's for a usage error, and it distinguishes the two: an unknown
   option name is named, an unknown flag gets the usage line. */
int	co_scan(t_shell *st, t_vec argv, size_t *first)
{
	char	*w;

	*first = 1;
	while (*first < argv.len && ft_strchr("-+", ((char **)argv.ctx)[*first][0])
		&& ((char **)argv.ctx)[*first][1])
	{
		w = ((char **)argv.ctx)[(*first)++];
		if (!ft_strcmp(w, "--"))
			return (0);
		if (!ft_strcmp(w, "-o") || !ft_strcmp(w, "+o"))
		{
			if (*first >= argv.len)
				return (co_usage(st, w, "option requires an argument"));
			if (!co_valid(((char **)argv.ctx)[*first]))
				return (ft_eprintf("%s: compopt: %s: invalid option name\n",
						st->ctx, ((char **)argv.ctx)[(*first)]), 2);
			(*first)++;
		}
		else if (!(w[1] && !w[2] && ft_strchr("DEI", w[1])))
			return (co_usage(st, w, "invalid option"));
	}
	return (0);
}

/* `compopt NAME` with no -o/+o: bash echoes the whole option state back as
   the compopt command that would reproduce it. */
void	co_print(t_compspec *c)
{
	int	i;

	ft_printf("compopt");
	i = -1;
	while (++i < 9)
	{
		if (pc_opt_has(c->opts, co_name_at(i)))
			ft_printf(" -o %s", co_name_at(i));
		else
			ft_printf(" +o %s", co_name_at(i));
	}
	ft_printf(" %s\n", c->name);
}

/* No NAME: the spec of the completion function running right now. The edit
   lands on the live copy (pc_live) and is applied at once, so THIS TAB sees
   it; it is thrown away when the call returns, which is what bash does --
   `complete -p` after a `compopt -o filenames` still prints the spec as it
   was registered. */
int	co_current(t_shell *st, t_vec argv)
{
	char	**live;

	live = pc_live();
	if (!*live)
		return (ft_eprintf("%s: compopt: not currently executing completion "
				"function\n", st->ctx), 1);
	co_edit(argv, live);
	pc_opt_apply(*live);
	return (0);
}

/* Named specs: the edit is permanent, and an unknown name is an error that
   does not stop the names after it. */
int	co_named(t_shell *st, t_vec argv, size_t i)
{
	t_compspec	*c;
	int			rc;

	rc = 0;
	while (i < argv.len)
	{
		c = comp_find(st, ((char **)argv.ctx)[i]);
		if (!c)
			rc = (ft_eprintf("%s: compopt: %s: no completion specification\n",
						st->ctx, ((char **)argv.ctx)[i]), 1);
		else if (co_has_edit(argv))
			co_edit(argv, &c->opts);
		else
			co_print(c);
		i++;
	}
	return (rc);
}
