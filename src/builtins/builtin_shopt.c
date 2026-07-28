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
	{"autocd", SHOPT_AUTOCD}, {"cdspell", SHOPT_CDSPELL}, {NULL, 0}};
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

/* Push the two glob-affecting options down to the expander accessors. */
static void	shopt_sync(t_shell *state)
{
	*glob_nullglob_cell() = (state->shopt & SHOPT_NULLGLOB) != 0;
	*glob_dotglob_cell() = (state->shopt & SHOPT_DOTGLOB) != 0;
}

/* Apply/query one name under mode ('s' set, 'u' unset, 'q' query,
   0 print). Returns the per-name status for -q/print accumulation. */
static int	shopt_one(t_shell *state, const char *name, char mode)
{
	unsigned int	bit;
	const char		*st;

	bit = shopt_bit(name);
	if (bit == 0)
		return (ft_eprintf("%s: shopt: %s: invalid shell option name\n",
				state->ctx, name), 1);
	if (mode == 's')
		state->shopt |= bit;
	else if (mode == 'u')
		state->shopt &= ~bit;
	else if (mode == 0)
	{
		st = "off";
		if (state->shopt & bit)
			st = "on";
		ft_printf("%-20s\t%s\n", name, st);
	}
	if (state->shopt & bit)
		return (0);
	return (1);
}

/* Print every known option as "name<TAB>on|off" (the no-argument form).
   Each line goes through shopt_one's print mode so the output format
   lives in exactly one place. */
static int	shopt_print_all(t_shell *state)
{
	static const char *const	names[] = {"autocd", "cdspell",
		"checkwinsize", "dotglob", "extglob", "globstar", "histappend",
		"lastpipe", "nocaseglob", "nullglob", NULL};
	int							i;

	i = 0;
	while (names[i])
		shopt_one(state, names[i++], 0);
	return (0);
}

int	builtin_shopt(t_shell *state, t_vec argv)
{
	char	mode;
	size_t	i;
	int		rc;

	mode = 0;
	i = 1;
	while (i < argv.len && ((char **)argv.ctx)[i][0] == '-'
		&& ((char **)argv.ctx)[i][1])
	{
		if (((char **)argv.ctx)[i][1] == 's')
			mode = 's';
		else if (((char **)argv.ctx)[i][1] == 'u')
			mode = 'u';
		else if (((char **)argv.ctx)[i][1] == 'q')
			mode = 'q';
		i++;
	}
	if (i >= argv.len)
		return (shopt_print_all(state));
	rc = 0;
	while (i < argv.len)
		if (shopt_one(state, ((char **)argv.ctx)[i++], mode) && mode != 0)
			rc = 1;
	return (shopt_sync(state), rc);
}
