/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UPDATE_H
# define UPDATE_H

# include "shell.h"
# include "sh_input.h"

/* How this running hellish got onto the machine — each updates differently. */
typedef enum e_origin
{
	ORIGIN_BINARY,
	ORIGIN_NPM,
	ORIGIN_PNPM,
	ORIGIN_DOCKER,
	ORIGIN_SOURCE
}	t_origin;

/* Classify the running binary by inspecting /proc/self/exe; for a source
   checkout, `repo` is filled with the repository root. */
t_origin	detect_origin(char *repo, size_t n);

/* A human label for the origin ("npm", "docker", "source checkout", …). */
const char	*origin_label(t_origin o);

/* The shell command that upgrades this origin, written into out. */
void		origin_command(t_origin o, const char *repo, char *out, size_t n);

/* Run the origin's upgrade command (or print guidance for docker). */
int			run_origin_update(t_origin o, const char *repo);

/* Compare two dotted versions ("2.1.0", optionally "v"-prefixed).
   Returns >0 if a is newer than b, 0 if equal, <0 if older. */
int			hellish_version_cmp(const char *a, const char *b);

/* Read the latest release tag cached by the last background check into `out`
   (NUL-terminated, no leading 'v'). Returns 1 on success, 0 if no cache. */
int			read_cached_latest(char *out, size_t n);

/* If interactive and the cache is stale (>24h), fork a detached child that
   fetches the latest tag from GitHub and rewrites the cache. Never blocks. */
void		maybe_spawn_update_check(t_shell *state);

/* The background worker: fetch the latest tag and write it to the cache. */
void		run_bg_update_check(void);

/* Build "$HOME/.cache/hellish/latest" into buf; 1 on success. */
int			hellish_cache_path(char *buf, size_t n);

/* Persist the latest tag to the cache file (creating ~/.cache/hellish). */
int			hellish_write_cache(const char *tag);

/* Fetch the latest release tag from GitHub via curl; 1 on success. Blocks up
   to a few seconds, so only call it from the background worker or `update`. */
int			fetch_latest_tag(char *out, size_t n);

/* The welcome banner, shown once at interactive startup. */
void		show_welcome(t_shell *state);

#endif
