/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mascot.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:20 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "prompt_private.h"

/* The mascot (blinking devil face) was removed from the live prompt in favour
   of the static intro animation shown once at startup. This stub keeps the
   signature intact so the rest of the prompt pipeline compiles unchanged.
   The prompt box now starts directly with the user/cwd line. */
int	push_mascot(t_string *ret, size_t frame, int status)
{
	(void)ret;
	(void)frame;
	(void)status;
	return (0);
}
