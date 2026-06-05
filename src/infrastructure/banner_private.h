/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   banner_private.h                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/05 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/05 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BANNER_PRIVATE_H
# define BANNER_PRIVATE_H

# include "header.h"
# include <stddef.h>

/* The little glyphs the status footer uses, picked once for the terminal:
   a tick, a warning triangle, a middle dot and an em dash (ASCII fallbacks). */
typedef struct s_sym
{
	const char	*ok;
	const char	*warn;
	const char	*sep;
	const char	*dash;
}	t_sym;

/* Fill the right column: getting-started tips, the "What's new" highlights for
   the running release, and a live version/update status footer. Backing
   strings live in static storage (the banner is shown once). */
void	build_right(const char **r);

/* Compose the one-line status footer into `out`: an HP_OK "up to date" line
   (with origin + last-checked age) or, when the cached check found a newer
   release, a vivid HP_WARN "vX available — run update --now" line. */
void	status_line(char *out, size_t n);

#endif
