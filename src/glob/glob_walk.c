/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   glob_walk.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 11:11:45 by dlesieur          #+#    #+#             */
/*   Updated: 2026/01/25 21:06:56 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "glob_private.h"

/* Start the directory walk at "/" for an absolute pattern (first token is
   G_SLASH) or at "" -- meaning the cwd -- for a relative one. */
void	glob_walk(t_vec *args, t_vec_glob glob)
{
	if (((t_glob *)glob.ctx)[0].ty == G_SLASH)
		match_dir(args, glob, "/", 1);
	else
		match_dir(args, glob, "", 0);
}

/* Arm dotglob for THIS walk if the pattern carried a (D) qualifier, and
   hand back the previous value for the caller to restore.
     Whether a dotfile is offered at all is the walk's decision, so a
   post-filter cannot add one back -- and `[*](D)` must not leave dotglob on
   for the rest of the session, hence save-and-restore rather than set. */
int	glob_dots_arm(t_gqual *q)
{
	int	was;

	was = *glob_dotglob_cell();
	if (q->dots)
		*glob_dotglob_cell() = 1;
	return (was);
}
