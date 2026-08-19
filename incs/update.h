/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/03 13:00:21 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UPDATE_H
# define UPDATE_H

# include "shell.h"
# include "sh_input.h"

/* How this running hellish got onto the machine — each updates differently.
   The two BINARY kinds differ only in who owns the file: a user-local copy
   (~/.local/bin) we can replace ourselves, a system one (/usr/local/bin)
   that needs elevation. Everything else delegates to whatever installed it,
   which is the rule issue #20 asks for: package-managed installs are the
   package manager's business, not ours. */
typedef enum e_origin
{
	ORIGIN_BINARY,
	ORIGIN_NPM,
	ORIGIN_PNPM,
	ORIGIN_DOCKER,
	ORIGIN_SOURCE,
	ORIGIN_BINARY_SYSTEM
}	t_origin;

/* Everything the shell remembers between runs about updates. Persisted to
   $XDG_CACHE_HOME/hellish/state (default ~/.cache/hellish/state) as plain
   key=value lines -- no parser to get wrong, and readable by a human
   debugging a stuck update. A zeroed struct is a valid "know nothing"
   state, so a missing or corrupt file degrades to "check again", never to
   a crash: update state is optional infrastructure. */
typedef struct s_upd_state
{
	char	latest[64];
	long	checked;
	long	notified;
	long	header_shown;
	long	header_rev;
	char	header_ver[64];
}	t_upd_state;

/* Load the persisted update state; zeroes `s` and returns 0 when absent. */
int			update_state_load(t_upd_state *s);

/* Persist it (temp file + rename, so a reader never sees a half record). */
int			update_state_save(const t_upd_state *s);

/* Path of a file inside the hellish cache dir, honouring XDG_CACHE_HOME. */
int			update_cache_file(const char *name, char *buf, size_t n);

/* Create that directory chain; 1 when it is writable afterwards. */
int			update_cache_mkdir(void);

/* Should the welcome header be drawn this time? See banner_gate.c. */
int			banner_should_show(void);

/* Record that it was just drawn. */
void		banner_mark_shown(void);

/* True when the known latest release is newer than this build. */
int			update_available(const t_upd_state *s);

/* Classify the running binary by inspecting /proc/self/exe; for a source
   checkout, `repo` is filled with the repository root. */
t_origin	detect_origin(char *repo, size_t n);

/* A human label for the origin ("npm", "docker", "source checkout", …). */
const char	*origin_label(t_origin o);

/* The shell command that upgrades this origin, written into out. */
void		origin_command(t_origin o, const char *repo, char *out, size_t n);

/* Run the origin's upgrade command (or print guidance for docker). */
int			run_origin_update(t_origin o, const char *repo, const char *tag);

/* One-line "an update is waiting" notice, emitted between commands so it
   can never land in the middle of a line the user is typing. Once per
   discovered version. */
void		update_notify_prompt(t_shell *state);

/* One-line "an update is waiting" notice, emitted between commands so it
   can never land in the middle of a line the user is typing. Once per
   discovered version. */
void		update_notify_prompt(t_shell *state);

/* Download one URL to a local path; 1 on success. */
int			update_download(const char *url, const char *dest, long min);

/* Verify a download against the checksum published beside the asset.
   1 verified, 0 REJECTED, -1 the release publishes no checksum. */
int			update_verify_sha(const char *tag, const char *asset,
				const char *file);

/* `update --now`: discover the latest release and install it. */
int			update_now(t_shell *state, t_origin origin, char *repo);

/* Replace this machine's hellish binary with release `tag` in place. */
int			update_selfupdate(t_origin o, const char *tag);

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

/* Download, verify and atomically install release `tag` over `target`.
   0 on success, or a step code (1 unsupported platform, 2 download,
   3 checksum rejected, 4 binary would not run, 5 replacement failed);
   on every failure the installed binary is left untouched. */
int			update_apply(const char *tag, const char *target, int sudo);

/* Seconds since the last successful check; -1 when none has ever run. */
long		update_last_check_age(void);

/* Fetch the latest release tag via curl. 1 = got a tag, 0 = source
   unreachable, -1 = source answered but published no release. Blocks up to a
   few seconds, so only call it from the background worker or `update`. */
int			fetch_latest_tag(char *out, size_t n);

/* Run argv and capture its stdout (stderr is discarded). Bytes read, or -1. */
ssize_t		update_capture(char *const argv[], char *out, size_t n);

/* The release-metadata URL: $HELLISH_UPDATE_API, else the GitHub API. */
void		update_api_url(char *out, size_t n);

/* The asset name this OS/arch can run; 0 when nothing is published for it. */
int			update_asset_name(char *out, size_t n);

/* Download URL for one asset of release `tag`; 0 if the base is not https. */
int			update_asset_url(const char *tag, const char *asset,
				char *out, size_t n);

/* Path of the running executable, from /proc/self/exe. 1 on success. */
int			update_exe_path(char *buf, size_t n);

/* True when replacing `path` needs elevation: its DIRECTORY is not
   writable. */
int			update_needs_sudo(const char *path);

/* Download release `tag`, verify it, and atomically replace the running
   binary. Returns 0 on success, non-zero on any failure -- and on failure
   the installed binary is guaranteed untouched. */
int			self_update(const char *tag);

/* The welcome banner, shown once at interactive startup. */
void		show_welcome(t_shell *state);

#endif
