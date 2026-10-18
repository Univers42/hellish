/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_fc3.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/24 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/09/24 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "history.h"

/* fc's command line and its view of the history.
**
** Options were matched as whole words -- `-l` and `-lr` only -- so every
** other spelling of a listing fell through to EDIT mode and launched
** $EDITOR on the history: `fc -ln -1`, `-nl`, `-lnr`, `-l -n`. The omz sudo
** widget asks `$(fc -ln -1)` for the previous command when ESC ESC hits an
** empty line (#137), which started vim inside a command substitution.
** Options now cluster the POSIX way, `--` ends them, and a `-` followed by
** a digit is the negative-number OPERAND it always is in fc (`fc -l -3`). */

static int	fc_bad_opt(t_shell *state, char c)
{
	ft_eprintf("%s: fc: -%c: invalid option\n", state->ctx, c);
	ft_eprintf("fc: usage: fc [-e ename] [-lnr] [first] [last]"
		" or fc -s [pat=rep] [command]\n");
	return (2);
}

/* One option word, `-lnr` or `-e` with its editor. Returns how many
   argv words it used (1 or 2), or -1 after reporting an error. */
static int	fc_cluster(t_shell *state, char **av, int i, t_fcopt *o)
{
	const char	*p;

	p = av[i];
	while (*++p && *p != 'e')
	{
		if (!ft_strchr("lnrs", *p))
			return (fc_bad_opt(state, *p), -1);
		o->list |= (*p == 'l');
		o->nonum |= (*p == 'n');
		o->rev |= (*p == 'r');
		o->subst |= (*p == 's');
	}
	if (*p != 'e')
		return (1);
	if (p[1])
		return (o->editor = p + 1, 1);
	if (!av[i + 1])
		return (ft_eprintf("%s: fc: -e: option requires an argument\n",
				state->ctx), -1);
	return (o->editor = av[i + 1], 2);
}

/* Fill `o` from argv. Returns 0, or 2 on a usage error (already reported). */
int	fc_parse(t_shell *state, char **av, int ac, t_fcopt *o)
{
	int	i;
	int	used;

	*o = (t_fcopt){0};
	i = 1;
	while (i < ac && av[i][0] == '-' && av[i][1]
		&& !ft_isdigit((unsigned char)av[i][1]))
	{
		if (ft_strcmp(av[i], "--") == 0)
		{
			i++;
			break ;
		}
		used = fc_cluster(state, av, i, o);
		if (used < 0)
			return (2);
		i += used;
	}
	if (o->editor && ft_strcmp(o->editor, "-") == 0)
		o->subst = true;
	o->ops = av + i;
	o->nops = ac - i;
	return (0);
}

/* How many entries fc sees: the whole list, minus this very line when it
   is already in it -- bash records a command before it runs, so `fc -l -1`
   would otherwise name the fc command itself. Inside a zle widget the line
   being edited is not recorded yet, and -1 is the previous command, which
   is what zsh code means by it. */
int	fc_total(t_shell *state)
{
	int	n;

	n = (int)state->hist.hist_cmds.len;
	if (state->hist.recorded && n > 0)
		n--;
	return (n);
}
