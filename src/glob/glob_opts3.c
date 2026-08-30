/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_opts3.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"
#include "ft_glob.h"

/* `shopt -s extglob`, mirrored the same way. Both the lexer (so `@(a|b)` is
   one word rather than a syntax error) and the matcher consult it, and they
   must agree: a group that lexes as a word but matches literally would be a
   pattern that silently never fires. */

int	*glob_extglob_cell(void)
{
	static int	on;

	return (&on);
}

int	glob_extglob(void)
{
	return (*glob_extglob_cell());
}
