/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_opts.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/21 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/07/21 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/* shopt nullglob/dotglob state, mirrored here from state->shopt by the
   shopt builtin so expand_word_glob (which has no t_shell) can consult
   them. Same function-local-static pattern as the readline/anim cells.
   A forked child inherits the current values, matching bash. */

int	*glob_nullglob_cell(void)
{
	static int	on;

	return (&on);
}

int	glob_nullglob(void)
{
	return (*glob_nullglob_cell());
}

int	*glob_dotglob_cell(void)
{
	static int	on;

	return (&on);
}

int	glob_dotglob(void)
{
	return (*glob_dotglob_cell());
}
