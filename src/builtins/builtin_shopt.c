/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_shopt.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "builtins_private.h"
#include "ft_glob.h"

/* shopt [-s|-u|-q] [name...]: bash's toggle for behaviour options. The
   named-option state lives in state->shopt; nullglob and dotglob are wired
   into the glob expander (glob_nullglob / glob_dotglob accessors, refreshed
   from here), the rest are stored+reported so scripts that toggle them run.
   Unknown names error like bash. -q is quiet (status reflects the setting),
   plain `shopt name` prints "name<TAB>on|off". */

/* Name → bit table. Order is display order (bash sorts, but a stable list
   is close enough and keeps the common ones first). */
static unsigned int	shopt_bit(const char *name)
{
	static const struct s_sh	tab[] = {
	{"nullglob", SHOPT_NULLGLOB}, {"dotglob", SHOPT_DOTGLOB},
	{"globstar", SHOPT_GLOBSTAR}, {"nocaseglob", SHOPT_NOCASEGLOB},
	{"extglob", SHOPT_EXTGLOB}, {"lastpipe", SHOPT_LASTPIPE},
	{"histappend", SHOPT_HISTAPPEND}, {"checkwinsize", SHOPT_CHECKWINSIZE},
	{"autocd", SHOPT_AUTOCD}, {"cdspell", SHOPT_CDSPELL},
	{"lithist", SHOPT_LITHIST}, {"progcomp", SHOPT_PROGCOMP},
	{NULL, 0}};
	int							i;

	i = 0;
	while (tab[i].name)
	{
		if (ft_strcmp(tab[i].name, name) == 0)
			return (tab[i].bit);
		i++;
	}
	return (0);
}

/* Push the glob-affecting options down to the expander accessors. */
static void	shopt_sync(t_shell *state)
{
	*glob_nullglob_cell() = (state->shopt & SHOPT_NULLGLOB) != 0;
	*glob_dotglob_cell() = (state->shopt & SHOPT_DOTGLOB) != 0;
	*glob_globstar_cell() = (state->shopt & SHOPT_GLOBSTAR) != 0;
	*glob_extglob_cell() = (state->shopt & SHOPT_EXTGLOB) != 0;
}

/* Apply/query one name under act ('s' set, 'u' unset, 'p' print in
   reusable form, 0 print as "name<TAB>on|off"). `quiet` is the -q
   modifier and suppresses output; it is ORTHOGONAL to act, which is why
   the two are separate parameters -- see the flag loop for why that
   matters. Returns the per-name status.

   The two status rules are bash's and they are NOT the same rule: -s and
   -u report whether the CHANGE succeeded, so a successful `shopt -u x`
   is 0 even though x ends up off; -q and the print forms report the
   SETTING, so `shopt x` on an unset option is 1. hellish had both
   backwards -- `shopt -u extglob` returned 1, and `shopt extglob`
   returned 0 whatever the setting -- which made either one useless in an
   `if`. */
static int	shopt_one(t_shell *state, const char *name, char act, int quiet)
{
	unsigned int	bit;

	bit = shopt_bit(name);
	if (bit == 0)
		return (ft_eprintf("%s: shopt: %s: invalid shell option name\n",
				state->ctx, name), 1);
	if (act == 's' || act == 'u')
	{
		if (act == 's')
			state->shopt |= bit;
		else
			state->shopt &= ~bit;
		return (0);
	}
	if (!quiet && act == 'p' && (state->shopt & bit))
		ft_printf("shopt -s %s\n", name);
	else if (!quiet && act == 'p')
		ft_printf("shopt -u %s\n", name);
	else if (!quiet && (state->shopt & bit))
		ft_printf("%-20s\ton\n", name);
	else if (!quiet)
		ft_printf("%-20s\toff\n", name);
	if (state->shopt & bit)
		return (0);
	return (1);
}

/* Print every known option. Each line goes through shopt_one so the two
   output formats (plain and the -p reusable form) live in one place. */
static int	shopt_print_all(t_shell *state, char act, int quiet)
{
	static const char *const	names[] = {"autocd", "cdspell",
		"checkwinsize", "dotglob", "extglob", "globstar", "histappend",
		"lastpipe", "lithist", "nocaseglob", "nullglob", "progcomp", NULL};
	int							i;

	i = 0;
	while (names[i])
		shopt_one(state, names[i++], act, quiet);
	return (0);
}

/* `shopt -o` addresses the `set -o` options, not shopt's own -- bash
   documents it as "restrict to option names defined for set -o", which is
   why bare `shopt -o` and bare `set -o` print the same table. hellish
   listed its shopt names there instead, so a script asking about
   `allexport` got an answer about `autocd`.

   -o is a SCOPE, not an action, and that distinction is the bug behind
   issue #51. It used to be parsed as one of the actions, so it lost every
   argument that came with it: `shopt -oq posix` -- which is how Ubuntu's
   stock ~/.bashrc asks whether posix mode is on -- dumped all 27 option
   lines to the screen at login and returned 0 no matter the setting, and
   `shopt -op posix` was read as a bare -p that then rejected "posix" as an
   unknown shopt name. It now rides alongside act and quiet, and
   shopt_setopt() applies the same action rules to the set -o roster. */
int	builtin_shopt(t_shell *state, t_vec argv)
{
	char	act;
	int		quiet;
	int		use_o;
	size_t	i;
	int		rc;

	act = 0;
	quiet = 0;
	use_o = 0;
	i = shopt_flags(argv, &act, &quiet, &use_o);
	if (use_o && i >= argv.len)
		return (list_set_options(state));
	if (use_o)
		return (shopt_setopt(state, argv, i, (t_shopt_act){act, quiet}));
	if (i >= argv.len)
		return (shopt_print_all(state, act, quiet));
	rc = 0;
	while (i < argv.len)
		if (shopt_one(state, ((char **)argv.ctx)[i++], act, quiet))
			rc = 1;
	return (shopt_sync(state), rc);
}
