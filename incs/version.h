/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   version.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef VERSION_H
# define VERSION_H

/* The single source of truth for the running shell's version. Bumped in step
   with the git tag / GitHub release / npm package / docker image. */
# define HELLISH_VERSION "2.1.0"

/* Where releases live; used by the `update` builtin and the daily check. */
# define HELLISH_REPO "Univers42/42sh"
# define HELLISH_PKG "hellish-shell"

#endif
