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
# define HELLISH_VERSION "2.4.1"

/* Where releases live; used by the `update` builtin and the daily check. */
# define HELLISH_REPO "Univers42/hellish"
# define HELLISH_PKG "hellish-shell"

/* The release asset this build can install over itself. Must match what
   .github/workflows/release.yml publishes, and it encodes the ABI: the
   updater refuses anything else rather than installing a binary for the
   wrong architecture. */
# define HELLISH_ASSET "hellish-linux-x86_64"

/* Bumped whenever the welcome header itself changes what it says. The
   header is shown once a day, but a new revision is new information, so
   it is shown again immediately rather than waiting for tomorrow. */
# define HELLISH_HEADER_REV 2

#endif
