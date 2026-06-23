/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ai.h                                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: dlesieur <dlesieur@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/22 00:00:00 by dlesieur          #+#    #+#             */
/*   Updated: 2026/06/22 00:00:00 by dlesieur         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AI_H
# define AI_H

# include "shell.h"
# include "libft.h"

/* 256 KB cap on a reply (max_tokens keeps responses far smaller). */
# define AI_MAX_REPLY 262144

/* Config, read from the environment on demand (no struct kept on t_shell):
   HELLISH_AI_HOST (default 127.0.0.1), HELLISH_AI_PORT (8080),
   HELLISH_AI_TIMEOUT_MS (20000). Returned host pointer is borrowed. */
char	*ai_host(void);
int		ai_port(void);
int		ai_timeout_ms(void);

/* Mirror the HELLISH_AI_* vars from hellish's env store into the process
   environ so getenv() (and the forked readline child) see exported config. */
void	ai_sync_env(t_shell *state);

/* Per interactive REPL turn: sync the AI env (interactive only) + refresh the
   background pro-tip. Called once per loop turn from repl_shell. */
void	ai_prompt_prep(t_shell *state);

/* TCP connect to host:port with send/recv timeouts. -1 on failure. */
int		ai_connect(const char *host, int port, int timeout_ms);
int		ai_send_all(int fd, const char *buf, size_t len);
char	*ai_read_all(int fd);

/* HTTP/1.1 POST/GET to path; returns the response body (xfree it) or NULL. */
char	*ai_post(const char *path, const char *body, int timeout_ms);
char	*ai_get(const char *path, int timeout_ms);
char	*ai_build_request(const char *path, const char *body);

/* Minimal JSON helpers: escape a value, extract a string value by key. */
char	*ai_json_escape(const char *s);
char	*ai_json_get_str(const char *buf, const char *key);

/* One-shot chat against llama.cpp's OpenAI-compatible endpoint. Returns the
   assistant text (xfree it) or NULL. ai_reachable: 0 if the server answers. */
char	*ai_chat(t_shell *state, const char *user_msg);
int		ai_reachable(void);

/* The core: prepend rich shell context to instruction, then try the provider
   chain (cloud $HELLISH_AI_URL primary -> local fallback). Strips a markdown
   fence when as_cmd. Returns the reply (xfree it) or NULL. */
char	*ai_request(const char *instruction, int as_cmd);

/* Shell-context preamble (os, cwd, git, recent commands, dir). xfree it. */
char	*ai_context(void);

/* POST a JSON body to a cloud endpoint via curl (TLS + optional Bearer key).
   Returns the response body (xfree it) or NULL. */
char	*ai_curl(const char *url, const char *key, const char *body, int tmo);

/* Complete a partial command line (state-free; for the readline keybinding).
   Returns the suggested command (xfree it) or NULL. */
char	*ai_complete_line(const char *line);

/* Unwrap a ```-fenced markdown code block to the bare command, in place. */
void	ai_strip_fence(char *s);

/* GET /v1/models and collect the model ids. Returns a fresh char*[] of fresh
   strings (free each + the array) and writes the count; NULL/0 on failure. */
char	**ai_models(int *count);
void	ai_models_free(char **list, int n);

/* Fuzzy picker: arrow keys + type-to-filter over names[0..n). Returns the
   chosen index into names (or -1 on cancel / no TTY). */
int		ai_select(char **names, int n, const char *title);

/* Async pro-tip: ai_tip_spawn refreshes a cached tip in a detached worker
   (throttled, interactive-only); ai_tip_read returns the cached line (xfree
   it) or NULL. The prompt only ever reads -- it never calls the network. */
char	*ai_tip_read(void);
void	ai_tip_spawn(t_shell *state);

#endif
