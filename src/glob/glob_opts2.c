/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_opts2.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include "ft_glob.h"

/* The zsh dialect, mirrored here from state->setopt by zsh_mode_swap so the
   glob layer -- which has no t_shell, exactly like the nullglob/dotglob
   cells in glob_opts.c -- can gate the qualifier syntax on it.
     Mirrored in ONE place, the single function that changes the mode, so it
   cannot drift: there is no path that arms the dialect without going
   through it. A forked child inherits the current value, matching how the
   other cells behave. */

int	*glob_zsh_cell(void)
{
	static int	on;

	return (&on);
}

int	glob_zsh(void)
{
	return (*glob_zsh_cell());
}

/* `shopt -s globstar`, mirrored the same way. The token that carries it is
   emitted only when the option is on, so a `**` written with globstar off
   stays two collapsed asterisks and every existing pattern keeps its POSIX
   meaning -- the option is the whole difference between the two readings. */

int	*glob_globstar_cell(void)
{
	static int	on;

	return (&on);
}

int	glob_globstar(void)
{
	return (*glob_globstar_cell());
}
