/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   brand.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/19 16:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/11/19 16:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* The product was renamed hellish -> polish in 3.4.0. Every name a user's
   machine already holds -- ~/.hellishrc, ~/.config/hellish, /etc/hellish,
   HELLISH_* knobs in rc files and PS1 -- keeps working: the new name is
   tried first and the legacy one is the fallback, so a fresh install only
   ever sees "polish" and an upgraded one never notices the rename. */

#ifndef BRAND_H
# define BRAND_H

# include "env.h"

# define BRAND_NAME "polish"
# define BRAND_LEGACY "hellish"
# define BRAND_PREFIX "POLISH_"
# define BRAND_LEGACY_PREFIX "HELLISH_"

char		*brand_pick(char *fresh, char *legacy);
const char	*brand_getenv(const char *suffix);
t_env		*brand_env(t_shell *state, const char *suffix);
char		*brand_expand(t_shell *state, const char *suffix);

#endif
